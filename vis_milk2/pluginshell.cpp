/*
  LICENSE
  -------
  Copyright 2005-2013 Nullsoft, Inc.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the name of Nullsoft nor the names of its contributors may be used to
      endorse or promote products derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
  FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "pch.h"
//#include "api.h"
#include "pluginshell.h"
#include "utility.h"
#include "defines.h"
#include "shell_defines.h"
//#include "resource.h"
//#include "vis.h"
//#include <multimon.h>
//#include "../Winamp/wa_ipc.h"
//#include "../nu/AutoCharFn.h"
#include <mmsystem.h>
#include "DirectXHelpers.h"
#pragma comment(lib, "winmm.lib") // for timeGetTime

// STATE VALUES & VERTEX FORMATS FOR HELP SCREEN TEXTURE:
#define TEXT_SURFACE_NOT_READY 0
#define TEXT_SURFACE_REQUESTED 1
#define TEXT_SURFACE_READY     2
#define TEXT_SURFACE_ERROR     3

typedef struct _HELPVERTEX
{
    float x, y;    // screen position
    float z;       // Z-buffer depth
    DWORD Diffuse; // diffuse color. also acts as filler; aligns struct to 16 bytes (good for random access/indexed prims)
    float tu, tv;  // texture coordinates for texture #0
} HELPVERTEX, *LPHELPVERTEX;
#define HELP_VERTEX_FORMAT (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)

typedef struct _SIMPLEVERTEX
{
    float x, y;    // screen position
    float z;       // Z-buffer depth
    DWORD Diffuse; // diffuse color. also acts as filler; aligns struct to 16 bytes (good for random access/indexed prims)
} SIMPLEVERTEX, *LPSIMPLEVERTEX;
#define SIMPLE_VERTEX_FORMAT (D3DFVF_XYZ | D3DFVF_DIFFUSE)

extern wchar_t* g_szHelp;
extern int g_szHelp_W;
//extern winampVisModule mod1;

// Resides in `vms_desktop.dll/lib`.
void getItemData(int x);

CPluginShell::CPluginShell() { /* This should remain empty! */ }
CPluginShell::~CPluginShell() { /* This should remain empty! */ }

eScrMode CPluginShell::GetScreenMode() { return m_screenmode; }
int CPluginShell::GetFrame() { return m_frame; }
float CPluginShell::GetTime() { return m_time; }
float CPluginShell::GetFps() { return m_fps; }
HWND CPluginShell::GetPluginWindow() const { if (m_lpDX) return m_lpDX->GetHwnd(); else return NULL; }
int CPluginShell::GetWidth() { if (m_lpDX) return m_lpDX->m_client_width; else return 0; }
int CPluginShell::GetHeight() { if (m_lpDX) return m_lpDX->m_client_height; else return 0; }
int CPluginShell::GetCanvasMarginX() { return 0; }
int CPluginShell::GetCanvasMarginY() { return 0; }
HWND CPluginShell::GetWinampWindow() { return m_hWndWinamp; }
void CPluginShell::SetWinampWindow(HWND window) { m_hWndWinamp = window; }
HINSTANCE CPluginShell::GetInstance() { return m_hInstance; }
wchar_t* CPluginShell::GetPluginsDirPath() { return m_szPluginsDirPath; }
wchar_t* CPluginShell::GetConfigIniFile() { return m_szConfigIniFile; }
char* CPluginShell::GetConfigIniFileA() { return m_szConfigIniFileA; }
int CPluginShell::GetFontHeight(eFontIndex idx) { if (idx >= 0 && idx < NUM_BASIC_FONTS + NUM_EXTRA_FONTS) return m_fontinfo[idx].nSize; else return 0; };
int CPluginShell::GetBitDepth() { return m_lpDX->GetBitDepth(); };
D3D11Shim* CPluginShell::GetDevice() { return m_lpDX->m_lpDevice.get(); };
#if 0
D3DCAPS9* CPluginShell::GetCaps() { if (m_lpDX) return &(m_lpDX->m_caps); else return NULL; };
D3DFORMAT CPluginShell::GetBackBufFormat() { if (m_lpDX) return m_lpDX->m_current_mode.display_mode.Format; else return D3DFMT_UNKNOWN; };
D3DFORMAT CPluginShell::GetBackBufZFormat() { if (m_lpDX) return m_lpDX->GetZFormat(); else return D3DFMT_UNKNOWN; };
#endif
char* CPluginShell::GetDriverFilename() { static char fake[1] = {0}; return fake; }
char* CPluginShell::GetDriverDescription() { static char fake[1] = {0}; return fake; }

int CPluginShell::InitNonDX11()
{
#ifdef TARGET_WINDOWS_DESKTOP
    timeBeginPeriod(1);
#endif
    m_fftobj.Init(576, NUM_FREQUENCIES);
    return AllocateMilkDropNonDX11();
}

void CPluginShell::CleanUpNonDX11()
{
    CleanUpMilkDropNonDX11();
    m_fftobj.CleanUp();
}

int CPluginShell::AllocateDX11()
{
    int ret = AllocateMilkDropDX11();

    // Invalidate various 'caches' here.
    m_playlist_top_idx = -1; // Invalidating playlist cache forces recompute of playlist width

    return ret;
}

void CPluginShell::CleanUpDX11(int final_cleanup)
{
    // Always unbind the textures before releasing textures,
    // otherwise they might still have a hanging reference!
    if (m_lpDX && m_lpDX->m_lpDevice)
    {
        for (int i = 0; i < 16; i++)
            m_lpDX->m_lpDevice->SetTexture(i, NULL);
    }

    if (!m_vj_mode)
    {
        for (int i = 0; i < NUM_BASIC_FONTS + NUM_EXTRA_FONTS; i++)
            SafeRelease(m_d3dx_font[i]);
        SafeRelease(m_lpDDSText);
    }

    CleanUpMilkDropDX11(final_cleanup);
}

void CPluginShell::OnWindowSizeChanged(int width, int height)
{
    // Update window properties
    if (!m_lpDX->OnWindowSizeChanged(width, height))
    {
        return;
    }
    //if (!AllocateDX11())
    //{
    //    m_lpDX->m_ready = false; // flag to exit
    //    return;
    //}
}

void CPluginShell::OnWindowSwap(HWND window, int width, int height)
{
    if (!m_lpDX->OnWindowSwap(window, width, height))
        return;
}

void CPluginShell::OnWindowMoved()
{
    m_lpDX->OnWindowMoved();
}

void CPluginShell::OnDisplayChange()
{
    m_lpDX->OnDisplayChange();
}

void CPluginShell::StuffParams(DXCONTEXT_PARAMS* pParams)
{
    pParams->screenmode = m_screenmode;
    pParams->back_buffer_format = static_cast<DXGI_FORMAT>(m_back_buffer_format);
    pParams->depth_buffer_format = static_cast<DXGI_FORMAT>(m_depth_buffer_format);
    pParams->back_buffer_count = m_back_buffer_count;
    pParams->min_feature_level = static_cast<D3D_FEATURE_LEVEL>(m_min_feature_level);
    pParams->m_skin = (m_screenmode == WINDOWED) ? m_skin : 0;
    pParams->enable_hdr = m_enable_hdr;
    switch (m_screenmode)
    {
        case WINDOWED:
            pParams->allow_page_tearing = m_allow_page_tearing_w;
            pParams->adapter_guid = m_adapter_guid_windowed;
            pParams->msaa = m_multisample_windowed;
            wcscpy_s(pParams->adapter_devicename, m_adapter_devicename_windowed);
            break;
        case FULLSCREEN:
        case FAKE_FULLSCREEN:
            pParams->allow_page_tearing = m_allow_page_tearing_fs;
            pParams->adapter_guid = m_adapter_guid_fullscreen;
            pParams->msaa = m_multisample_fullscreen;
            wcscpy_s(pParams->adapter_devicename, m_adapter_devicename_fullscreen);
            break;
        case DESKTOP:
            pParams->allow_page_tearing = m_allow_page_tearing_dm;
            pParams->adapter_guid = m_adapter_guid_desktop;
            pParams->msaa = m_multisample_desktop;
            wcscpy_s(pParams->adapter_devicename, m_adapter_devicename_desktop);
            break;
    }
    pParams->parent_window = m_hWndWinamp; //(m_screenmode == DESKTOP) ? m_hWndDesktopListView : NULL;
}

void CPluginShell::ToggleFullScreen()
{
    CleanUpDX11(0);

    switch (m_screenmode)
    {
        case DESKTOP:
        case WINDOWED:
            m_screenmode = FULLSCREEN;
            break;
        case FULLSCREEN:
        case FAKE_FULLSCREEN:
            m_screenmode = WINDOWED;
            break;
    }

    DXCONTEXT_PARAMS params{};
    StuffParams(&params);

    if (!m_lpDX->StartOrRestartDevice(&params))
    {
        // Note: A basic warning message box will have already been given.
        if (m_lpDX->m_lastErr == DXC_ERR_CREATEDEV_PROBABLY_OUTOFVIDEOMEMORY)
        {
            // Make specific suggestions on how to regain more video memory.
            //SuggestHowToFreeSomeMem();
        }
        return;
    }

    if (!AllocateDX11())
    {
        m_lpDX->m_ready = false; // flag to exit
        return;
    }

    //SetForegroundWindow(m_lpDX->GetHwnd());
    //SetActiveWindow(m_lpDX->GetHwnd());
    //SetFocus(m_lpDX->GetHwnd());
}

int CPluginShell::InitDirectX()
{
    DXCONTEXT_PARAMS params{};
    StuffParams(&params);

    m_lpDX = std::make_unique<DXContext>(m_hWndWinamp, &params); //, m_szConfigIniFile, m_hInstance, CLASSNAME, WINDOWCAPTION, CPluginShell::WindowProc, (LONG_PTR)this, m_minimize_winamp

    if (!m_lpDX)
    {
        PopupMessage(IDS_UNABLE_TO_INIT_DXCONTEXT, IDS_MILKDROP_ERROR);
        return FALSE;
    }

    if (m_lpDX->m_lastErr != S_OK)
    {
        // Warning message box will have already been given.
        m_lpDX.reset(); //delete m_lpDX;
        return FALSE;
    }

    // Initialize graphics.
    if (!m_lpDX->StartOrRestartDevice(&params))
    {
        // Note: A basic warning message box will have already been given.
        /* if (m_lpDX->m_lastErr == DXC_ERR_CREATEDEV_PROBABLY_OUTOFVIDEOMEMORY)
        {
            // Make specific suggestions on how to regain more video memory.
            SuggestHowToFreeSomeMem();
        } */

        m_lpDX.reset();

        return FALSE;
    }

    return TRUE;
}

void CPluginShell::CleanUpDirectX()
{
    if (m_lpDX) m_lpDX.reset();
    //if (m_device) m_device->Release();
}

int CPluginShell::PluginPreInitialize(HWND hWinampWnd, HINSTANCE hWinampInstance)
{
    // PROTECTED CONFIG PANEL SETTINGS (also see 'private' settings, below)
    m_start_fullscreen = 0;
    m_start_desktop = 0;
    m_fake_fullscreen_mode = 0;
    m_max_fps_fs = 30;
    m_max_fps_dm = 30;
    m_max_fps_w = 30;
    m_show_press_f1_msg = 1;
    m_allow_page_tearing_w = 0;
    m_allow_page_tearing_fs = 0;
    m_allow_page_tearing_dm = 0;
    m_minimize_winamp = 1;
    m_desktop_show_icons = 1;
    m_desktop_textlabel_boxes = 1;
    m_desktop_manual_icon_scoot = 0;
    m_desktop_555_fix = 2;
    m_dualhead_horz = 2;
    m_dualhead_vert = 1;
    m_save_cpu = 1;
    m_skin = 1;
    m_fix_slow_text = 0;
    m_enable_hdr = 0;
    m_enable_downmix = 0;
    m_back_buffer_format = DXGI_FORMAT_UNKNOWN; 
    m_depth_buffer_format = DXGI_FORMAT_UNKNOWN;
    m_back_buffer_count = 2;
    m_min_feature_level = D3D_FEATURE_LEVEL_9_1;

    // Initialize font settings.
    wcscpy_s(m_fontinfo[SIMPLE_FONT].szFace, SIMPLE_FONT_DEFAULT_FACE);
    m_fontinfo[SIMPLE_FONT].nSize = SIMPLE_FONT_DEFAULT_SIZE;
    m_fontinfo[SIMPLE_FONT].bBold = SIMPLE_FONT_DEFAULT_BOLD;
    m_fontinfo[SIMPLE_FONT].bItalic = SIMPLE_FONT_DEFAULT_ITAL;
    m_fontinfo[SIMPLE_FONT].bAntiAliased = SIMPLE_FONT_DEFAULT_AA;
    wcscpy_s(m_fontinfo[DECORATIVE_FONT].szFace, DECORATIVE_FONT_DEFAULT_FACE);
    m_fontinfo[DECORATIVE_FONT].nSize = DECORATIVE_FONT_DEFAULT_SIZE;
    m_fontinfo[DECORATIVE_FONT].bBold = DECORATIVE_FONT_DEFAULT_BOLD;
    m_fontinfo[DECORATIVE_FONT].bItalic = DECORATIVE_FONT_DEFAULT_ITAL;
    m_fontinfo[DECORATIVE_FONT].bAntiAliased = DECORATIVE_FONT_DEFAULT_AA;
    wcscpy_s(m_fontinfo[HELPSCREEN_FONT].szFace, HELPSCREEN_FONT_DEFAULT_FACE);
    m_fontinfo[HELPSCREEN_FONT].nSize = HELPSCREEN_FONT_DEFAULT_SIZE;
    m_fontinfo[HELPSCREEN_FONT].bBold = HELPSCREEN_FONT_DEFAULT_BOLD;
    m_fontinfo[HELPSCREEN_FONT].bItalic = HELPSCREEN_FONT_DEFAULT_ITAL;
    m_fontinfo[HELPSCREEN_FONT].bAntiAliased = HELPSCREEN_FONT_DEFAULT_AA;
    wcscpy_s(m_fontinfo[PLAYLIST_FONT].szFace, PLAYLIST_FONT_DEFAULT_FACE);
    m_fontinfo[PLAYLIST_FONT].nSize = PLAYLIST_FONT_DEFAULT_SIZE;
    m_fontinfo[PLAYLIST_FONT].bBold = PLAYLIST_FONT_DEFAULT_BOLD;
    m_fontinfo[PLAYLIST_FONT].bItalic = PLAYLIST_FONT_DEFAULT_ITAL;
    m_fontinfo[PLAYLIST_FONT].bAntiAliased = PLAYLIST_FONT_DEFAULT_AA;

#if (NUM_EXTRA_FONTS >= 1)
    wcscpy_s(m_fontinfo[NUM_BASIC_FONTS + 0].szFace, EXTRA_FONT_1_DEFAULT_FACE);
    m_fontinfo[NUM_BASIC_FONTS + 0].nSize = EXTRA_FONT_1_DEFAULT_SIZE;
    m_fontinfo[NUM_BASIC_FONTS + 0].bBold = EXTRA_FONT_1_DEFAULT_BOLD;
    m_fontinfo[NUM_BASIC_FONTS + 0].bItalic = EXTRA_FONT_1_DEFAULT_ITAL;
    m_fontinfo[NUM_BASIC_FONTS + 0].bAntiAliased = EXTRA_FONT_1_DEFAULT_AA;
#endif
#if (NUM_EXTRA_FONTS >= 2)
    wcscpy_s(m_fontinfo[NUM_BASIC_FONTS + 1].szFace, EXTRA_FONT_2_DEFAULT_FACE);
    m_fontinfo[NUM_BASIC_FONTS + 1].nSize = EXTRA_FONT_2_DEFAULT_SIZE;
    m_fontinfo[NUM_BASIC_FONTS + 1].bBold = EXTRA_FONT_2_DEFAULT_BOLD;
    m_fontinfo[NUM_BASIC_FONTS + 1].bItalic = EXTRA_FONT_2_DEFAULT_ITAL;
    m_fontinfo[NUM_BASIC_FONTS + 1].bAntiAliased = EXTRA_FONT_2_DEFAULT_AA;
#endif
#if (NUM_EXTRA_FONTS >= 3)
    wcscpy_s(m_fontinfo[NUM_BASIC_FONTS + 2].szFace, EXTRA_FONT_3_DEFAULT_FACE);
    m_fontinfo[NUM_BASIC_FONTS + 2].nSize = EXTRA_FONT_3_DEFAULT_SIZE;
    m_fontinfo[NUM_BASIC_FONTS + 2].bBold = EXTRA_FONT_3_DEFAULT_BOLD;
    m_fontinfo[NUM_BASIC_FONTS + 2].bItalic = EXTRA_FONT_3_DEFAULT_ITAL;
    m_fontinfo[NUM_BASIC_FONTS + 2].bAntiAliased = EXTRA_FONT_3_DEFAULT_AA;
#endif
#if (NUM_EXTRA_FONTS >= 4)
    wcscpy_s(m_fontinfo[NUM_BASIC_FONTS + 3].szFace, EXTRA_FONT_4_DEFAULT_FACE);
    m_fontinfo[NUM_BASIC_FONTS + 3].nSize = EXTRA_FONT_4_DEFAULT_SIZE;
    m_fontinfo[NUM_BASIC_FONTS + 3].bBold = EXTRA_FONT_4_DEFAULT_BOLD;
    m_fontinfo[NUM_BASIC_FONTS + 3].bItalic = EXTRA_FONT_4_DEFAULT_ITAL;
    m_fontinfo[NUM_BASIC_FONTS + 3].bAntiAliased = EXTRA_FONT_4_DEFAULT_AA;
#endif
#if (NUM_EXTRA_FONTS >= 5)
    wcscpy_s(m_fontinfo[NUM_BASIC_FONTS + 4].szFace, EXTRA_FONT_5_DEFAULT_FACE);
    m_fontinfo[NUM_BASIC_FONTS + 4].nSize = EXTRA_FONT_5_DEFAULT_SIZE;
    m_fontinfo[NUM_BASIC_FONTS + 4].bBold = EXTRA_FONT_5_DEFAULT_BOLD;
    m_fontinfo[NUM_BASIC_FONTS + 4].bItalic = EXTRA_FONT_5_DEFAULT_ITAL;
    m_fontinfo[NUM_BASIC_FONTS + 4].bAntiAliased = EXTRA_FONT_5_DEFAULT_AA;
#endif

    // PROTECTED STRUCTURES/POINTERS
    for (int i = 0; i < NUM_BASIC_FONTS + NUM_EXTRA_FONTS; i++)
        m_d3dx_font[i] = NULL;
    m_d3dx_desktop_font = nullptr;
    m_lpDDSText = nullptr;
    ZeroMemory(&m_sound, sizeof(td_soundinfo));
    for (int ch = 0; ch < 2; ch++)
        for (int i = 0; i < 3; i++)
        {
            m_sound.infinite_avg[ch][i] = m_sound.avg[ch][i] = m_sound.med_avg[ch][i] = m_sound.long_avg[ch][i] = 1.0f;
        }

    // GENERAL PRIVATE STUFF
    //m_screenmode: set at end (derived setting)
    m_frame = 0;
    m_time = 0;
    m_fps = 30;
    m_hWndWinamp = hWinampWnd;
    m_hInstance = hWinampInstance;
    m_lpDX = nullptr;
    //m_szPluginsDirPath[0] = 0;  // will be set further down
    //m_szConfigIniFile[0] = 0;  // will be set further down
    //m_szPluginsDirPath:

    /*
    wchar_t *p;
    if (hWinampWnd
        && (p = (wchar_t *)SendMessage(hWinampWnd, WM_WA_IPC, 0, IPC_GETPLUGINDIRECTORYW))
        && p != (wchar_t *)1)
    {
        swprintf(m_szPluginsDirPath, L"%s\\", p);
    }
    else
    {
        // Get path to INI file & read in prefs/settings right away, so DumpMsg works!
        GetModuleFileNameW(m_hInstance, m_szPluginsDirPath, MAX_PATH);
        wchar_t *p = m_szPluginsDirPath + wcslen(m_szPluginsDirPath);
        while (p >= m_szPluginsDirPath && *p != L'\\') p--;
        if (++p >= m_szPluginsDirPath) *p = 0;
    }
    */
    //	swprintf(m_szPluginsDirPath, L"");

    /*
    if (hWinampWnd
        && (p = (wchar_t *)SendMessage(hWinampWnd, WM_WA_IPC, 0, IPC_GETINIDIRECTORYW))
        && p != (wchar_t *)1)
    {
        // load settings as well as coping with moving old settings to a contained folder
        wchar_t m_szOldConfigIniFile[MAX_PATH] = {0}, temp[MAX_PATH] = {0}, temp2[MAX_PATH] = {0};
        swprintf(m_szOldConfigIniFile, L"%s\\Plugins\\%s", p, INIFILE);
        swprintf(m_szConfigIniFile, L"%s\\Plugins\\%s%s", p, SUBDIR, INIFILE);
        swprintf(temp, L"%s\\Plugins\\%s", p, SUBDIR);
        swprintf(temp2, L"%s\\Plugins\\", p);
        CreateDirectoryW(temp, NULL);

        if (PathFileExistsW(m_szOldConfigIniFile) && !PathFileExistsW(m_szConfigIniFile))
        {
            MoveFileW(m_szOldConfigIniFile, m_szConfigIniFile);

            wchar_t m_szMsgIniFile[MAX_PATH] = {0}, m_szNewMsgIniFile[MAX_PATH] = {0},
                    m_szImgIniFile[MAX_PATH] = {0}, m_szNewImgIniFile[MAX_PATH] = {0},
                    m_szAdaptersFile[MAX_PATH] = {0}, m_szNewAdaptersFile[MAX_PATH] = {0};
            swprintf(m_szMsgIniFile, L"%s%s", temp2, MSG_INIFILE);
            swprintf(m_szNewMsgIniFile, L"%s%s", temp, MSG_INIFILE);
            swprintf(m_szImgIniFile, L"%s%s", temp2, IMG_INIFILE);
            swprintf(m_szNewImgIniFile, L"%s%s", temp, IMG_INIFILE);
            swprintf(m_szAdaptersFile, L"%s%s", temp2, ADAPTERSFILE);
            swprintf(m_szNewAdaptersFile, L"%s%s", temp, ADAPTERSFILE);

            MoveFileW(m_szImgIniFile, m_szNewImgIniFile);
            MoveFileW(m_szMsgIniFile, m_szNewMsgIniFile);
            MoveFileW(m_szAdaptersFile, m_szNewAdaptersFile);
        }
    }
    else
    {
        swprintf(m_szConfigIniFile, L"%s%s", m_szPluginsDirPath, INIFILE);
    }
    lstrcpyn(m_szConfigIniFileA,AutoCharFn(m_szConfigIniFile),MAX_PATH);
    */

    // PRIVATE CONFIG PANEL SETTINGS
    m_multisample_fullscreen = {1u, 0u}; //D3DMULTISAMPLE_NONE;
    m_multisample_desktop = {1u, 0u}; //D3DMULTISAMPLE_NONE;
    m_multisample_windowed = {1u, 0u}; //D3DMULTISAMPLE_NONE;
    ZeroMemory(&m_adapter_guid_fullscreen, sizeof(GUID));
    ZeroMemory(&m_adapter_guid_desktop , sizeof(GUID));
    ZeroMemory(&m_adapter_guid_windowed , sizeof(GUID));
    m_adapter_devicename_windowed[0] = '\0';
    m_adapter_devicename_fullscreen[0] = '\0';
    m_adapter_devicename_desktop[0] = '\0';

    // PRIVATE RUNTIME SETTINGS
    m_lost_focus = 0;
    m_hidden = 0;
    m_resizing = 0;
    m_show_help = 0;
    m_show_playlist = 0;
    m_playlist_pos = 0;
    m_playlist_pageups = 0;
    m_playlist_top_idx = -1;
    m_playlist_btm_idx = -1;
    // `m_playlist_width_pixels` will be considered invalid whenever `m_playlist_top_idx` is -1.
    // `m_playlist[256][256]` will be considered invalid whenever `m_playlist_top_idx` is -1.
    m_exiting = 0;
    m_upper_left_corner_y = 0;
    m_lower_left_corner_y = 0;
    m_upper_right_corner_y = 0;
    m_lower_right_corner_y = 0;
    m_left_edge = 0;
    m_right_edge = 0;
    m_force_accept_WM_WINDOWPOSCHANGING = 0;

    // PRIVATE - GDI STUFF
    m_main_menu = NULL;
    m_context_menu = NULL;
    for (int i = 0; i < NUM_BASIC_FONTS + NUM_EXTRA_FONTS; i++)
        m_font[i] = NULL;
    m_font_desktop = NULL;

    // PRIVATE - MORE TIMEKEEPING
    m_last_raw_time = 0;
    memset(m_time_hist, 0, sizeof(m_time_hist));
    m_time_hist_pos = 0;
    if (!QueryPerformanceFrequency(&m_high_perf_timer_freq))
    {
        throw std::exception(); //m_high_perf_timer_freq.QuadPart = 0;
    }
    m_prev_end_of_frame.QuadPart = 0;

    // PRIVATE AUDIO PROCESSING DATA
    //(m_fftobj needs no init)
    memset(m_oldwave[0], 0, sizeof(float) * 576);
    memset(m_oldwave[1], 0, sizeof(float) * 576);
    m_prev_align_offset[0] = 0;
    m_prev_align_offset[1] = 0;
    m_align_weights_ready = 0;

    // SEPARATE TEXT WINDOW (FOR VJ MODE)
    m_vj_mode = 0;
    m_hidden_textwnd = 0;
    m_resizing_textwnd = 0;
    m_hTextWnd = NULL;
    m_nTextWndWidth = 0;
    m_nTextWndHeight = 0;
    m_bTextWindowClassRegistered = false;
    //m_vjd3d9 = NULL;
    //m_vjd3d9_device = NULL;

    //-----

    m_screenmode = FULLSCREEN;

    OverrideDefaults();
    ReadConfig();

    MilkDropPreInitialize();
    MilkDropReadConfig();

    return TRUE;
}

int CPluginShell::PluginInitialize(int iWidth, int iHeight)
{
    if (!InitDirectX()) return FALSE;
    m_lpDX->m_client_width = iWidth;
    m_lpDX->m_client_height = iHeight;
    if (!InitNonDX11()) return FALSE;
    if (!AllocateDX11()) return FALSE;
    //if (!InitVJ()) return FALSE;

    return TRUE;
}

void CPluginShell::PluginQuit()
{
    //CleanUpVJ();
    CleanUpDX11(1);
    CleanUpNonDX11();
    CleanUpDirectX();
}

wchar_t* BuildSettingName(const wchar_t* name, const int number)
{
    static wchar_t temp[64];
    swprintf_s(temp, L"%s%d", name, number);
    return temp;
}

void CPluginShell::ReadFont(const int /*n*/)
{
#ifndef _FOOBAR
    GetPrivateProfileString(L"settings", BuildSettingName(L"szFontFace", n), m_fontinfo[n].szFace, m_fontinfo[n].szFace, ARRAYSIZE(m_fontinfo[n].szFace), m_szConfigIniFile);
    m_fontinfo[n].nSize = GetPrivateProfileInt(L"settings", BuildSettingName(L"nFontSize", n), m_fontinfo[n].nSize, m_szConfigIniFile);
    m_fontinfo[n].bBold = GetPrivateProfileInt(L"settings", BuildSettingName(L"bFontBold", n), m_fontinfo[n].bBold, m_szConfigIniFile);
    m_fontinfo[n].bItalic = GetPrivateProfileInt(L"settings", BuildSettingName(L"bFontItalic", n), m_fontinfo[n].bItalic, m_szConfigIniFile);
    m_fontinfo[n].bAntiAliased = GetPrivateProfileInt(L"settings", BuildSettingName(L"bFontAA", n), m_fontinfo[n].bItalic, m_szConfigIniFile);
#endif
}

void CPluginShell::ReadConfig()
{
#ifndef _FOOBAR
    int old_ver = GetPrivateProfileInt(L"settings", L"version", -1, m_szConfigIniFile);
    int old_subver = GetPrivateProfileInt(L"settings", L"subversion", -1, m_szConfigIniFile);

    // Nuke old settings from previous version.
    if (old_ver < INT_VERSION)
        return;
    else if (old_subver < INT_SUBVERSION)
        return;

    m_multisample_fullscreen = {static_cast<UINT>(GetPrivateProfileInt(L"settings", L"multisample_fullscreen", m_multisample_fullscreen.Count, m_szConfigIniFile)), 0U};
    m_multisample_desktop = {static_cast<UINT>(GetPrivateProfileInt(L"settings", L"multisample_desktop", m_multisample_desktop.Count, m_szConfigIniFile)), 0U};
    m_multisample_windowed = {static_cast<UINT>(GetPrivateProfileInt(L"settings", L"multisample_windowed", m_multisample_windowed.Count, m_szConfigIniFile)), 0U};

    char str[256];
    GetPrivateProfileStringA("settings", "adapter_guid_fullscreen", "", str, sizeof(str), m_szConfigIniFileA);
    TextToLuidA(str, &m_adapter_guid_fullscreen);
    GetPrivateProfileStringA("settings", "adapter_guid_desktop", "", str, sizeof(str), m_szConfigIniFileA);
    TextToLuidA(str, &m_adapter_guid_desktop);
    GetPrivateProfileStringA("settings", "adapter_guid_windowed", "", str, sizeof(str), m_szConfigIniFileA);
    TextToLuidA(str, &m_adapter_guid_windowed);
    GetPrivateProfileString(L"settings", L"adapter_devicename_fullscreen", L"", m_adapter_devicename_fullscreen, ARRAYSIZE(m_adapter_devicename_fullscreen), m_szConfigIniFile);
    GetPrivateProfileString(L"settings", L"adapter_devicename_desktop", L"", m_adapter_devicename_desktop, ARRAYSIZE(m_adapter_devicename_desktop), m_szConfigIniFile);
    GetPrivateProfileString(L"settings", L"adapter_devicename_windowed", L"", m_adapter_devicename_windowed, ARRAYSIZE(m_adapter_devicename_windowed), m_szConfigIniFile);

    // FONTS
    ReadFont(0);
    ReadFont(1);
    ReadFont(2);
    ReadFont(3);
#if (NUM_EXTRA_FONTS >= 1)
    ReadFont(4);
#endif
#if (NUM_EXTRA_FONTS >= 2)
    ReadFont(5);
#endif
#if (NUM_EXTRA_FONTS >= 3)
    ReadFont(6);
#endif
#if (NUM_EXTRA_FONTS >= 4)
    ReadFont(7);
#endif
#if (NUM_EXTRA_FONTS >= 5)
    ReadFont(8);
#endif

    m_start_fullscreen = GetPrivateProfileInt(L"settings", L"start_fullscreen", m_start_fullscreen, m_szConfigIniFile);
    m_start_desktop = GetPrivateProfileInt(L"settings", L"start_desktop", m_start_desktop, m_szConfigIniFile);
    m_fake_fullscreen_mode = GetPrivateProfileInt(L"settings", L"fake_fullscreen_mode", m_fake_fullscreen_mode, m_szConfigIniFile);
    m_max_fps_fs = GetPrivateProfileInt(L"settings", L"max_fps_fs", m_max_fps_fs, m_szConfigIniFile);
    m_max_fps_dm = GetPrivateProfileInt(L"settings", L"max_fps_dm", m_max_fps_dm, m_szConfigIniFile);
    m_max_fps_w = GetPrivateProfileInt(L"settings", L"max_fps_w", m_max_fps_w, m_szConfigIniFile);
    m_show_press_f1_msg = GetPrivateProfileInt(L"settings", L"show_press_f1_msg", m_show_press_f1_msg, m_szConfigIniFile);
    m_allow_page_tearing_w = GetPrivateProfileInt(L"settings", L"allow_page_tearing_w", m_allow_page_tearing_w, m_szConfigIniFile);
    m_allow_page_tearing_fs = GetPrivateProfileInt(L"settings", L"allow_page_tearing_fs", m_allow_page_tearing_fs, m_szConfigIniFile);
    m_allow_page_tearing_dm = GetPrivateProfileInt(L"settings", L"allow_page_tearing_dm", m_allow_page_tearing_dm, m_szConfigIniFile);
    m_minimize_winamp = GetPrivateProfileInt(L"settings", L"minimize_winamp", m_minimize_winamp, m_szConfigIniFile);
    m_desktop_show_icons = GetPrivateProfileInt(L"settings", L"desktop_show_icons", m_desktop_show_icons, m_szConfigIniFile);
    m_desktop_textlabel_boxes = GetPrivateProfileInt(L"settings", L"desktop_textlabel_boxes", m_desktop_textlabel_boxes, m_szConfigIniFile);
    m_desktop_manual_icon_scoot = GetPrivateProfileInt(L"settings", L"desktop_manual_icon_scoot", m_desktop_manual_icon_scoot, m_szConfigIniFile);
    m_desktop_555_fix = GetPrivateProfileInt(L"settings", L"desktop_555_fix", m_desktop_555_fix, m_szConfigIniFile);
    m_dualhead_horz = GetPrivateProfileInt(L"settings", L"dualhead_horz", m_dualhead_horz, m_szConfigIniFile);
    m_dualhead_vert = GetPrivateProfileInt(L"settings", L"dualhead_vert", m_dualhead_vert, m_szConfigIniFile);
    m_save_cpu = GetPrivateProfileInt(L"settings", L"save_cpu", m_save_cpu, m_szConfigIniFile);
    m_skin = GetPrivateProfileInt(L"settings", L"skin", m_skin, m_szConfigIniFile);
    m_fix_slow_text = GetPrivateProfileInt(L"settings", L"fix_slow_text", m_fix_slow_text, m_szConfigIniFile);
    m_vj_mode = GetPrivateProfileBoolW(L"settings", L"vj_mode", m_vj_mode, m_szConfigIniFile);

    m_disp_mode_fs.Width = GetPrivateProfileInt(L"settings", L"disp_mode_fs_w", m_disp_mode_fs.Width, m_szConfigIniFile);
    m_disp_mode_fs.Height = GetPrivateProfileInt(L"settings", L"disp_mode_fs_h", m_disp_mode_fs.Height, m_szConfigIniFile);
    GetPrivateProfileStringA("settings", "disp_mode_fs_r", "60000/1000", str, sizeof(str), m_szConfigIniFileA);
    sscanf_s(str, "%d/%d", &m_disp_mode_fs.RefreshRate.Numerator, &m_disp_mode_fs.RefreshRate.Denominator);
    m_disp_mode_fs.Format = static_cast<DXGI_FORMAT>(GetPrivateProfileInt(L"settings", L"disp_mode_fs_f", m_disp_mode_fs.Format, m_szConfigIniFile));

    // Note: Do not call CPlugin's `Preinit()` and `ReadConfig()` until
    //       CPluginShell's `PreInit()` (and `ReadConfig()`) finish.
#endif
}

void CPluginShell::WriteFont(const int /*n*/)
{
#ifndef _FOOBAR
    WritePrivateProfileString(L"settings", BuildSettingName(L"szFontFace", n), m_fontinfo[n].szFace, m_szConfigIniFile);
    WritePrivateProfileIntW(m_fontinfo[n].bBold, BuildSettingName(L"bFontBold", n), m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_fontinfo[n].bItalic, BuildSettingName(L"bFontItalic", n), m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_fontinfo[n].nSize, BuildSettingName(L"nFontSize", n), m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_fontinfo[n].bAntiAliased, BuildSettingName(L"bFontAA", n), m_szConfigIniFile, L"settings");
#endif
}

void CPluginShell::WriteConfig()
{
#ifndef _FOOBAR
    WritePrivateProfileIntW(m_multisample_fullscreen.Count, L"multisample_fullscreen", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_multisample_desktop.Count, L"multisample_desktop", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_multisample_windowed.Count, L"multisample_windowed", m_szConfigIniFile, L"settings");

    char str[256];
    LuidToTextA(&m_adapter_guid_fullscreen, str, sizeof(str));
    WritePrivateProfileStringA("settings", "adapter_guid_fullscreen", str, m_szConfigIniFileA);
    LuidToTextA(&m_adapter_guid_desktop, str, sizeof(str));
    WritePrivateProfileStringA("settings", "adapter_guid_desktop", str, m_szConfigIniFileA);
    LuidToTextA(&m_adapter_guid_windowed, str, sizeof(str));
    WritePrivateProfileStringA("settings", "adapter_guid_windowed", str, m_szConfigIniFileA);
    WritePrivateProfileString(L"settings", L"adapter_devicename_fullscreen", m_adapter_devicename_fullscreen, m_szConfigIniFile);
    WritePrivateProfileString(L"settings", L"adapter_devicename_desktop", m_adapter_devicename_desktop, m_szConfigIniFile);
    WritePrivateProfileString(L"settings", L"adapter_devicename_windowed", m_adapter_devicename_windowed, m_szConfigIniFile);

    // FONTS
    WriteFont(0);
    WriteFont(1);
    WriteFont(2);
    WriteFont(3);
#if (NUM_EXTRA_FONTS >= 1)
    WriteFont(4);
#endif
#if (NUM_EXTRA_FONTS >= 2)
    WriteFont(5);
#endif
#if (NUM_EXTRA_FONTS >= 3)
    WriteFont(6);
#endif
#if (NUM_EXTRA_FONTS >= 4)
    WriteFont(7);
#endif
#if (NUM_EXTRA_FONTS >= 5)
    WriteFont(8);
#endif

    WritePrivateProfileIntW(m_start_fullscreen, L"start_fullscreen", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_start_desktop, L"start_desktop", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_fake_fullscreen_mode, L"fake_fullscreen_mode", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_max_fps_fs, L"max_fps_fs", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_max_fps_dm, L"max_fps_dm", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_max_fps_w, L"max_fps_w", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_show_press_f1_msg, L"show_press_f1_msg", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_allow_page_tearing_w, L"allow_page_tearing_w", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_allow_page_tearing_fs, L"allow_page_tearing_fs", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_allow_page_tearing_dm, L"allow_page_tearing_dm", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_minimize_winamp, L"minimize_winamp", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_desktop_show_icons, L"desktop_show_icons", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_desktop_textlabel_boxes, L"desktop_textlabel_boxes", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_desktop_manual_icon_scoot, L"desktop_manual_icon_scoot", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_desktop_555_fix, L"desktop_555_fix", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_dualhead_horz, L"dualhead_horz", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_dualhead_vert, L"dualhead_vert", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_save_cpu, L"save_cpu", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_skin, L"skin", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_fix_slow_text, L"fix_slow_text", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_vj_mode, L"vj_mode", m_szConfigIniFile, L"settings");

    WritePrivateProfileIntW(m_disp_mode_fs.Width, L"disp_mode_fs_w", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_disp_mode_fs.Height, L"disp_mode_fs_h", m_szConfigIniFile, L"settings");
    sprintf_s(str, "%d/%d", m_disp_mode_fs.RefreshRate.Numerator, m_disp_mode_fs.RefreshRate.Denominator);
    WritePrivateProfileStringA("settings", "disp_mode_fs_r", str, m_szConfigIniFileA);
    WritePrivateProfileIntW(m_disp_mode_fs.Format, L"disp_mode_fs_f", m_szConfigIniFile, L"settings");

    WritePrivateProfileIntW(INT_VERSION, L"version", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(INT_SUBVERSION, L"subversion", m_szConfigIniFile, L"settings");

    // Finally, save the plugin's unique settings.
    MilkDropWriteConfig();
#endif
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------

int CPluginShell::PluginRender(unsigned char* pWaveL, unsigned char* pWaveR) //, unsigned char *pSpecL, unsigned char *pSpecR)
{
    // Return `FALSE' here to tell Winamp to terminate the plugin.
    if (!m_lpDX || !m_lpDX->m_ready)
    {
        // note: 'm_ready' will go false when a device reset fatally fails
        //       (for example, when user resizes window, or toggles fullscreen.)
        m_exiting = 1;
        return false; // EXIT THE PLUGIN
    }

    /*
    if (m_hTextWnd)
        m_lost_focus = ((GetFocus() != GetPluginWindow()) && (GetFocus() != m_hTextWnd));
    else
        m_lost_focus = (GetFocus() != GetPluginWindow());

    if ((m_screenmode==WINDOWED   && m_hidden) ||
        (m_screenmode==FULLSCREEN && m_lost_focus) ||
        (m_screenmode==WINDOWED   && m_resizing)
       )
    {
        Sleep(30);
        return true;
    }*/

    /*// test for lost device
    // (this happens when device is fullscreen & user alt-tabs away,
    //  or when monitor power-saving kicks in)
    HRESULT hr = m_lpDX->m_lpDevice->TestCooperativeLevel();
    if (hr == D3DERR_DEVICENOTRESET)
    {
        // device WAS lost, and is now ready to be reset (and come back online):
        CleanUpDX9Stuff(0);
        if (m_lpDX->m_lpDevice->Reset(&m_lpDX->m_d3dpp) != D3D_OK)
        {
            // note: a basic warning messagebox will have already been given.
            // now suggest specific advice on how to regain more video memory:
    / *
            if (m_lpDX->m_lastErr == DXC_ERR_CREATEDEV_PROBABLY_OUTOFVIDEOMEMORY)
                SuggestHowToFreeSomeMem();* /
            return false;  // EXIT THE PLUGIN
        }
        if (!AllocateDX9Stuff())
            return false;  // EXIT THE PLUGIN
    }
    else if (hr != D3D_OK)
    {
        // device is lost, and not yet ready to come back; sleep.
        Sleep(30);
        return true;
    }
    */

    DoTime();
    AnalyzeNewSound(pWaveL, pWaveR);
    AlignWaves();

    DrawAndDisplay(0);

    EnforceMaxFPS();

    m_frame++;

    return true;
}

void CPluginShell::DrawAndDisplay(int redraw)
{
    int cx = m_lpDX->m_client_width;
    int cy = m_lpDX->m_client_height;
    m_upper_left_corner_y = TEXT_MARGIN + GetCanvasMarginY();
    m_upper_right_corner_y = TEXT_MARGIN + GetCanvasMarginY();
    m_lower_left_corner_y = cy - TEXT_MARGIN - GetCanvasMarginY();
    m_lower_right_corner_y = cy - TEXT_MARGIN - GetCanvasMarginY();
    m_left_edge = TEXT_MARGIN + GetCanvasMarginX();
    m_right_edge = cx - TEXT_MARGIN - GetCanvasMarginX();

    //m_lpDX->Clear();

    MilkDropRenderFrame(redraw);
    //RenderBuiltInTextMsgs();
    MilkDropRenderUI(&m_upper_left_corner_y, &m_upper_right_corner_y, &m_lower_left_corner_y, &m_lower_right_corner_y, m_left_edge, m_right_edge);
    //RenderPlaylist();

    m_lpDX->Show();
}

void CPluginShell::EnforceMaxFPS()
{
    int max_fps = 60;
    //switch (m_screenmode)
    //{
    //    case WINDOWED:        max_fps = m_max_fps_w;  break;
    //    case FULLSCREEN:      max_fps = m_max_fps_fs; break;
    //    case FAKE_FULLSCREEN: max_fps = m_max_fps_fs; break;
    //    case DESKTOP:         max_fps = m_max_fps_dm; break;
    //}

    if (max_fps <= 0)
        return;

    float fps_lo = (float)max_fps;
    float fps_hi = (float)max_fps;

    if (m_save_cpu)
    {
        // Find the optimal lo/hi bounds for the fps
        // that will result in a maximum difference,
        // in the time for a single frame, of 0.003 seconds -
        // the assumed granularity for Sleep(1) -

        // Using this range of acceptable fps
        // will allow us to do (sloppy) fps limiting
        // using only Sleep(1), and never the
        // second half of it: Sleep(0) in a tight loop,
        // which sucks up the CPU (whereas Sleep(1)
        // leaves it idle).

        // The original equation:
        //   1/(max_fps*t1) = 1/(max*fps/t1) - 0.003
        // where:
        //   t1 > 0
        //   max_fps*t1 is the upper range for fps
        //   max_fps/t1 is the lower range for fps

        float a = 1;
        float b = -0.003f * max_fps;
        float c = -1.0f;
        float det = b * b - 4 * a * c;
        if (det > 0)
        {
            float t1 = (-b + sqrtf(det)) / (2 * a);
            //float t2 = (-b - sqrtf(det)) / (2 * a);

            if (t1 > 1.0f)
            {
                fps_lo = max_fps / t1;
                fps_hi = max_fps * t1;
                // verify: now [1.0f/fps_lo - 1.0f/fps_hi] should equal 0.003 seconds.
                // note: allowing tolerance to go beyond these values for
                // fps_lo and fps_hi would gain nothing.
            }
        }
    }

    if (m_high_perf_timer_freq.QuadPart > 0)
    {
        LARGE_INTEGER t;
        QueryPerformanceCounter(&t);

        if (m_prev_end_of_frame.QuadPart != 0)
        {
            int ticks_to_wait_lo = (int)((float)m_high_perf_timer_freq.QuadPart / (float)fps_hi);
            int ticks_to_wait_hi = (int)((float)m_high_perf_timer_freq.QuadPart / (float)fps_lo);
            int done = 0;
            //int loops = 0;
            do
            {
                QueryPerformanceCounter(&t);

                __int64 t2 = t.QuadPart - m_prev_end_of_frame.QuadPart;
                if (t2 > 2147483000)
                    done = 1;
                if (t.QuadPart < m_prev_end_of_frame.QuadPart) // time wrap
                    done = 1;

                // This is sloppy - if your freq. is high, this can overflow (to a (-) int) in just a few minutes
                // but it's ok, we have protection for that above.
                int ticks_passed = (int)(t.QuadPart - m_prev_end_of_frame.QuadPart);
                if (ticks_passed >= ticks_to_wait_lo)
                    done = 1;

                if (!done)
                {
                    // if > 0.01s left, do Sleep(1), which will actually sleep some
                    //   steady amount of up to 3 ms (depending on the OS),
                    //   and do so in a nice way (cpu meter drops; laptop battery spared).
                    // otherwise, do a few Sleep(0)'s, which just give up the timeslice,
                    //   but don't really save cpu or battery, but do pass a tiny
                    //   amount of time.

                    //if (ticks_left > (int)m_high_perf_timer_freq.QuadPart / 500)
                    if (ticks_to_wait_hi - ticks_passed > (int)m_high_perf_timer_freq.QuadPart / 100)
                        Sleep(5);
                    else if (ticks_to_wait_hi - ticks_passed > (int)m_high_perf_timer_freq.QuadPart / 1000)
                        Sleep(1);
                    else
                        for (int i = 0; i < 10; i++)
                            Sleep(0); // causes thread to give up its timeslice
                }
            } while (!done);
        }

        m_prev_end_of_frame = t;
    }
    else
    {
        Sleep(1000 / max_fps);
    }
}

void CPluginShell::DoTime()
{
    if (m_frame == 0)
    {
        m_fps = 30;
        m_time = 0;
        m_time_hist_pos = 0;
    }

    double new_raw_time = 0.0;
    float elapsed = 0.0f;

    if (m_high_perf_timer_freq.QuadPart != 0)
    {
        // Get high-precision time.
        // Precision is usually from 1..6 us (MICROseconds), depending on the CPU speed
        // (higher CPU speeds tend to have better precision).
        // Note: On systems that run Windows XP or later, `QueryPerformanceCounter()`
        //        will always succeed and will thus never return zero.
        LARGE_INTEGER t;
        if (!QueryPerformanceCounter(&t))
        {
            m_high_perf_timer_freq.QuadPart = 0; // something went wrong (exception thrown) -> revert to crappy timer
        }
        else
        {
            new_raw_time = (double)t.QuadPart;
            elapsed = (float)((new_raw_time - m_last_raw_time) / (double)m_high_perf_timer_freq.QuadPart);
        }
    }

    if (m_high_perf_timer_freq.QuadPart == 0)
    {
        // Get low-precision time.
        // Precision is usually 1 ms (MILLIsecond) for Windows 98, and 10 ms for Windows 2000.
        new_raw_time = (double)(GetTickCount64() * 0.001);
        elapsed = (float)(new_raw_time - m_last_raw_time);
    }

    m_last_raw_time = new_raw_time;
    int slots_to_look_back = (m_high_perf_timer_freq.QuadPart == 0) ? TIME_HIST_SLOTS : TIME_HIST_SLOTS / 2;

    m_time += 1.0f / m_fps;

    // Timekeeping goals:
    //   1. Keep `m_time` increasing SMOOTHLY (smooth animation depends on it):
    //        m_time += 1.0f / m_fps; // where `m_fps` is a bit damped
    //   2. Keep m_time_hist[] 100% accurate (except for filtering out pauses),
    //      so that taking the difference between two entries, results in the
    //      real amount of time that passed between those 2 frames:
    //        m_time_hist[i] = m_last_raw_time + elapsed_corrected;
    if (m_frame > TIME_HIST_SLOTS)
    {
        if (m_fps < 60.0f)
            slots_to_look_back = (int)(slots_to_look_back * (0.1f + 0.9f * (m_fps / 60.0f)));

        if (elapsed > 5.0f / m_fps || elapsed > 1.0f || elapsed < 0)
            elapsed = 1.0f / 30.0f;

        float old_hist_time = m_time_hist[(m_time_hist_pos - slots_to_look_back + TIME_HIST_SLOTS) % TIME_HIST_SLOTS];
        float new_hist_time = m_time_hist[(m_time_hist_pos - 1 + TIME_HIST_SLOTS) % TIME_HIST_SLOTS] + elapsed;

        m_time_hist[m_time_hist_pos] = new_hist_time;
        m_time_hist_pos = (m_time_hist_pos + 1) % TIME_HIST_SLOTS;

        float new_fps = slots_to_look_back / (float)(new_hist_time - old_hist_time);
        float damping = (m_high_perf_timer_freq.QuadPart == 0) ? 0.93f : 0.87f;

        // Damp heavily, so that crappy timer precision doesn't make animation jerky.
        if (fabsf(m_fps - new_fps) > 3.0f)
            m_fps = new_fps;
        else
            m_fps = damping * m_fps + (1 - damping) * new_fps;
    }
    else
    {
        float damping = (m_high_perf_timer_freq.QuadPart == 0) ? 0.8f : 0.6f;

        if (m_frame < 2)
            elapsed = 1.0f / 30.0f;
        else if (elapsed > 1.0f || elapsed < 0)
            elapsed = 1.0f / m_fps;

        float old_hist_time = m_time_hist[0];
        float new_hist_time = m_time_hist[(m_time_hist_pos - 1 + TIME_HIST_SLOTS) % TIME_HIST_SLOTS] + elapsed;

        m_time_hist[m_time_hist_pos] = new_hist_time;
        m_time_hist_pos = (m_time_hist_pos + 1) % TIME_HIST_SLOTS;

        if (m_frame > 0)
        {
            float new_fps = (m_frame) / (new_hist_time - old_hist_time);
            m_fps = damping * m_fps + (1 - damping) * new_fps;
        }
    }

    // Synchronize the audio and video by telling Winamp how many milliseconds we want the audio data,
    // before it's actually audible.  If we set this to the amount of time it takes to display 1 frame
    // (1/fps), the video and audio should be perfectly synchronized.
    /*
    if (m_fps < 2.0f)
        mod1.latencyMs = 500;
    else if (m_fps > 125.0f)
        mod1.latencyMs = 8;
    else
        mod1.latencyMs = (int)(1000.0f/m_fps*m_lpDX->m_frame_delay + 0.5f);
     */
}

// We get 576 samples in from winamp.
// the output of the fft has 'num_frequencies' samples,
//   and represents the frequency range 0 hz - 22,050 hz.
// usually, plugins only use half of this output (the range 0 hz - 11,025 hz),
//   since >10 khz doesn't usually contribute much.
void CPluginShell::AnalyzeNewSound(unsigned char* pWaveL, unsigned char* pWaveR)
{
    float temp_wave[2][576];

    int old_i = 0;
    for (int i = 0; i < 576; i++)
    {
        m_sound.fWaveform[0][i] = (float)((pWaveL[i] ^ 128) - 128);
        m_sound.fWaveform[1][i] = (float)((pWaveR[i] ^ 128) - 128);

        // simulating single frequencies from 200 to 11,025 Hz:
        //float freq = 1.0f + 11050*(GetFrame() % 100)*0.01f;
        //m_sound.fWaveform[0][i] = 10*sinf(i*freq*6.28f/44100.0f);

        // damp the input into the FFT a bit, to reduce high-frequency noise:
        temp_wave[0][i] = 0.5f * (m_sound.fWaveform[0][i] + m_sound.fWaveform[0][old_i]);
        temp_wave[1][i] = 0.5f * (m_sound.fWaveform[1][i] + m_sound.fWaveform[1][old_i]);
        old_i = i;
    }

    m_fftobj.time_to_frequency_domain(temp_wave[0], m_sound.fSpectrum[0]);
    m_fftobj.time_to_frequency_domain(temp_wave[1], m_sound.fSpectrum[1]);

    // sum (left channel) spectrum up into 3 bands
    // [note: the new ranges do it so that the 3 bands are equally spaced, pitch-wise]
    float min_freq = 200.0f;
    float max_freq = 11025.0f;
    float net_octaves = (logf(max_freq / min_freq) / logf(2.0f)); // 5.7846348455575205777914165223593
    float octaves_per_band = net_octaves / 3.0f;                  // 1.9282116151858401925971388407864
    float mult = powf(2.0f, octaves_per_band); // each band's highest freq. divided by its lowest freq.; 3.805831305510122517035102576162
    // [to verify: min_freq * mult * mult * mult should equal max_freq.]
    for (int ch = 0; ch < 2; ch++)
    {
        for (int i = 0; i < 3; i++)
        {
            // Old guesswork code for this.
            //   float exp = 2.1f;
            //   int start = (int)(NUM_FREQUENCIES*0.5f*powf(i/3.0f, exp));
            //   int end   = (int)(NUM_FREQUENCIES*0.5f*powf((i+1)/3.0f, exp));
            // results:
            //          old range:      new range (ideal):
            //   bass:  0-1097          200-761
            //   mids:  1097-4705       761-2897
            //   treb:  4705-11025      2897-11025
            int start = (int)(NUM_FREQUENCIES * min_freq * powf(mult, (float)i) / 11025.0f);
            int end = (int)(NUM_FREQUENCIES * min_freq * powf(mult, (float)(i + 1)) / 11025.0f);
            if (start < 0) start = 0;
            if (end > NUM_FREQUENCIES) end = NUM_FREQUENCIES;

            m_sound.imm[ch][i] = 0;
            for (int j = start; j < end; j++)
                m_sound.imm[ch][i] += m_sound.fSpectrum[ch][j];
            m_sound.imm[ch][i] /= (float)(end - start);
        }
    }

    // Some code to find empirical long-term averages for `imm[0..2]`.
    /*
    {
        static float sum[3];
        static int count = 0;

        #define FRAMES_PER_SONG 300     // should be at least 200!

        if (m_frame < FRAMES_PER_SONG)
        {
            sum[0] = sum[1] = sum[2] = 0;
            count = 0;
        }
        else
        {
            if (m_frame%FRAMES_PER_SONG == 0)
            {
                char buf[256];
                sprintf_s(buf, "%.4f, %.4f, %.4f     (%d samples / ~%d songs)\n",
                    sum[0]/(float)(count),
                    sum[1]/(float)(count),
                    sum[2]/(float)(count),
                    count,
                    count/(FRAMES_PER_SONG-10)
                );
                OutputDebugString(buf);

                // skip to next song
                PostMessage(m_hWndWinamp,WM_COMMAND,40048,0);
            }
            else if (m_frame%FRAMES_PER_SONG == 5)
            {
                // then advance to 0-2 minutes into the song:
                PostMessage(m_hWndWinamp,WM_USER,(20 + (warand()%65) + (rand()%65))*1000,106);
            }
            else if (m_frame%FRAMES_PER_SONG >= 10)
            {
                sum[0] += m_sound.imm[0];
                sum[1] += m_sound.imm[1];
                sum[2] += m_sound.imm[2];
                count++;
            }
        }
    }
    */

    // multiply by long-term, empirically-determined inverse averages:
    // (for a trial of 244 songs, 10 seconds each, somewhere in the 2nd or 3rd minute,
    //  the average levels were: 0.326781557	0.38087377	0.199888934
    for (int ch = 0; ch < 2; ch++)
    {
        m_sound.imm[ch][0] /= 0.326781557f; //0.270f;
        m_sound.imm[ch][1] /= 0.380873770f; //0.343f;
        m_sound.imm[ch][2] /= 0.199888934f; //0.295f;
    }

    // do temporal blending to create attenuated and super-attenuated versions
    for (int ch = 0; ch < 2; ch++)
    {
        for (int i = 0; i < 3; i++)
        {
            // m_sound.avg[i]
            {
                float avg_mix;
                if (m_sound.imm[ch][i] > m_sound.avg[ch][i])
                    avg_mix = AdjustRateToFPS(0.2f, 14.0f, m_fps);
                else
                    avg_mix = AdjustRateToFPS(0.5f, 14.0f, m_fps);
                m_sound.avg[ch][i] = m_sound.avg[ch][i] * avg_mix + m_sound.imm[ch][i] * (1 - avg_mix);
            }

            // m_sound.med_avg[i]
            // m_sound.long_avg[i]
            {
                float med_mix = 0.91f;  //0.800f + 0.11f*powf(t, 0.4f); // primarily used for velocity_damping
                float long_mix = 0.96f; //0.800f + 0.16f*powf(t, 0.2f); // primarily used for smoke plumes
                med_mix = AdjustRateToFPS(med_mix, 14.0f, m_fps);
                long_mix = AdjustRateToFPS(long_mix, 14.0f, m_fps);
                m_sound.med_avg[ch][i] = m_sound.med_avg[ch][i] * (med_mix) + m_sound.imm[ch][i] * (1 - med_mix);
                m_sound.long_avg[ch][i] = m_sound.long_avg[ch][i] * (long_mix) + m_sound.imm[ch][i] * (1 - long_mix);
            }
        }
    }
}

// Parameter `pr` is the rectangle that some text will occupy;
// a black box will be drawn around it, plus a bit of extra margin space.
void CPluginShell::DrawDarkTranslucentBox(RECT* pr)
{
    m_lpDX->m_lpDevice->SetVertexShader(NULL, NULL);
    m_lpDX->m_lpDevice->SetPixelShader(NULL, NULL);
    //m_lpDX->m_lpDevice->SetFVF(SIMPLE_VERTEX_FORMAT); // TODO DX11
    m_lpDX->m_lpDevice->SetTexture(0, NULL);

    m_lpDX->m_lpDevice->SetBlendState(true, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA);
    //m_lpDX->m_lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    //m_lpDX->m_lpDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    //m_lpDX->m_lpDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

    m_lpDX->m_lpDevice->SetShader(2);
    //m_lpDX->m_lpDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    //m_lpDX->m_lpDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    //m_lpDX->m_lpDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    //m_lpDX->m_lpDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    //m_lpDX->m_lpDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);

    // Set up a quad.
    SIMPLEVERTEX verts[4];
    for (int i = 0; i < 4; i++)
    {
        verts[i].x = (i % 2 == 0) ? (float)(-m_lpDX->m_client_width / 2 + pr->left) : (float)(-m_lpDX->m_client_width / 2 + pr->right);
        verts[i].y = (i / 2 == 0) ? (float)-(-m_lpDX->m_client_height / 2 + pr->bottom) : (float)-(-m_lpDX->m_client_height / 2 + pr->top);
        verts[i].z = 0;
        verts[i].Diffuse = (m_screenmode == DESKTOP) ? 0xE0000000 : 0xD0000000;
    }

    m_lpDX->m_lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, verts, sizeof(SIMPLEVERTEX));

    // Undo unusual state changes.
    m_lpDX->m_lpDevice->SetDepth(true);
    m_lpDX->m_lpDevice->SetBlendState(false);
    //m_lpDX->m_lpDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    //m_lpDX->m_lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
}

// Aligns waves, using recursive (mipmap-style) least-error matching.
// Note: `NUM_WAVEFORM_SAMPLES` must be between 32 and 576.
void CPluginShell::AlignWaves()
{
    int align_offset[2] = {0, 0};

#if (NUM_WAVEFORM_SAMPLES < 576) // [don't let this code bloat our DLL size if it's not going to be used]
    int nSamples = NUM_WAVEFORM_SAMPLES;
    constexpr auto MAX_OCTAVES = 10;

    int octaves = (int)floorf(logf((float)(576 - nSamples)) / logf(2.0f));
    if (octaves < 4)
        return;
    if (octaves > MAX_OCTAVES)
        octaves = MAX_OCTAVES;

    for (int ch = 0; ch < 2; ch++)
    {
        // Only worry about matching the lower `nSamples` samples.
        float temp_new[MAX_OCTAVES][576];
        float temp_old[MAX_OCTAVES][576];
        static float temp_weight[MAX_OCTAVES][576];
        static int first_nonzero_weight[MAX_OCTAVES];
        static int last_nonzero_weight[MAX_OCTAVES];
        int spls[MAX_OCTAVES];
        int space[MAX_OCTAVES];

        memcpy_s(temp_new[0], sizeof(temp_new[0]), m_sound.fWaveform[ch], sizeof(float) * 576);
        memcpy_s(temp_old[0], sizeof(temp_old[0]), &m_oldwave[ch][m_prev_align_offset[ch]], sizeof(float) * nSamples);
        spls[0] = 576;
        space[0] = 576 - nSamples;

        // Potential optimization: Could reuse (instead of recompute) MIP levels for `m_oldwave[2][]`?
        for (int octave = 1; octave < octaves; octave++)
        {
            spls[octave] = spls[octave - 1] / 2;
            space[octave] = space[octave - 1] / 2;
            for (int n = 0; n < spls[octave]; n++)
            {
                temp_new[octave][n] = 0.5f * (temp_new[octave - 1][n * 2] + temp_new[octave - 1][n * 2 + 1]);
                temp_old[octave][n] = 0.5f * (temp_old[octave - 1][n * 2] + temp_old[octave - 1][n * 2 + 1]);
            }
        }

        if (!m_align_weights_ready)
        {
            m_align_weights_ready = 1;
            for (int octave = 0; octave < octaves; octave++)
            {
                int compare_samples = spls[octave] - space[octave];
                for (int n = 0; n < compare_samples; n++)
                {
                    // Start with pyramid-shaped PDF, from 0..1..0.
                    if (n < compare_samples / 2)
                        temp_weight[octave][n] = n * 2 / (float)compare_samples;
                    else
                        temp_weight[octave][n] = (compare_samples - 1 - n) * 2 / (float)compare_samples;

                    // Tweak how much the center matters vs. the edges.
                    temp_weight[octave][n] = (temp_weight[octave][n] - 0.8f) * 5.0f + 0.8f;

                    // Clip.
                    if (temp_weight[octave][n] > 1) temp_weight[octave][n] = 1;
                    if (temp_weight[octave][n] < 0) temp_weight[octave][n] = 0;
                }

                int p = 0;
                while (temp_weight[octave][p] == 0 && p < compare_samples)
                    p++;
                first_nonzero_weight[octave] = p;

                p = compare_samples - 1;
                while (temp_weight[octave][p] == 0 && p >= 0)
                    p--;
                last_nonzero_weight[octave] = p;
            }
        }

        int n1 = 0;
        int n2 = space[octaves - 1];
        for (int octave = octaves - 1; octave >= 0; octave--)
        {
            // For example:
            //  space[octave] == 4
            //  spls[octave] == 36
            //  (so we test 32 samples, w/4 offsets)
            //int compare_samples = spls[octave] - space[octave];

            int lowest_err_offset = -1;
            float lowest_err_amount = 0;
            for (int n = n1; n < n2; n++)
            {
                float err_sum = 0;
                //for (int i=0; i<compare_samples; i++)
                for (int i = first_nonzero_weight[octave]; i <= last_nonzero_weight[octave]; i++)
                {
                    float x = (temp_new[octave][i + n] - temp_old[octave][i]) * temp_weight[octave][i];
                    if (x > 0)
                        err_sum += x;
                    else
                        err_sum -= x;
                }

                if (lowest_err_offset == -1 || err_sum < lowest_err_amount)
                {
                    lowest_err_offset = n;
                    lowest_err_amount = err_sum;
                }
            }

            // Now use `lowest_err_offset` to guide bounds of search in next octave:
            //  space[octave] == 8
            //  spls[octave] == 72
            //     -say `lowest_err_offset` was 2
            //     -that corresponds to samples 4 & 5 of the next octave
            //     -also, expand about this by 2 samples?  YES.
            //  (so we'd test 64 samples, w/8->4 offsets)
            if (octave > 0)
            {
                n1 = lowest_err_offset * 2 - 1;
                n2 = lowest_err_offset * 2 + 2 + 1;
                if (n1 < 0) n1 = 0;
                if (n2 > space[octave - 1]) n2 = space[octave - 1];
            }
            else
                align_offset[ch] = lowest_err_offset;
        }
    }
#endif
    memcpy_s(m_oldwave[0], sizeof(m_oldwave[0]), m_sound.fWaveform[0], sizeof(float) * 576);
    memcpy_s(m_oldwave[1], sizeof(m_oldwave[1]), m_sound.fWaveform[1], sizeof(float) * 576);
    m_prev_align_offset[0] = align_offset[0];
    m_prev_align_offset[1] = align_offset[1];

    // finally, apply the results: modify m_sound.fWaveform[2][0..576]
    // by scooting the aligned samples so that they start at m_sound.fWaveform[2][0].
    for (int ch = 0; ch < 2; ch++)
        if (align_offset[ch] > 0)
        {
            for (int i = 0; i < nSamples; i++)
                m_sound.fWaveform[ch][i] = m_sound.fWaveform[ch][i + align_offset[ch]];
            // zero the rest out, so it's visually evident that these samples are now bogus:
            memset(&m_sound.fWaveform[ch][nSamples], 0, (576 - nSamples) * sizeof(float));
        }
}