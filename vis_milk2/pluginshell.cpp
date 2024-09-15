/*
  LICENSE
  -------
  Copyright 2005-2013 Nullsoft, Inc.
  Copyright 2021-2024 Jimmy Cassis
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
#include "pluginshell.h"

#include "defines.h"
#include "shell_defines.h"
#include "support.h"
#include "utility.h"
#define WASABI_API_ORIG_HINST GetInstance()
#include "api.h"
#include <nu/AutoWide.h>
#include <winamp/wa_ipc.h>

// STATE VALUES AND VERTEX FORMATS FOR HELP SCREEN TEXTURE:
//#define TEXT_SURFACE_NOT_READY 0
//#define TEXT_SURFACE_REQUESTED 1
//#define TEXT_SURFACE_READY     2
//#define TEXT_SURFACE_ERROR     3

extern wchar_t* g_szHelp;
//extern winampVisModule mod1;

CPluginShell::CPluginShell() { /* This should remain empty! */ }
CPluginShell::~CPluginShell() { /* This should remain empty! */ }

eScrMode CPluginShell::GetScreenMode() const { return m_screenmode; }
uint32_t CPluginShell::GetFrame() const { return m_frame; }
float CPluginShell::GetTime() const { return m_time; }
float CPluginShell::GetFps() const { return m_fps; }
HWND CPluginShell::GetPluginWindow() const { if (m_lpDX) return m_lpDX->GetHwnd(); else return NULL; }
int CPluginShell::GetWidth() const { if (m_lpDX) return m_lpDX->m_client_width; else return 0; }
int CPluginShell::GetHeight() const { if (m_lpDX) return m_lpDX->m_client_height; else return 0; }
int CPluginShell::GetCanvasMarginX() { return 0; }
int CPluginShell::GetCanvasMarginY() { return 0; }
HWND CPluginShell::GetWinampWindow() const { return m_hWndWinamp; }
void CPluginShell::SetWinampWindow(HWND window) { m_hWndWinamp = window; }
HINSTANCE CPluginShell::GetInstance() const { return m_hInstance; }
wchar_t* CPluginShell::GetPluginsDirPath() { return m_szPluginsDirPath; }
wchar_t* CPluginShell::GetConfigIniFile() { return m_szConfigIniFile; }
TextStyle* CPluginShell::GetFont(eFontIndex idx) { if (idx >= eFontIndex::SIMPLE_FONT && idx < eFontIndex::EXTRA_5 && idx < NUM_BASIC_FONTS + NUM_EXTRA_FONTS) { return m_dwrite_font[idx].get(); } else { return NULL; } }
int CPluginShell::GetFontHeight(eFontIndex idx) const { if (idx >= eFontIndex::SIMPLE_FONT  && idx < eFontIndex::EXTRA_5 && idx < NUM_BASIC_FONTS + NUM_EXTRA_FONTS) return m_fontinfo[idx].nSize; else return 0; }
int CPluginShell::GetBitDepth() const { return m_lpDX->GetBitDepth(); };
D3D11Shim* CPluginShell::GetDevice() const { return m_lpDX->m_lpDevice.get(); };

int CPluginShell::InitNonDX11()
{
    return AllocateMilkDropNonDX11();
}

void CPluginShell::CleanUpNonDX11()
{
    CleanUpMilkDropNonDX11();
}

int CPluginShell::AllocateFonts()
{
    // Create system fonts.
    for (int i = 0; i < NUM_BASIC_FONTS + NUM_EXTRA_FONTS; i++)
    {
        m_dwrite_font[i] = std::make_unique<TextStyle>(
            m_fontinfo[i].szFace,
            static_cast<float>(m_fontinfo[i].nSize), //* 4 / 10),
            m_fontinfo[i].bBold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_REGULAR,
            m_fontinfo[i].bItalic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
            DWRITE_TEXT_ALIGNMENT_LEADING
        );
    }

#if 0
    // Get actual font heights.
    for (int i = 0; i < NUM_BASIC_FONTS + NUM_EXTRA_FONTS; i++)
    {
        D2D1_RECT_F r{0.0f, 0.0f, static_cast<FLOAT>(1024), static_cast<FLOAT>(1024)};
        DWORD textColor = 0xFFFFFFFF;
        D2D1_COLOR_F fTextColor = D2D1::ColorF(textColor, static_cast<FLOAT>(((textColor & 0xFF000000) >> 24) / 255.0f));
        TextElement m_d2d_font;
        m_d2d_font.Initialize(m_lpDX->GetD2DDeviceContext());
        m_d2d_font.SetAlignment(AlignNear, AlignNear);
        m_d2d_font.SetTextColor(fTextColor);
        m_d2d_font.SetTextOpacity(fTextColor.a);
        m_d2d_font.SetTextShadow(false);
        m_d2d_font.SetText(L"Mg");
        m_d2d_font.SetContainer(r);
        m_d2d_font.SetTextStyle(m_dwrite_font[i].get());
        r = m_d2d_font.GetBounds(m_lpDX->GetDWriteFactory());
        int h = static_cast<int>(std::ceil(r.bottom - r.top));
        if (h > 0)
            m_fontinfo[i].nSize = h;
    }
#endif

    return true;
}

void CPluginShell::CleanUpFonts()
{
    //for (int i = 0; i < NUM_BASIC_FONTS + NUM_EXTRA_FONTS; i++)
    //    SafeDelete(m_dwrite_font[i]);
}

void CPluginShell::AllocateTextSurface()
{
    ID3D11Device* pDevice = m_lpDX->GetD3DDevice();
    int w = GetWidth();
    int h = GetHeight();

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = static_cast<UINT>(w);
    desc.Height = static_cast<UINT>(h);
    desc.MipLevels = 0;
    desc.ArraySize = 1;
    desc.Format = m_lpDX->GetBackBufferFormat();
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    //const uint32_t s_pixel = 0xFFFFFFFF;
    //D3D11_SUBRESOURCE_DATA initData = {&s_pixel, sizeof(uint32_t), 0};

    if (FAILED(pDevice->CreateTexture2D(&desc, NULL, &m_lpDDSText)))
    {
        m_lpDDSText.Reset(); // OK if there is not enough memory for it!
    }
    else
    {
        // If `m_lpDDSText` does not cover enough of screen, cancel it.
        desc = {};
        m_lpDDSText->GetDesc(&desc);
        if ((desc.Width < 256 && w >= 256) ||
            (desc.Height < 256 && h >= 256) ||
            (static_cast<float>(desc.Width / w) < 0.74f) ||
            (desc.Height / (float)h < 0.74f))
        {
            m_lpDDSText.Reset();
        }
    }
}

int CPluginShell::AllocateDX11()
{
    AllocateFonts();
    if (m_fix_slow_text)
        AllocateTextSurface();
    int ret = AllocateMilkDropDX11();

    // Invalidate various 'caches' here.
    m_playlist_top_idx = -1; // Invalidating playlist cache forces recompute of playlist width

    if (m_lpDX)
    {
        //m_text.Finish();
        m_text.Init(m_lpDX.get()
#ifndef _FOOBAR
                    , nullptr, 1
#endif
        );
    }

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

    CleanUpFonts();
    m_text.Finish();
    m_lpDDSText.Reset();
    CleanUpMilkDropDX11(final_cleanup);
}

void CPluginShell::OnWindowSizeChanged(int width, int height)
{
    // Update window properties
    if (!m_lpDX->OnWindowSizeChanged(width, height))
    {
        return;
    }
    if (!AllocateDX11())
    {
        m_lpDX->m_ready = false; // flag to exit
        return;
    }
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
            pParams->adapter_guid = m_adapter_guid_w;
            pParams->msaa = m_multisample_w;
            wcscpy_s(pParams->adapter_devicename, m_adapter_devicename_w);
            break;
        case FULLSCREEN:
            pParams->allow_page_tearing = m_allow_page_tearing_fs;
            pParams->adapter_guid = m_adapter_guid_fs;
            pParams->msaa = m_multisample_fs;
            wcscpy_s(pParams->adapter_devicename, m_adapter_devicename_fs);
            break;
    }
    pParams->parent_window = m_hWndWinamp;
}

void CPluginShell::ToggleFullScreen()
{
    CleanUpDX11(0);

    switch (m_screenmode)
    {
        case WINDOWED: m_screenmode = FULLSCREEN; break;
        case FULLSCREEN: m_screenmode = WINDOWED; break;
    }

    DXCONTEXT_PARAMS params{};
    StuffParams(&params);

    if (!m_lpDX->StartOrRestartDevice(&params))
    {
        // Note: A basic warning message box will have already been given.
        if (m_lpDX->m_lastErr == DX_ERR_CREATEDEV_PROBABLY_OUTOFVIDEOMEMORY)
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

void CPluginShell::ToggleHelp()
{
    m_show_help = !m_show_help;
    //int ret = CheckMenuItem(m_context_menu, ID_SHOWHELP, MF_BYCOMMAND | (m_show_help ? MF_CHECKED : MF_UNCHECKED));
}

void CPluginShell::TogglePlaylist()
{
    m_show_playlist = !m_show_playlist;
    m_playlist_top_idx = -1; // <- invalidates playlist cache
    //int ret = CheckMenuItem(m_context_menu, ID_SHOWPLAYLIST, MF_BYCOMMAND | (m_show_playlist ? MF_CHECKED : MF_UNCHECKED));
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

    m_lpDX->GetDeviceResources()->RegisterDeviceNotify(this);

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
    m_max_fps_fs = 30;
    m_max_fps_w = 30;
    m_show_press_f1_msg = 1;
    m_allow_page_tearing_w = 0;
    m_allow_page_tearing_fs = 0;
    m_minimize_winamp = 1;
    m_dualhead_horz = 2;
    m_dualhead_vert = 1;
    m_save_cpu = 1;
    m_skin = 1;
    m_fix_slow_text = 0;
    m_enable_hdr = 0;
    m_enable_downmix = 0;
    m_show_album = 0;
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
    m_lpDDSText.Reset();
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

    wchar_t* p;
    if (hWinampWnd && (p = reinterpret_cast<wchar_t*>(SendMessage(hWinampWnd, WM_WA_IPC, 0, IPC_GETPLUGINDIRECTORYW))) != NULL && p != (wchar_t*)1)
    {
#ifndef _FOOBAR
        swprintf_s(m_szPluginsDirPath, L"%ls\\", p);
#else
        swprintf_s(m_szPluginsDirPath, L"%ls", p);
#endif
    }
    else
    {
        GetModuleFileName(m_hInstance, m_szPluginsDirPath, MAX_PATH);
        p = m_szPluginsDirPath + wcsnlen_s(m_szPluginsDirPath, MAX_PATH);
        while (p >= m_szPluginsDirPath && *p != L'\\')
            p--;
        if (++p >= m_szPluginsDirPath)
            *p = 0;
    }

    // Get path to INI file and read in preferences/settings right away.
    if (hWinampWnd && (p = reinterpret_cast<wchar_t*>(SendMessage(hWinampWnd, WM_WA_IPC, 0, IPC_GETINIDIRECTORYW))) != NULL && p != (wchar_t*)1)
    {
        wchar_t szConfigDir[MAX_PATH] = {0};
#ifndef _FOOBAR
        swprintf_s(szConfigDir, L"%ls\\Plugins\\%ls", p, SUBDIR);
#else
        swprintf_s(szConfigDir, L"%ls", p);
#endif
        if (GetFileAttributes(szConfigDir) == INVALID_FILE_ATTRIBUTES)
            if (CreateDirectory(szConfigDir, NULL) == 0)
                ConsoleMessage(TEXT("CreateDirectory"), IDS_MILK2_DIR_WARN, IDS_MILKDROP_WARNING);

        swprintf_s(m_szConfigIniFile, L"%ls%ls", szConfigDir, INIFILE);
    }
    else
    {
        swprintf_s(m_szConfigIniFile, L"%ls%ls", m_szPluginsDirPath, INIFILE);
    }

    // PRIVATE CONFIG PANEL SETTINGS
    m_multisample_fs = {1u, 0u}; //D3DMULTISAMPLE_NONE;
    m_multisample_w = {1u, 0u}; //D3DMULTISAMPLE_NONE;
    ZeroMemory(&m_adapter_guid_fs, sizeof(GUID));
    ZeroMemory(&m_adapter_guid_w, sizeof(GUID));
    m_adapter_devicename_w[0] = L'\0';
    m_adapter_devicename_fs[0] = L'\0';

    // PRIVATE RUNTIME SETTINGS
    m_lost_focus = 0;
    m_hidden = 0;
    m_resizing = 0;
    m_show_help = false;
    m_show_playlist = false;
    m_playlist_pos = 0;
    m_playlist_pageups = 0;
    m_playlist_top_idx = -1; // `m_playlist_width_pixels` and `m_playlist[256][256]` will be considered invalid whenever `m_playlist_top_idx` is -1.
    m_playlist_btm_idx = -1;
    m_exiting = 0;
    m_upper_left_corner_y = 0;
    m_lower_left_corner_y = 0;
    m_upper_right_corner_y = 0;
    m_lower_right_corner_y = 0;
    m_left_edge = 0;
    m_right_edge = 0;
    m_force_accept_WM_WINDOWPOSCHANGING = 0;

    /*
    // PRIVATE - GDI STUFF
    m_main_menu = NULL;
    m_context_menu = NULL;
    for (int i = 0; i < NUM_BASIC_FONTS + NUM_EXTRA_FONTS; i++)
        m_font[i] = NULL;
    m_font_desktop = NULL;
    */

    // PRIVATE TIMEKEEPING
    m_last_raw_time = 0;
    memset(m_time_hist, 0, sizeof(m_time_hist));
    m_time_hist_pos = 0;
    if (!QueryPerformanceFrequency(&m_high_perf_timer_freq))
    {
        throw std::exception(); //m_high_perf_timer_freq.QuadPart = 0;
    }
    m_prev_end_of_frame.QuadPart = 0;

    // PRIVATE AUDIO PROCESSING DATA
    memset(m_oldwave[0], 0, sizeof(float) * NUM_AUDIO_BUFFER_SAMPLES);
    memset(m_oldwave[1], 0, sizeof(float) * NUM_AUDIO_BUFFER_SAMPLES);
    m_prev_align_offset[0] = 0;
    m_prev_align_offset[1] = 0;
    m_align_weights_ready = 0;

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

void CPluginShell::ReadFont(const int n)
{
#ifndef _FOOBAR
    GetPrivateProfileString(L"settings", BuildSettingName(L"szFontFace", n), m_fontinfo[n].szFace, m_fontinfo[n].szFace, ARRAYSIZE(m_fontinfo[n].szFace), m_szConfigIniFile);
    m_fontinfo[n].nSize = GetPrivateProfileInt(L"settings", BuildSettingName(L"nFontSize", n), m_fontinfo[n].nSize, m_szConfigIniFile);
    m_fontinfo[n].bBold = GetPrivateProfileInt(L"settings", BuildSettingName(L"bFontBold", n), m_fontinfo[n].bBold, m_szConfigIniFile);
    m_fontinfo[n].bItalic = GetPrivateProfileInt(L"settings", BuildSettingName(L"bFontItalic", n), m_fontinfo[n].bItalic, m_szConfigIniFile);
    m_fontinfo[n].bAntiAliased = GetPrivateProfileInt(L"settings", BuildSettingName(L"bFontAA", n), m_fontinfo[n].bItalic, m_szConfigIniFile);
#else
    UNREFERENCED_PARAMETER(n);
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

    m_multisample_fs = {static_cast<UINT>(GetPrivateProfileInt(L"settings", L"multisample_fullscreen", m_multisample_fs.Count, m_szConfigIniFile)), 0u};
    m_multisample_d = {static_cast<UINT>(GetPrivateProfileInt(L"settings", L"multisample_desktop", m_multisample_d.Count, m_szConfigIniFile)), 0u};
    m_multisample_w = {static_cast<UINT>(GetPrivateProfileInt(L"settings", L"multisample_windowed", m_multisample_w.Count, m_szConfigIniFile)), 0u};

    char str[256];
    GetPrivateProfileStringA("settings", "adapter_guid_fullscreen", "", str, sizeof(str), m_szConfigIniFileA);
    TextToLuidA(str, &m_adapter_guid_fs);
    GetPrivateProfileStringA("settings", "adapter_guid_desktop", "", str, sizeof(str), m_szConfigIniFileA);
    TextToLuidA(str, &m_adapter_guid_d);
    GetPrivateProfileStringA("settings", "adapter_guid_windowed", "", str, sizeof(str), m_szConfigIniFileA);
    TextToLuidA(str, &m_adapter_guid_w);
    GetPrivateProfileString(L"settings", L"adapter_devicename_fullscreen", L"", m_adapter_devicename_fs, ARRAYSIZE(m_adapter_devicename_fs), m_szConfigIniFile);
    GetPrivateProfileString(L"settings", L"adapter_devicename_desktop", L"", m_adapter_devicename_d, ARRAYSIZE(m_adapter_devicename_d), m_szConfigIniFile);
    GetPrivateProfileString(L"settings", L"adapter_devicename_windowed", L"", m_adapter_devicename_w, ARRAYSIZE(m_adapter_devicename_w), m_szConfigIniFile);

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
    m_max_fps_d = GetPrivateProfileInt(L"settings", L"max_fps_dm", m_max_fps_d, m_szConfigIniFile);
    m_max_fps_w = GetPrivateProfileInt(L"settings", L"max_fps_w", m_max_fps_w, m_szConfigIniFile);
    m_show_press_f1_msg = GetPrivateProfileInt(L"settings", L"show_press_f1_msg", m_show_press_f1_msg, m_szConfigIniFile);
    m_allow_page_tearing_w = GetPrivateProfileInt(L"settings", L"allow_page_tearing_w", m_allow_page_tearing_w, m_szConfigIniFile);
    m_allow_page_tearing_fs = GetPrivateProfileInt(L"settings", L"allow_page_tearing_fs", m_allow_page_tearing_fs, m_szConfigIniFile);
    m_allow_page_tearing_d = GetPrivateProfileInt(L"settings", L"allow_page_tearing_dm", m_allow_page_tearing_d, m_szConfigIniFile);
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
    WritePrivateProfileIntW(m_multisample_fs.Count, L"multisample_fullscreen", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_multisample_d.Count, L"multisample_desktop", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_multisample_windowed.Count, L"multisample_windowed", m_szConfigIniFile, L"settings");

    char str[256];
    LuidToTextA(&m_adapter_guid_fs, str, sizeof(str));
    WritePrivateProfileStringA("settings", "adapter_guid_fullscreen", str, m_szConfigIniFileA);
    LuidToTextA(&m_adapter_guid_d, str, sizeof(str));
    WritePrivateProfileStringA("settings", "adapter_guid_desktop", str, m_szConfigIniFileA);
    LuidToTextA(&m_adapter_guid_w, str, sizeof(str));
    WritePrivateProfileStringA("settings", "adapter_guid_windowed", str, m_szConfigIniFileA);
    WritePrivateProfileString(L"settings", L"adapter_devicename_fullscreen", m_adapter_devicename_fs, m_szConfigIniFile);
    WritePrivateProfileString(L"settings", L"adapter_devicename_desktop", m_adapter_devicename_d, m_szConfigIniFile);
    WritePrivateProfileString(L"settings", L"adapter_devicename_windowed", m_adapter_devicename_w, m_szConfigIniFile);

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
    WritePrivateProfileIntW(m_max_fps_d, L"max_fps_dm", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_max_fps_w, L"max_fps_w", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_show_press_f1_msg, L"show_press_f1_msg", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_allow_page_tearing_w, L"allow_page_tearing_w", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_allow_page_tearing_fs, L"allow_page_tearing_fs", m_szConfigIniFile, L"settings");
    WritePrivateProfileIntW(m_allow_page_tearing_d, L"allow_page_tearing_dm", m_szConfigIniFile, L"settings");
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
#ifndef _FOOBAR
int CPluginShell::PluginRender(unsigned char* pWaveL, unsigned char* pWaveR) //, unsigned char *pSpecL, unsigned char *pSpecR)
#else
int CPluginShell::PluginRender(float* pWaveL, float* pWaveR)
#endif
{
    // Return `FALSE' here to tell Winamp to terminate the plugin.
    if (!m_lpDX || !m_lpDX->m_ready)
    {
        // Note: 'm_ready' will go false when a device reset fatally fails
        //       (for example, when user resizes window, or toggles fullscreen).
        m_exiting = 1;
        return false; // EXIT THE PLUGIN
    }

    /*
    if (m_hTextWnd)
        m_lost_focus = ((GetFocus() != GetPluginWindow()) && (GetFocus() != m_hTextWnd));
    else
        m_lost_focus = (GetFocus() != GetPluginWindow());

    if ((m_screenmode == WINDOWED   && m_hidden) ||
        (m_screenmode == FULLSCREEN && m_lost_focus) ||
        (m_screenmode == WINDOWED   && m_resizing))
    {
        Sleep(30);
        return true;
    }

    // Test for lost device.
    // (this happens when device is fullscreen & user alt-tabs away,
    //  or when monitor power-saving kicks in)
    HRESULT hr = m_lpDX->m_lpDevice->TestCooperativeLevel();
    if (hr == D3DERR_DEVICENOTRESET)
    {
        // Device WAS lost, and is now ready to be reset (and come back online).
        CleanUpDX11Stuff(0);
        if (m_lpDX->m_lpDevice->Reset(&m_lpDX->m_d3dpp) != D3D_OK)
        {
            // Note: a basic warning message box will have already been given.
            // now suggest specific advice on how to regain more video memory:
            //if (m_lpDX->m_lastErr == DXC_ERR_CREATEDEV_PROBABLY_OUTOFVIDEOMEMORY)
            //    SuggestHowToFreeSomeMem();
            return false; // EXIT THE PLUGIN
        }
        if (!AllocateDX11Stuff())
            return false; // EXIT THE PLUGIN
    }
    else if (hr != D3D_OK)
    {
        // Device is lost, and not yet ready to come back; sleep.
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
    RenderBuiltInTextMsgs();
    MilkDropRenderUI(&m_upper_left_corner_y, &m_upper_right_corner_y, &m_lower_left_corner_y, &m_lower_right_corner_y, m_left_edge, m_right_edge);
    RenderPlaylist();
    m_text.DrawNow();
    m_lpDX->Show();
}

void CPluginShell::EnforceMaxFPS()
{
    int max_fps = 0;
    switch (m_screenmode)
    {
        case WINDOWED:   max_fps = m_max_fps_w;  break;
        case FULLSCREEN: max_fps = m_max_fps_fs; break;
    }

    if (max_fps <= 0)
        return;

    float fps_lo = (float)max_fps;
    float fps_hi = (float)max_fps;

    // Find the optimal lo/hi bounds for the fps
    // that will result in a maximum difference,
    // in the time for a single frame, of 0.003 seconds -
    // the assumed granularity for Sleep(1) -
    //
    // Using this range of acceptable fps
    // will allow us to do (sloppy) fps limiting
    // using only Sleep(1), and never the
    // second half of it: Sleep(0) in a tight loop,
    // which sucks up the CPU (whereas Sleep(1)
    // leaves it idle).
    //
    // The original equation:
    //   1 / (max_fps * t1) = 1 / (max_fps / t1) - 0.003
    // where:
    //   t1 > 0
    //   max_fps * t1 is the upper range for fps
    //   max_fps / t1 is the lower range for fps
    if (m_save_cpu)
    {
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
                // Verify: Now [1.0f / fps_lo - 1.0f / fps_hi] should equal 0.003 seconds.
                // Note: Allowing tolerance to go beyond these values for `fps_lo` and `fps_hi` would gain nothing.
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
        // Precision is usually from 1..6 us, depending on the CPU speed
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
        // Precision is usually 1 ms for Windows 98 and 10 ms for Windows 2000.
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
    // before it's actually audible. If we set this to the amount of time it takes to display 1 frame
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

// Receives 576 samples per channel from the audio source.
// The output of the FFT has `num_frequencies` samples,
// and represents the frequency range 0 Hz - 22,050 Hz.
// Usually, plugins only use half of this output (the range 0 Hz - 11,025 Hz),
// since frequencies above 10 kHz do not usually contribute much.
#ifndef _FOOBAR
void CPluginShell::AnalyzeNewSound(unsigned char* pWaveL, unsigned char* pWaveR)
#else
void CPluginShell::AnalyzeNewSound(float* pWaveL, float* pWaveR)
#endif
{
    std::array<std::vector<float>, 2> temp_wave;
    temp_wave[0].resize(NUM_AUDIO_BUFFER_SAMPLES);
    temp_wave[1].resize(NUM_AUDIO_BUFFER_SAMPLES);

    int old_i = 0;
    for (int i = 0; i < NUM_AUDIO_BUFFER_SAMPLES; i++)
    {
#ifndef _FOOBAR
        m_sound.fWaveform[0][i] = (float)((pWaveL[i] ^ 128) - 128);
        m_sound.fWaveform[1][i] = (float)((pWaveR[i] ^ 128) - 128);
#else
        m_sound.fWaveform[0][i] = pWaveL[i];
        m_sound.fWaveform[1][i] = pWaveR[i];
#endif
        // Simulating single frequencies from 200 to 11,025 Hz.
        //float freq = 1.0f + 11050 * (GetFrame() % 100) * 0.01f;
        //m_sound.fWaveform[0][i] = 10 * sinf(i * freq * 6.28f / 44100.0f);

        // Dampen the input into the FFT slightly, to reduce high-frequency noise.
        temp_wave[0][i] = 0.5f * (m_sound.fWaveform[0][i] + m_sound.fWaveform[0][old_i]);
        temp_wave[1][i] = 0.5f * (m_sound.fWaveform[1][i] + m_sound.fWaveform[1][old_i]);
        old_i = i;
    }

    m_fftobj.TimeToFrequencyDomain(temp_wave[0], m_sound.fSpectrum[0]);
    m_fftobj.TimeToFrequencyDomain(temp_wave[1], m_sound.fSpectrum[1]);

    // Sum (left channel) spectrum up into 3 bands.
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
            int start = static_cast<int>(NUM_FREQUENCIES * min_freq * powf(mult, (float)i) / 11025.0f);
            int end = static_cast<int>(NUM_FREQUENCIES * min_freq * powf(mult, (float)(i + 1)) / 11025.0f);
            if (start < 0) start = 0;
            if (end > NUM_FREQUENCIES) end = NUM_FREQUENCIES;

            m_sound.imm[ch][i] = 0;
            for (int j = start; j < end; j++)
                m_sound.imm[ch][i] += m_sound.fSpectrum[ch][j];
            m_sound.imm[ch][i] /= static_cast<float>(end - start);
        }
    }

    /*
    // Finds empirical long-term averages for `imm[0..2]`.
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
                    sum[0] / static_cast<float>(count),
                    sum[1] / static_cast<float>(count),
                    sum[2] / static_cast<float>(count),
                    count,
                    count / (FRAMES_PER_SONG - 10)
                );
                OutputDebugString(buf);

                // Skip to next song.
                PostMessage(m_hWndWinamp,WM_COMMAND,40048,0);
            }
            else if (m_frame%FRAMES_PER_SONG == 5)
            {
                // Then advance to 0-2 minutes into the song.
                PostMessage(m_hWndWinamp, WM_USER, (20 + (warand() % 65) + (rand() % 65)) * 1000, 106);
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

    // Multiply by long-term, empirically-determined inverse averages.
    // For a trial of 244 songs, 10 seconds each, somewhere in the 2nd or 3rd minute,
    // the average levels were: 0.326781557  0.38087377  0.199888934
    for (int ch = 0; ch < 2; ch++)
    {
        m_sound.imm[ch][0] /= 0.326781557f; //0.270f;
        m_sound.imm[ch][1] /= 0.380873770f; //0.343f;
        m_sound.imm[ch][2] /= 0.199888934f; //0.295f;
    }

    // Do temporal blending to create attenuated and super-attenuated versions.
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
                float med_mix = 0.91f;  //0.800f + 0.11f * powf(t, 0.4f); // primarily used for velocity_damping
                float long_mix = 0.96f; //0.800f + 0.16f * powf(t, 0.2f); // primarily used for smoke plumes
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
void CPluginShell::DrawDarkTranslucentBox(D2D1_RECT_F* pr)
{
    m_lpDX->m_lpDevice->SetVertexShader(NULL, NULL);
    m_lpDX->m_lpDevice->SetPixelShader(NULL, NULL);
    //m_lpDX->m_lpDevice->SetFVF(SIMPLE_VERTEX_FORMAT); // TODO: DirectX11
    m_lpDX->m_lpDevice->SetTexture(0, NULL);
    m_lpDX->m_lpDevice->SetBlendState(true, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA);
    m_lpDX->m_lpDevice->SetShader(2);

    // Set up a quad.
    MDVERTEX verts[4]{};
    for (int i = 0; i < 4; i++)
    {
        verts[i].x = (i % 2 == 0) ? static_cast<float>(-m_lpDX->m_client_width / 2 + pr->left) : static_cast<float>(-m_lpDX->m_client_width / 2 + pr->right);
        verts[i].y = (i / 2 == 0) ? static_cast<float>(-(-m_lpDX->m_client_height / 2 + pr->bottom)) : static_cast<float>(-(-m_lpDX->m_client_height / 2 + pr->top));
        verts[i].z = 0.0f;
        verts[i].a = 0xD0 / 255.0f; verts[i].r = 0.0f; verts[i].g = 0.0f; verts[i].b = 0.0f; // 0xD0000000
    }
    XMMATRIX ortho = XMMatrixOrthographicLH(static_cast<float>(m_lpDX->m_client_width), static_cast<float>(m_lpDX->m_client_height), 0.0f, 1.0f);
    m_lpDX->m_lpDevice->SetTransform(3, &ortho);

    m_lpDX->m_lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, verts, sizeof(MDVERTEX));

    // Undo unusual state changes.
    m_lpDX->m_lpDevice->SetDepth(true);
    m_lpDX->m_lpDevice->SetBlendState(false);
    XMMATRIX identity = XMMatrixIdentity();
    m_lpDX->m_lpDevice->SetTransform(3, &identity);
    //m_lpDX->m_lpDevice->SetVertexShader(NULL, NULL);
    //m_lpDX->m_lpDevice->SetPixelShader(NULL, NULL);
}

void CPluginShell::RenderBuiltInTextMsgs()
{
    int _show_press_f1_NOW = (m_show_press_f1_msg & int(m_time < PRESS_F1_DUR));

    {
        D2D1_RECT_F r{};
        DWORD textColor = 0xFFFFFFFF;
        D2D1_COLOR_F fTextColor = D2D1::ColorF(textColor, static_cast<FLOAT>(((textColor & 0xFF000000) >> 24) / 255.0f));

        if (m_show_help)
        {
            if (!m_helpManual.IsVisible()) m_helpManual.Initialize(m_lpDX->GetD2DDeviceContext());
            m_helpManual.SetAlignment(AlignNear, AlignNear);
            m_helpManual.SetTextColor(fTextColor);
            m_helpManual.SetTextOpacity(fTextColor.a);
            m_helpManual.SetTextShadow(false);
            m_helpManual.SetTextStyle(GetFont(HELPSCREEN_FONT));

            //int y = m_upper_left_corner_y;

            r = D2D1::RectF(0.0f, 0.0f, static_cast<FLOAT>(GetWidth()), static_cast<FLOAT>(GetHeight()));
            m_helpManual.SetContainer(r);
            m_helpManual.SetVisible(true);

            m_helpManual.SetText(AutoWide(reinterpret_cast<char*>(g_szHelp)));
            m_text.DrawD2DText(GetFont(HELPSCREEN_FONT), &m_helpManual, AutoWide(reinterpret_cast<char*>(g_szHelp)), &r, DT_CALCRECT, textColor, false);

            r.top += static_cast<FLOAT>(m_upper_left_corner_y);
            r.left += static_cast<FLOAT>(m_left_edge);
            r.right += static_cast<FLOAT>(m_left_edge + PLAYLIST_INNER_MARGIN * 2.0f);
            r.bottom += static_cast<FLOAT>(m_upper_left_corner_y + PLAYLIST_INNER_MARGIN * 2.0f);
            DrawDarkTranslucentBox(&r);

            r.top += PLAYLIST_INNER_MARGIN;
            r.left += PLAYLIST_INNER_MARGIN;
            r.right -= PLAYLIST_INNER_MARGIN;
            r.bottom -= PLAYLIST_INNER_MARGIN;
            m_helpManual.SetContainer(r);
            m_helpManual.SetText(AutoWide(reinterpret_cast<char*>(g_szHelp)));
            m_text.DrawD2DText(GetFont(HELPSCREEN_FONT), &m_helpManual, AutoWide(reinterpret_cast<char*>(g_szHelp)), &r, 0, textColor, false);

            m_text.RegisterElement(&m_helpManual);

            m_upper_left_corner_y += static_cast<int>(r.bottom - r.top + PLAYLIST_INNER_MARGIN * 3.0f);
        }
        else
        {
            if (m_helpManual.IsVisible())
            {
                m_helpManual.SetVisible(false);
                m_text.UnregisterElement(&m_helpManual);
            }
        }

        // Render 'Press F1 for Help' message in lower-right corner.
        if (_show_press_f1_NOW)
        {
            if (!m_helpMessage.IsVisible())
            {
                m_helpMessage.Initialize(m_lpDX->GetD2DDeviceContext());
                m_helpMessage.SetAlignment(AlignFar, AlignFar);
                m_helpMessage.SetTextColor(fTextColor);
                m_helpMessage.SetTextOpacity(fTextColor.a);
                m_helpMessage.SetTextShadow(true);
                m_helpMessage.SetTextStyle(GetFont(DECORATIVE_FONT));
                m_helpMessage.SetText(WASABI_API_LNGSTRINGW(IDS_PRESS_F1_MSG));
            }

            int dx = static_cast<int>(160.0f * powf(m_time / static_cast<float>(PRESS_F1_DUR), static_cast<float>(PRESS_F1_EXP)));
            r = D2D1::RectF(static_cast<FLOAT>(m_left_edge), static_cast<FLOAT>(m_lower_right_corner_y - GetFontHeight(DECORATIVE_FONT)), static_cast<FLOAT>(m_right_edge + dx), static_cast<FLOAT>(m_lower_right_corner_y));
            m_helpMessage.SetContainer(r);
            if (!m_helpMessage.IsVisible())
            {
                m_helpMessage.SetVisible(true);

                m_lower_right_corner_y -= m_text.DrawD2DText(GetFont(DECORATIVE_FONT), &m_helpMessage, WASABI_API_LNGSTRINGW(IDS_PRESS_F1_MSG), &r, DT_RIGHT, textColor, false);
                m_text.RegisterElement(&m_helpMessage);
            }
        }
        else
        {
            if (m_helpMessage.IsVisible())
            {
                m_helpMessage.SetVisible(false);
                m_text.UnregisterElement(&m_helpMessage);
            }
        }
    }
}

// Draws playlist.
void CPluginShell::RenderPlaylist()
{
    if (m_show_playlist)
    {
        D2D1_RECT_F r;
        DWORD_PTR pSongs = NULL, p_now_playing = NULL;
        SendMessageTimeout(m_hWndWinamp, WM_USER, 0, IPC_GETLISTLENGTH, SMTO_NORMAL | SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 100, &pSongs); int nSongs = static_cast<int>(pSongs);
        SendMessageTimeout(m_hWndWinamp, WM_USER, 0, IPC_GETLISTPOS, SMTO_NORMAL | SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 100, &p_now_playing); int now_playing = static_cast<int>(p_now_playing);
        DWORD dwFlags = DT_SINGLELINE; //| DT_NOPREFIX | DT_WORD_ELLIPSIS; // Note: `dwFlags` is used for both DDRAW and DX9
        int nFontHeight = GetFontHeight(PLAYLIST_FONT);
        if (nSongs <= 0)
        {
            m_show_playlist = false;
        }
        else
        {
            int playlist_vert_pixels = m_lower_left_corner_y - m_upper_left_corner_y;
            int disp_lines = std::min(MAX_SONGS_PER_PAGE, (playlist_vert_pixels - static_cast<int>(PLAYLIST_INNER_MARGIN) * 2) / nFontHeight);
            //int total_pages = nSongs / disp_lines;

            if (disp_lines <= 0)
                return;

            // Apply PgUp/PgDn keypresses since last time.
            m_playlist_pos -= m_playlist_pageups * disp_lines;
            m_playlist_pageups = 0;

            if (m_playlist_pos < 0)
                m_playlist_pos = 0;
            if (m_playlist_pos >= nSongs)
                m_playlist_pos = nSongs - 1;

#ifdef _FOOBAR
            SendMessageTimeout(m_hWndWinamp, WM_USER, m_playlist_pos, IPC_SETPLAYLISTPOS, SMTO_NORMAL | SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 100, NULL);
#endif

            int cur_page = static_cast<int>(m_playlist_pos / disp_lines);
            //int cur_line = (m_playlist_pos + disp_lines - 1) % disp_lines;
            int new_top_idx = cur_page * disp_lines;
            int new_btm_idx = new_top_idx + disp_lines;
            wchar_t buf[1024] = {0};

            // Ask Winamp for the song names, but DO IT BEFORE getting the DC,
            // otherwise might crash (~DDRAW port).
            if (m_playlist_top_idx != new_top_idx || m_playlist_btm_idx != new_btm_idx)
            {
                for (int i = 0; i < disp_lines; i++)
                {
                    int j = new_top_idx + i;
                    if (j < nSongs)
                    {
                        // Clip maximum length of song name to 240 characters, to prevent overflows.
                        DWORD_PTR szTitle = NULL; SendMessageTimeout(m_hWndWinamp, WM_WA_IPC, j, IPC_GETPLAYLISTTITLEW, SMTO_NORMAL | SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 100, &szTitle);
                        wcsncpy_s(buf, reinterpret_cast<wchar_t*>(szTitle), 240);
                        swprintf_s(m_playlist[i], L"%d. %s ", j + 1, buf); // leave an extra space at end, so italicized fonts do not get clipped
                    }
                }
            }

            // Update playlist cache, if necessary.
            if (m_playlist_top_idx != new_top_idx || m_playlist_btm_idx != new_btm_idx)
            {
                m_playlist_top_idx = new_top_idx;
                m_playlist_btm_idx = new_btm_idx;
                m_playlist_width_pixels = 0.0f;

                FLOAT max_w = static_cast<FLOAT>(std::min(m_right_edge - m_left_edge, m_lpDX->m_client_width - TEXT_MARGIN * 2 - static_cast<int>(PLAYLIST_INNER_MARGIN) * 2));

                for (int i = 0; i < disp_lines; i++)
                {
                    int j = new_top_idx + i;
                    if (j < nSongs)
                    {
                        // Clip maximum length of song name to 240 characters, to prevent overflows.
                        //char bufA[240] = L'\0';
                        //LRESULT szTitle = SendMessage(m_hWndWinamp, WM_WA_IPC, j, IPC_GETPLAYLISTTITLE);
                        //strcpy_s(bufA, reinterpret_cast<char*>(szTitle), 240);
                        //sprintf_s(m_playlist[i], "%d. %s ", j + 1, bufA); // leave an extra space @ end, so italicized fonts don't get clipped

                        r = {0.0f, 0.0f, max_w, 1024.0f};
                        if (!m_playlist_song[i].IsVisible())
                            m_playlist_song[i].Initialize(m_lpDX->GetD2DDeviceContext());
                        m_playlist_song[i].SetAlignment(AlignNear, AlignNear);
                        m_playlist_song[i].SetTextColor(D2D1::ColorF(0xFFFFFFFF));
                        m_playlist_song[i].SetTextOpacity(1.0f);
                        m_playlist_song[i].SetContainer(r);
                        m_playlist_song[i].SetVisible(true);
                        m_playlist_song[i].SetText(m_playlist[i]);
                        m_playlist_song[i].SetTextStyle(GetFont(PLAYLIST_FONT));
                        m_playlist_song[i].SetTextShadow(false);
                        int h = m_text.DrawD2DText(GetFont(PLAYLIST_FONT), &m_playlist_song[i], m_playlist[i], &r, dwFlags | DT_CALCRECT, 0xFFFFFFFF, false);
                        float w = r.right - r.left;
                        if (w > 0)
                            m_playlist_width_pixels = std::max(m_playlist_width_pixels, w);
                        if (h > 0)
                            nFontHeight = std::max(nFontHeight, h);
                    }
                    else
                    {
                        m_playlist[i][0] = L'\0';
                        if (m_playlist_song[i].IsVisible())
                            m_playlist_song[i].SetVisible(false);
                    }
                }

                if (m_playlist_width_pixels == 0.0f || m_playlist_width_pixels > max_w)
                    m_playlist_width_pixels = max_w;
            }

            int start = std::max(0, cur_page * disp_lines);
            int end = std::min(nSongs, (cur_page + 1) * disp_lines);

            // Draw dark box around where the playlist will go.
            r.top = static_cast<FLOAT>(m_upper_left_corner_y);
            r.left = static_cast<FLOAT>(m_left_edge);
            r.right = static_cast<FLOAT>(m_left_edge + m_playlist_width_pixels + PLAYLIST_INNER_MARGIN * 2.0f);
            r.bottom = static_cast<FLOAT>(m_upper_left_corner_y + (end - start) * nFontHeight + PLAYLIST_INNER_MARGIN * 2.0f);
            DrawDarkTranslucentBox(&r);

            // Draw playlist text.
            int y = m_upper_left_corner_y + static_cast<int>(PLAYLIST_INNER_MARGIN);
            for (int i = start; i < end; i++)
            {
                r = {static_cast<FLOAT>(m_left_edge + PLAYLIST_INNER_MARGIN),
                     static_cast<FLOAT>(y),
                     static_cast<FLOAT>(m_left_edge + PLAYLIST_INNER_MARGIN + m_playlist_width_pixels),
                     static_cast<FLOAT>(y + nFontHeight)};
                m_playlist_song[i - start].SetContainer(r);
                DWORD color;
                if (m_lpDX->GetBitDepth() == 8)
                    color = (i == m_playlist_pos) ? (i == now_playing ? 0xFFFFFFFF : 0xFFFFFFFF)
                                                  : (i == now_playing ? 0xFFFFFFFF : 0xFF707070);
                else
                    color = (i == m_playlist_pos) ? (i == now_playing ? PLAYLIST_COLOR_BOTH : PLAYLIST_COLOR_HILITE_TRACK)
                                                  : (i == now_playing ? PLAYLIST_COLOR_PLAYING_TRACK : PLAYLIST_COLOR_NORMAL);

                D2D1_COLOR_F fColor = D2D1::ColorF(color, GetAlpha(color));
                m_playlist_song[i - start].SetTextColor(fColor);
                m_playlist_song[i - start].SetTextOpacity(fColor.a);
                y += nFontHeight;
                m_text.DrawD2DText(GetFont(PLAYLIST_FONT), &m_playlist_song[i - start], m_playlist[i - start], &r, dwFlags, color, false);
                m_text.RegisterElement(&m_playlist_song[i - start]);
            }
        }
    }
    else
        for (unsigned int i = 0; i < MAX_SONGS_PER_PAGE; ++i)
            if (m_playlist_song[i].IsVisible())
            {
                m_playlist_song[i].SetVisible(false);
                m_text.UnregisterElement(&m_playlist_song[i]);
            }
}

// Aligns waves, using recursive (mipmap-style) least-error matching.
// Note: `NUM_WAVEFORM_SAMPLES` must be between 32 and 576.
void CPluginShell::AlignWaves()
{
    int align_offset[2] = {0, 0};
    int nSamples = NUM_WAVEFORM_SAMPLES;

#if (NUM_WAVEFORM_SAMPLES < NUM_AUDIO_BUFFER_SAMPLES) // [don't let this code bloat the DLL size if it's not going to be used]
    constexpr auto MAX_OCTAVES = 10;
    int octaves = (int)floorf(logf((float)(NUM_AUDIO_BUFFER_SAMPLES - nSamples)) / logf(2.0f));
    if (octaves < 4)
        return;
    if (octaves > MAX_OCTAVES)
        octaves = MAX_OCTAVES;

    for (int ch = 0; ch < 2; ch++)
    {
        // Only worry about matching the lower `nSamples` samples.
        float temp_new[MAX_OCTAVES][NUM_AUDIO_BUFFER_SAMPLES];
        float temp_old[MAX_OCTAVES][NUM_AUDIO_BUFFER_SAMPLES];
        static float temp_weight[MAX_OCTAVES][NUM_AUDIO_BUFFER_SAMPLES];
        static int first_nonzero_weight[MAX_OCTAVES];
        static int last_nonzero_weight[MAX_OCTAVES];
        int spls[MAX_OCTAVES];
        int space[MAX_OCTAVES];

        memcpy_s(temp_new[0], sizeof(temp_new[0]), m_sound.fWaveform[ch].data(), sizeof(float) * NUM_AUDIO_BUFFER_SAMPLES);
        memcpy_s(temp_old[0], sizeof(temp_old[0]), &m_oldwave[ch][m_prev_align_offset[ch]], sizeof(float) * nSamples);
        spls[0] = NUM_AUDIO_BUFFER_SAMPLES;
        space[0] = NUM_AUDIO_BUFFER_SAMPLES - nSamples;

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
            //  (so test 32 samples, with 4 offsets)
            //int compare_samples = spls[octave] - space[octave];

            int lowest_err_offset = -1;
            float lowest_err_amount = 0;
            for (int n = n1; n < n2; n++)
            {
                float err_sum = 0;
                //for (int i = 0; i < compare_samples; i++)
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
            //     - Say `lowest_err_offset` was 2.
            //     - That corresponds to samples 4 and 5 of the next octave.
            //     - Also, expand about this by 2 samples? YES.
            //  (so test 64 samples, with 8->4 offsets)
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
    memcpy_s(m_oldwave[0], sizeof(m_oldwave[0]), m_sound.fWaveform[0].data(), sizeof(float) * NUM_AUDIO_BUFFER_SAMPLES);
    memcpy_s(m_oldwave[1], sizeof(m_oldwave[1]), m_sound.fWaveform[1].data(), sizeof(float) * NUM_AUDIO_BUFFER_SAMPLES);
    m_prev_align_offset[0] = align_offset[0];
    m_prev_align_offset[1] = align_offset[1];

    // Finally, apply the results: modify m_sound.fWaveform[2][0..576]
    // by scooting the aligned samples so that they start at m_sound.fWaveform[2][0].
    for (int ch = 0; ch < 2; ch++)
        if (align_offset[ch] > 0)
        {
            for (int i = 0; i < nSamples; i++)
                m_sound.fWaveform[ch][i] = m_sound.fWaveform[ch][i + align_offset[ch]];
            // Zero the rest out, so it is visually evident that these samples are now bogus.
            memset(&m_sound.fWaveform[ch][nSamples], 0, (NUM_AUDIO_BUFFER_SAMPLES - nSamples) * sizeof(float));
        }
}

// Notifies renderers that device resources need to be released.
void CPluginShell::OnDeviceLost()
{
    CleanUpDX11(TRUE);

    m_lpDX->m_lpDevice.reset();
}

// Notifies renderers that device resources may now be re-created.
void CPluginShell::OnDeviceRestored()
{
    m_lpDX->CreateDeviceDependentResources();

    m_lpDX->CreateWindowSizeDependentResources();

    if (!AllocateDX11())
    {
        m_lpDX->m_ready = false;
        return;
    }
}