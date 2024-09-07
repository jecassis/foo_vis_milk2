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

/*
  Order of Function Calls
  -----------------------
      The only code that will be called by the plugin framework are the
      12 virtual functions in "plugin.h". But in what order are they called?
      A breakdown follows. A function name in { } means that it is only
      called under certain conditions.

      Order of function calls...

      When the PLUGIN launches
      ------------------------
          INITIALIZATION
              OverrideDefaults
              MilkDropPreInitialize
              MilkDropReadConfig
              << DirectX gets initialized at this point >>
              AllocateMilkDropNonDX11
              AllocateMilkDropDX11
          RUNNING
              +--> { CleanUpMilkDropDX11 + AllocateMilkDropDX11 }  // called together when user resizes window or toggles fullscreen<->windowed.
              |    MilkDropRenderFrame
              |    MilkDropRenderUI
              |    { MilkDropWindowProc }  // called, between frames, on mouse/keyboard/system events. 100% thread safe.
              +----<< repeat >>
          CLEANUP
              CleanUpMilkDropDX11
              CleanUpMilkDropNonDX11
              << DirectX gets uninitialized at this point >>

      When the CONFIG PANEL launches
      ------------------------------
          INITIALIZATION
              OverrideDefaults
              MilkDropPreInitialize
              MilkDropReadConfig
              << DirectX gets initialized at this point >>
          RUNNING
              { MilkDropConfigTabProc }  // called on startup & on keyboard events
          CLEANUP
              [ MilkDropWriteConfig ]  // only called if user clicked 'OK' to exit
              << DirectX gets uninitialized at this point >>
*/

#include "pch.h"
#include "plugin.h"

#include "defines.h"
#include "shell_defines.h"
#include "utility.h"
#include "support.h"
#define WASABI_API_ORIG_HINST GetInstance()
#include "api.h"
//#include "resource.h"
#include <nu/AutoChar.h>
#include <nu/AutoWide.h>

//#pragma comment(lib, "d3dcompiler.lib")
//#pragma comment(lib, "dxguid.lib")

int warand() { return rand(); }

void NSEEL_HOSTSTUB_EnterMutex() {}

void NSEEL_HOSTSTUB_LeaveMutex() {}

#ifdef NS_EEL2
void NSEEL_VM_resetvars(NSEEL_VMCTX ctx)
{
    NSEEL_VM_freeRAM(ctx);
    NSEEL_VM_remove_all_nonreg_vars(ctx);
}
#endif

_locale_t g_use_C_locale = 0;

extern CPlugin g_plugin;

// From "support.cpp".
extern bool g_bDebugOutput;
extern bool g_bDumpFileCleared;

// For `__UpdatePresetList`.
volatile HANDLE g_hThread;        // only r/w from our MAIN thread
volatile bool g_bThreadAlive;     // set true by MAIN thread, and set false upon exit from 2nd thread.
volatile int g_bThreadShouldQuit; // set by MAIN thread to flag 2nd thread that it wants it to exit.
static CRITICAL_SECTION g_cs;

#define IsAlphabetChar(x) ((x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z'))
#define IsAlphanumericChar(x) ((x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z') || (x >= '0' && x <= '9') || x == '.')
#define IsNumericChar(x) (x >= '0' && x <= '9')

// Check if file exists.
BOOL FileExists(LPCTSTR szPath)
{
    DWORD dwAttrib = GetFileAttributes(szPath);

    return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

// Copies the given string to the clipboard.
void copyStringToClipboardA(const char* source)
{
    int ok = OpenClipboard(NULL);
    if (!ok)
        return;

    HGLOBAL clipbuffer;
    EmptyClipboard();
    if ((clipbuffer = GlobalAlloc(GMEM_DDESHARE, (strlen(source) + 1) * sizeof(wchar_t))) == NULL)
        return;
    else
    {
        char* buffer = reinterpret_cast<char*>(GlobalLock(clipbuffer));
        if (buffer)
            strcpy_s(buffer, strlen(source) + 1, source);
        else
            return;
    }
    GlobalUnlock(clipbuffer);
    SetClipboardData(CF_TEXT, clipbuffer);
    CloseClipboard();
}

// Copies the given string to the clipboard.
void copyStringToClipboardW(const wchar_t* source)
{
    int ok = OpenClipboard(NULL);
    if (!ok)
        return;

    HGLOBAL clipbuffer;
    EmptyClipboard();
    if ((clipbuffer = GlobalAlloc(GMEM_DDESHARE, (wcslen(source) + 1) * sizeof(wchar_t))) == NULL)
        return;
    else
    {
        wchar_t* buffer = reinterpret_cast<wchar_t*>(GlobalLock(clipbuffer));
        if (buffer)
            wcscpy_s(buffer, wcslen(source) + 1, source);
        else
            return;
    }
    GlobalUnlock(clipbuffer);
    SetClipboardData(CF_UNICODETEXT, clipbuffer);
    CloseClipboard();
}

// Copies a string from the clipboard.
char* getStringFromClipboardA()
{
    int ok = OpenClipboard(NULL);
    if (!ok)
        return NULL;

    HANDLE hData = GetClipboardData(CF_TEXT);
    char* buffer = reinterpret_cast<char*>(GlobalLock(hData));
    GlobalUnlock(hData);
    CloseClipboard();
    return buffer;
}

// Copies a string from the clipboard.
wchar_t* getStringFromClipboardW()
{
    int ok = OpenClipboard(NULL);
    if (!ok)
        return NULL;

    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    wchar_t* buffer = reinterpret_cast<wchar_t*>(GlobalLock(hData));
    GlobalUnlock(hData);
    CloseClipboard();
    return buffer;
}

void ConvertCRsToLFCA(const char* src, char* dst)
{
    while (*src)
    {
        //char ch = *src;
        if (*src == '\r' && *(src + 1) == '\n')
        {
            *dst++ = LINEFEED_CONTROL_CHAR;
            src += 2;
        }
        else
        {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

void ConvertCRsToLFCW(const wchar_t* src, wchar_t* dst)
{
    while (*src)
    {
        //wchar_t ch = *src;
        if (*src == L'\r' && *(src + 1) == L'\n')
        {
            *dst++ = LINEFEED_CONTROL_CHAR;
            src += 2;
        }
        else
        {
            *dst++ = *src++;
        }
    }
    *dst = L'\0';
}

void ConvertLFCToCRsA(const char* src, char* dst)
{
    while (*src)
    {
        //char ch = *src;
        if (*src == LINEFEED_CONTROL_CHAR)
        {
            *dst++ = '\r'; // 13
            *dst++ = '\n'; // 10
            src++;
        }
        else
        {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

void ConvertLFCToCRsW(const wchar_t* src, wchar_t* dst)
{
    while (*src)
    {
        //wchar_t ch = *src;
        if (*src == LINEFEED_CONTROL_CHAR)
        {
            *dst++ = L'\r'; // 13
            *dst++ = L'\n'; // 10
            src++;
        }
        else
        {
            *dst++ = *src++;
        }
    }
    *dst = L'\0';
}

// Read in all characters and replace character combinations
// {13; 13+10; 10} with `LINEFEED_CONTROL_CHAR`, if
// `bConvertLFsToSpecialChar` is true.
bool ReadFileToString(const wchar_t* szBaseFilename, char* szDestText, int nMaxBytes, bool bConvertLFsToSpecialChar)
{
    wchar_t szFile[MAX_PATH];
#ifndef _FOOBAR
    swprintf_s(szFile, L"%ls%ls", g_plugin.m_szMilkdrop2Path, szBaseFilename);
#else
    swprintf_s(szFile, L"%ls%ls", g_plugin.m_szComponentDirPath, szBaseFilename);
#endif

    FILE* f;
    errno_t err = _wfopen_s(&f, szFile, L"rb");
    if (err || !f)
    {
        /*
        wchar_t buf[1024] = {0}, title[64] = {0};
        swprintf_s(buf, WASABI_API_LNGSTRINGW(IDS_UNABLE_TO_READ_DATA_FILE_X), szFile);
        g_plugin.DumpDebugMessage(buf);
        MessageBox(NULL, buf, WASABI_API_LNGSTRINGW_BUF(IDS_MILKDROP_ERROR, title, 64), MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
        */
        return false;
    }
    int len = 0;
    int x;
    char prev_ch = 0;
    while ((x = fgetc(f)) >= 0 && len < nMaxBytes - 4)
    {
        char orig_ch = (char)x;
        char ch = orig_ch;
        bool bSkipChar = false;
        if (bConvertLFsToSpecialChar)
        {
            if (ch == '\n')
            {
                if (prev_ch == '\r')
                    bSkipChar = true;
                else
                    ch = LINEFEED_CONTROL_CHAR;
            }
            else if (ch == '\r')
                ch = LINEFEED_CONTROL_CHAR;
        }

        if (!bSkipChar)
            szDestText[len++] = ch;
        prev_ch = orig_ch;
    }
    szDestText[len] = 0;
    szDestText[len++] = ' '; // make sure there is some whitespace after
    fclose(f);
    return true;
}

// These callback functions are called by "menu.cpp" whenever the user finishes editing an `eval_` expression.
void OnUserEditedPerFrame(LPARAM param1, LPARAM param2)
{
    UNREFERENCED_PARAMETER(param1);
    UNREFERENCED_PARAMETER(param2);
    g_plugin.m_pState->RecompileExpressions(RECOMPILE_PRESET_CODE, 0);
}

void OnUserEditedPerPixel(LPARAM param1, LPARAM param2)
{
    UNREFERENCED_PARAMETER(param1);
    UNREFERENCED_PARAMETER(param2);
    g_plugin.m_pState->RecompileExpressions(RECOMPILE_PRESET_CODE, 0);
}

void OnUserEditedPresetInit(LPARAM param1, LPARAM param2)
{
    UNREFERENCED_PARAMETER(param1);
    UNREFERENCED_PARAMETER(param2);
    g_plugin.m_pState->RecompileExpressions(RECOMPILE_PRESET_CODE, 1);
}

void OnUserEditedWavecode(LPARAM param1, LPARAM param2)
{
    UNREFERENCED_PARAMETER(param1);
    UNREFERENCED_PARAMETER(param2);
    g_plugin.m_pState->RecompileExpressions(RECOMPILE_WAVE_CODE, 0);
}

void OnUserEditedWavecodeInit(LPARAM param1, LPARAM param2)
{
    UNREFERENCED_PARAMETER(param1);
    UNREFERENCED_PARAMETER(param2);
    g_plugin.m_pState->RecompileExpressions(RECOMPILE_WAVE_CODE, 1);
}

void OnUserEditedShapecode(LPARAM param1, LPARAM param2)
{
    UNREFERENCED_PARAMETER(param1);
    UNREFERENCED_PARAMETER(param2);
    g_plugin.m_pState->RecompileExpressions(RECOMPILE_SHAPE_CODE, 0);
}

void OnUserEditedShapecodeInit(LPARAM param1, LPARAM param2)
{
    UNREFERENCED_PARAMETER(param1);
    UNREFERENCED_PARAMETER(param2);
    g_plugin.m_pState->RecompileExpressions(RECOMPILE_SHAPE_CODE, 1);
}

void OnUserEditedWarpShaders(LPARAM param1, LPARAM param2)
{
    UNREFERENCED_PARAMETER(param1);
    UNREFERENCED_PARAMETER(param2);
    g_plugin.m_bNeedRescanTexturesDir = true;
    g_plugin.ClearErrors(ERR_PRESET);
    if (g_plugin.m_nMaxPSVersion == 0)
        return;
    if (!g_plugin.RecompilePShader(g_plugin.m_pState->m_szWarpShadersText, &g_plugin.m_shaders.warp, SHADER_WARP, false, g_plugin.m_pState->m_nWarpPSVersion))
    {
        // Switch to fallback.
        g_plugin.m_fallbackShaders_ps.warp.ptr->AddRef();
        g_plugin.m_fallbackShaders_ps.warp.CT->AddRef();
        g_plugin.m_shaders.warp = g_plugin.m_fallbackShaders_ps.warp;
    }
}

void OnUserEditedCompShaders(LPARAM param1, LPARAM param2)
{
    UNREFERENCED_PARAMETER(param1);
    UNREFERENCED_PARAMETER(param2);
    g_plugin.m_bNeedRescanTexturesDir = true;
    g_plugin.ClearErrors(ERR_PRESET);
    if (g_plugin.m_nMaxPSVersion == 0)
        return;
    if (!g_plugin.RecompilePShader(g_plugin.m_pState->m_szCompShadersText, &g_plugin.m_shaders.comp, SHADER_COMP, false, g_plugin.m_pState->m_nCompPSVersion))
    {
        // Switch to fallback.
        g_plugin.m_fallbackShaders_ps.comp.ptr->AddRef();
        g_plugin.m_fallbackShaders_ps.comp.CT->AddRef();
        g_plugin.m_shaders.comp = g_plugin.m_fallbackShaders_ps.comp;
    }
}

// Modify the help screen text here.
// Watch the number of lines, though; if there are too many, they will get cut off;
// and watch the length of the lines, since there is no wordwrap.
// A good guideline: the entire help screen should be visible when fullscreen
// at 640x480 and using the default help screen font.
wchar_t* g_szHelp = nullptr;

// Here, you have the option of overriding the "default defaults"
// for the stuff on tab 1 of the config panel, replacing them
// with custom defaults for your plugin.
// To override any of the defaults, just uncomment the line
// and change the value.
// DO NOT modify these values from any function but this one!
void CPlugin::OverrideDefaults()
{
    //m_start_fullscreen      = 0;   // 0 or 1
    //m_start_desktop         = 0;   // 0 or 1
    //m_fake_fullscreen_mode  = 0;   // 0 or 1
    m_max_fps_fs            = 30;  // 1-144, or 0 for 'unlimited'
    //m_max_fps_d             = 30;  // 1-144, or 0 for 'unlimited'
    //m_max_fps_w             = 30;  // 1-144, or 0 for 'unlimited'
    m_show_press_f1_msg     = 1;   // 0 or 1
    //m_allow_page_tearing_w  = 0;   // 0 or 1
    m_allow_page_tearing_fs = 0;   // 0 or 1
    //m_minimize_winamp       = 1;   // 0 or 1
    //m_desktop_textlabel_boxes = 1; // 0 or 1
    m_save_cpu              = 0;   // 0 or 1
    //m_skin                  = 1;   // 0 or 1
    m_fix_slow_text         = 1;

    //wcscpy_s(m_fontinfo[0].szFace, "Trebuchet MS"); // system font
    //m_fontinfo[0].nSize     = 18;
    //m_fontinfo[0].bBold     = 0;
    //m_fontinfo[0].bItalic   = 0;
    //wcscpy_s(m_fontinfo[1].szFace, "Times New Roman"); // decorative font
    //m_fontinfo[1].nSize     = 24;
    //m_fontinfo[1].bBold     = 0;
    //m_fontinfo[1].bItalic   = 1;

    // Don't override default FS mode here; shell is now smart and sets it to match
    // the current desktop display mode, by default.

    //m_disp_mode_fs.Width = 1024; // normally 640
    //m_disp_mode_fs.Height = 768; // normally 480
    // Use either D3DFMT_X8R8G8B8 or D3DFMT_R5G6B5.
    // The former will match to any 32-bit color format available,
    // and the latter will match to any 16-bit color available,
    // if that exact format can't be found.
    //m_disp_mode_fs.Format = D3DFMT_UNKNOWN; //<- this tells config panel & visualizer to use current display mode as a default!!   //D3DFMT_X8R8G8B8;
    //m_disp_mode_fs.RefreshRate = 60;
}

// Initialize every data member in CPlugin with their default values.
// To initialize any of the variables with random values
// (using rand()), seed the random number generator first!
// seed the system's random number generator w/the current system time:
//srand((unsigned)time(NULL));  -don't - let winamp do it
// (If you want to change the default values for settings that are part of
// the plugin shell (framework), do so from OverrideDefaults() above.)
void CPlugin::MilkDropPreInitialize()
{
    // Attempt to load a Unicode `F1` help message.
    g_szHelp = reinterpret_cast<wchar_t*>(GetTextResource(IDR_HELP_TEXT, 0));

    // CONFIG PANEL SETTINGS THAT MilkDrop ADDED (TAB #2)
    m_bInitialPresetSelected = false;
    m_fBlendTimeUser = 1.7f;
    m_fBlendTimeAuto = 2.7f;
    m_fTimeBetweenPresets = 16.0f;
    m_fTimeBetweenPresetsRand = 10.0f;
    m_bSequentialPresetOrder = false;
    m_bHardCutsDisabled = true;
    m_fHardCutLoudnessThresh = 2.5f;
    m_fHardCutHalflife = 60.0f;
    //m_nWidth = 1024;
    //m_nHeight = 768;
    //m_nDispBits = 16;
    m_nCanvasStretch = 0;
    m_nTexSizeX = -1; // -1 means "auto"
    m_nTexSizeY = -1; // -1 means "auto"
    m_nTexBitsPerCh = 8;
    m_nGridX = 48; //32;
    m_nGridY = 36; //24;

    m_bShowPressF1ForHelp = true;
    m_bShowMenuToolTips = true; // NOTE: THIS IS CURRENTLY HARDWIRED TO TRUE - NO OPTION TO CHANGE
    m_n16BitGamma = 2;
    m_bAutoGamma = true;
    //m_nFpsLimit = -1;
    m_bEnableRating = true;
    m_bSongTitleAnims = true;
    m_fSongTitleAnimDuration = 1.7f;
    m_fTimeBetweenRandomSongTitles = -1.0f;
    m_fTimeBetweenRandomCustomMsgs = -1.0f;
    m_nSongTitlesSpawned = 0;
    m_nCustMsgsSpawned = 0;
    m_nFramesSinceResize = 0;

    //m_bAlways3D = false;
    //m_fStereoSep = 1.0f;
    //m_bAlwaysOnTop = false;
    //m_bFixSlowText = true;
    //m_bWarningsDisabled = false;
    m_bWarningsDisabled2 = false;
    //m_bAnisotropicFiltering = true;
    m_bPresetLockOnAtStartup = false;
    m_bPreventScollLockHandling = false;
    m_nMaxPSVersion_ConfigPanel = -1;
    m_nMaxPSVersion_DX = -1;
    m_nMaxPSVersion = -1;
    m_nMaxImages = 32;
    m_nMaxBytes = 16000000;

    //m_pFragmentLinker = NULL;
    //m_pCompiledFragments = NULL;
    m_pShaderCompileErrors = NULL;
    //m_vs_warp = NULL;
    //m_ps_warp = NULL;
    //m_vs_comp = NULL;
    //m_ps_comp = NULL;
    ZeroMemory(&m_shaders, sizeof(PShaderSet));
    ZeroMemory(&m_OldShaders, sizeof(PShaderSet));
    ZeroMemory(&m_NewShaders, sizeof(PShaderSet));
    ZeroMemory(&m_fallbackShaders_vs, sizeof(VShaderSet));
    ZeroMemory(&m_fallbackShaders_ps, sizeof(PShaderSet));
    ZeroMemory(m_BlurShaders, sizeof(m_BlurShaders));
    m_bWarpShaderLock = false;
    m_bCompShaderLock = false;
    m_bNeedRescanTexturesDir = true;

    // RUNTIME SETTINGS THAT MilkDrop ADDED
    m_prev_time = GetTime() - 0.0333f; // note: this will be updated each frame, at bottom of `MilkDropRenderFn()`.
    m_bTexSizeWasAutoPow2 = false;
    m_bTexSizeWasAutoExact = false;
    //m_bPresetLockedByUser = false;  NOW SET IN DERIVED SETTINGS
    m_bPresetLockedByCode = false;
    m_fStartTime = 0.0f;
    m_fPresetStartTime = 0.0f;
    m_fNextPresetTime = -1.0f; // negative value means no time set (...it will be auto-set on first call to UpdateTime)
    m_nLoadingPreset = 0;
    m_nPresetsLoadedTotal = 0;
    m_fSnapPoint = 0.5f;
    m_pState = &m_state_DO_NOT_USE[0];
    m_pOldState = &m_state_DO_NOT_USE[1];
    m_pNewState = &m_state_DO_NOT_USE[2];
    m_UI_mode = UI_REGULAR;
    m_bShowShaderHelp = false;

    m_nMashSlot = 0; //0..MASH_SLOTS-1
    for (int mash = 0; mash < MASH_SLOTS; mash++)
        m_nLastMashChangeFrame[mash] = 0;

    //m_nTrackPlaying = 0;
    //m_nSongPosMS = 0;
    //m_nSongLenMS = 0;
    m_bUserPagedUp = false;
    m_bUserPagedDown = false;

    m_fMotionVectorsTempDx = 0.0f;
    m_fMotionVectorsTempDy = 0.0f;

    m_waitstring.bActive = false;
    m_waitstring.bOvertypeMode = false;
    m_waitstring.szClipboard[0] = 0;

    m_nPresets = 0;
    m_nDirs = 0;
    m_nPresetListCurPos = 0;
    m_nCurrentPreset = -1;
    m_szCurrentPresetFile[0] = 0;
    m_szLoadingPreset[0] = 0;
    m_bPresetListReady = false;
    m_szUpdatePresetMask[0] = 0;
    //m_nRatingReadProgress = -1;

    memset(&mdsound, 0, sizeof(mdsound));

    for (int i = 0; i < PRESET_HIST_LEN; i++)
        m_presetHistory[i] = L"";
    m_presetHistoryPos = 0;
    m_presetHistoryBackFence = 0;
    m_presetHistoryFwdFence = 0;

    //m_nTextHeightPixels = -1;
    //m_nTextHeightPixels_Fancy = -1;
    m_bShowFPS = false;
    m_bShowRating = false;
    m_bShowPresetInfo = false;
    m_bShowDebugInfo = false;
    m_bShowSongTitle = false;
    m_bShowSongTime = false;
    m_bShowSongLen = false;
    m_fShowRatingUntilThisTime = -1.0f;
    //ClearErrors();

    m_lpVS[0] = NULL;
    m_lpVS[1] = NULL;
#if (NUM_BLUR_TEX > 0)
    for (int i = 0; i < NUM_BLUR_TEX; i++)
        m_lpBlur[i] = NULL;
#endif
    m_lpDDSTitle = NULL;
    m_nTitleTexSizeX = 0;
    m_nTitleTexSizeY = 0;
    m_verts = NULL;
    m_verts_temp = NULL;
    m_vertinfo = NULL;
    m_indices_list = NULL;
    m_indices_strip = NULL;

    m_bHasFocus = true;
    m_bHadFocus = false;
    //m_bOrigScrollLockState  = GetKeyState(VK_SCROLL) & 1;
    //m_bMilkdropScrollLockState is derived at end of `MilkDropReadConfig()`

    m_nNumericInputMode = NUMERIC_INPUT_MODE_CUST_MSG;
    m_nNumericInputNum = 0;
    m_nNumericInputDigits = 0;
    //td_custom_msg_font m_customMessageFont[MAX_CUSTOM_MESSAGE_FONTS];
    //td_custom_msg m_customMessage[MAX_CUSTOM_MESSAGES];

    //texmgr m_texmgr; // for user sprites

    m_supertext.bRedrawSuperText = false;
    m_supertext.fStartTime = -1.0f;

    // Other initialization.
    g_bDebugOutput = false;
    g_bDumpFileCleared = false;

    swprintf_s(m_szMilkdrop2Path, L"%ls%ls", GetPluginsDirPath(), SUBDIR);
    swprintf_s(m_szPresetDir, L"%lspresets\\", m_szMilkdrop2Path);

    // Note that the configuration directory can be under "Program Files" or "Application Data"!!
    wchar_t szConfigDir[MAX_PATH] = {0};
    wcscpy_s(szConfigDir, GetConfigIniFile());
    wchar_t* p = wcsrchr(szConfigDir, L'\\');
    if (p)
        *(p + 1) = L'\0';
    swprintf_s(m_szMsgIniFile, L"%ls%ls", szConfigDir, MSG_INIFILE);
    swprintf_s(m_szImgIniFile, L"%ls%ls", szConfigDir, IMG_INIFILE);
}

// Reads the user's settings from the .INI file.
// Read the value from the .INI file for any controls
// added to the configuration panel.
void CPlugin::MilkDropReadConfig()
{
#ifndef _FOOBAR
    // Use this function         declared in   to read a value of this type
    // -----------------         -----------   ----------------------------
    // GetPrivateProfileInt      WinBase.h     int
    // GetPrivateProfileBool     utility.h     bool
    // GetPrivateProfileFloat    utility.h     float
    // GetPrivateProfileString   WinBase.h     string

    int n = 0;
    wchar_t* pIni = GetConfigIniFile();

    m_bEnableRating = GetPrivateProfileBool(L"settings", L"bEnableRating", m_bEnableRating, pIni);
    m_bHardCutsDisabled = GetPrivateProfileBool(L"settings", L"bHardCutsDisabled", m_bHardCutsDisabled, pIni);
    g_bDebugOutput = GetPrivateProfileBool(L"settings", L"bDebugOutput", g_bDebugOutput, pIni);
    //m_bShowSongInfo = GetPrivateProfileBool(L"settings", L"bShowSongInfo", m_bShowSongInfo, pIni);
    m_bShowPressF1ForHelp = GetPrivateProfileBool(L"settings", L"bShowPressF1ForHelp", m_bShowPressF1ForHelp, pIni);
    //m_bShowMenuToolTips = GetPrivateProfileBool(L"settings", L"bShowMenuToolTips", m_bShowMenuToolTips, pIni);
    m_bSongTitleAnims = GetPrivateProfileBool(L"settings", L"bSongTitleAnims", m_bSongTitleAnims, pIni);

    m_bShowFPS = GetPrivateProfileBool(L"settings", L"bShowFPS", m_bShowFPS, pIni);
    m_bShowRating = GetPrivateProfileBool(L"settings", L"bShowRating", m_bShowRating, pIni);
    m_bShowPresetInfo = GetPrivateProfileBool(L"settings", L"bShowPresetInfo", m_bShowPresetInfo, pIni);
    //m_bShowDebugInfo = GetPrivateProfileBool(L"settings", L"bShowDebugInfo", m_bShowDebugInfo, pIni);
    m_bShowSongTitle = GetPrivateProfileBool(L"settings", L"bShowSongTitle", m_bShowSongTitle, pIni);
    m_bShowSongTime = GetPrivateProfileBool(L"settings", L"bShowSongTime", m_bShowSongTime, pIni);
    m_bShowSongLen = GetPrivateProfileBool(L"settings", L"bShowSongLen", m_bShowSongLen, pIni);

    //m_bFixPinkBug = GetPrivateProfileBool(L"settings", L"bFixPinkBug", m_bFixPinkBug, pIni);
    int nTemp = GetPrivateProfileBool(L"settings", L"bFixPinkBug", -1, pIni);
    if (nTemp == 0)
        m_n16BitGamma = 0;
    else if (nTemp == 1)
        m_n16BitGamma = 2;
    m_n16BitGamma = GetPrivateProfileInt(L"settings", L"n16BitGamma", m_n16BitGamma, pIni);
    m_bAutoGamma = GetPrivateProfileBool(L"settings", L"bAutoGamma", m_bAutoGamma, pIni);
    //m_bAlways3D = GetPrivateProfileBool(L"settings", L"bAlways3D", m_bAlways3D, pIni);
    //m_fStereoSep = GetPrivateProfileFloat(L"settings", L"fStereoSep", m_fStereoSep, pIni);
    //m_bFixSlowText = GetPrivateProfileBool(L"settings", L"bFixSlowText", m_bFixSlowText, pIni);
    //m_bAlwaysOnTop = GetPrivateProfileBool(L"settings", L"bAlwaysOnTop", m_bAlwaysOnTop, pIni);
    //m_bWarningsDisabled = GetPrivateProfileBool("settings","bWarningsDisabled",m_bWarningsDisabled, pIni);
    m_bWarningsDisabled2 = GetPrivateProfileBool(L"settings", L"bWarningsDisabled2", m_bWarningsDisabled2, pIni);
    //m_bAnisotropicFiltering = GetPrivateProfileBool(L"settings", L"bAnisotropicFiltering", m_bAnisotropicFiltering, pIni);
    m_bPresetLockOnAtStartup = GetPrivateProfileBool(L"settings", L"bPresetLockOnAtStartup", m_bPresetLockOnAtStartup, pIni);
    m_bPreventScollLockHandling = GetPrivateProfileBool(L"settings", L"m_bPreventScollLockHandling", m_bPreventScollLockHandling, pIni);

    m_nCanvasStretch = GetPrivateProfileInt(L"settings", L"nCanvasStretch", m_nCanvasStretch, pIni);
    m_nTexSizeX = GetPrivateProfileInt(L"settings", L"nTexSize", m_nTexSizeX, pIni);
    m_nTexSizeY = m_nTexSizeX;
    m_bTexSizeWasAutoPow2 = (m_nTexSizeX == -2);
    m_bTexSizeWasAutoExact = (m_nTexSizeX == -1);
    m_nTexBitsPerCh = GetPrivateProfileInt(L"settings", L"nTexBitsPerCh", m_nTexBitsPerCh, pIni);
    m_nGridX = GetPrivateProfileInt(L"settings", L"nMeshSize", m_nGridX, pIni);
    m_nGridY = m_nGridX * 3 / 4;
    m_nMaxPSVersion_ConfigPanel = GetPrivateProfileInt(L"settings", L"MaxPSVersion", m_nMaxPSVersion_ConfigPanel, pIni);
    m_nMaxImages = GetPrivateProfileInt(L"settings", L"MaxImages", m_nMaxImages, pIni);
    m_nMaxBytes = GetPrivateProfileInt(L"settings", L"MaxBytes", m_nMaxBytes, pIni);

    m_fBlendTimeUser = GetPrivateProfileFloat(L"settings", L"fBlendTimeUser", m_fBlendTimeUser, pIni);
    m_fBlendTimeAuto = GetPrivateProfileFloat(L"settings", L"fBlendTimeAuto", m_fBlendTimeAuto, pIni);
    m_fTimeBetweenPresets = GetPrivateProfileFloat(L"settings", L"fTimeBetweenPresets", m_fTimeBetweenPresets, pIni);
    m_fTimeBetweenPresetsRand = GetPrivateProfileFloat(L"settings", L"fTimeBetweenPresetsRand", m_fTimeBetweenPresetsRand, pIni);
    m_fHardCutLoudnessThresh = GetPrivateProfileFloat(L"settings", L"fHardCutLoudnessThresh", m_fHardCutLoudnessThresh, pIni);
    m_fHardCutHalflife = GetPrivateProfileFloat(L"settings", L"fHardCutHalflife", m_fHardCutHalflife, pIni);
    m_fSongTitleAnimDuration = GetPrivateProfileFloat(L"settings", L"fSongTitleAnimDuration", m_fSongTitleAnimDuration, pIni);
    m_fTimeBetweenRandomSongTitles = GetPrivateProfileFloat(L"settings", L"fTimeBetweenRandomSongTitles", m_fTimeBetweenRandomSongTitles, pIni);
    m_fTimeBetweenRandomCustomMsgs = GetPrivateProfileFloat(L"settings", L"fTimeBetweenRandomCustomMsgs", m_fTimeBetweenRandomCustomMsgs, pIni);

    GetPrivateProfileString(L"settings", L"szPresetDir", m_szPresetDir, m_szPresetDir, sizeof(m_szPresetDir), pIni);
#endif

    ReadCustomMessages();

    m_nTexSizeY = m_nTexSizeX;
    m_bTexSizeWasAutoPow2 = (m_nTexSizeX == -2);
    m_bTexSizeWasAutoExact = (m_nTexSizeX == -1);
    m_nGridY = m_nGridX * 3 / 4;

    // Bounds checking.
    if (m_nGridX > MAX_GRID_X)
        m_nGridX = MAX_GRID_X;
    if (m_nGridY > MAX_GRID_Y)
        m_nGridY = MAX_GRID_Y;
    if (m_fTimeBetweenPresetsRand < 0)
        m_fTimeBetweenPresetsRand = 0;
    if (m_fTimeBetweenPresets < 0.1f)
        m_fTimeBetweenPresets = 0.1f;

    // DERIVED SETTINGS
    m_bPresetLockedByUser = m_bPresetLockOnAtStartup;
    //m_bMilkdropScrollLockState = m_bPresetLockOnAtStartup;
}

// Write the user's settings to the .INI file.
// This gets called only when the user runs the config panel and hits OK.
// If you've added any controls to the config panel, write their value out
// to the .INI file here.
void CPlugin::MilkDropWriteConfig()
{
#ifndef _FOOBAR
    // Use this function           declared in   to write a value of this type
    // -----------------           -----------   -----------------------------
    // WritePrivateProfileInt      utility.h     int
    // WritePrivateProfileInt      utility.h     bool
    // WritePrivateProfileFloat    utility.h     float
    // WritePrivateProfileString   WinBase.h     string

    wchar_t* pIni = GetConfigIniFile();

    // Constants.
    WritePrivateProfileString(L"settings", L"bConfigured", L"1", pIni);

    // Note: `m_szPresetDir` is not written here; it is written manually, whenever it changes.

    wchar_t szSectionName[] = L"settings";

    WritePrivateProfileInt(m_bSongTitleAnims, L"bSongTitleAnims", pIni, L"settings");
    WritePrivateProfileInt(m_bHardCutsDisabled, L"bHardCutsDisabled", pIni, L"settings");
    WritePrivateProfileInt(m_bEnableRating, L"bEnableRating", pIni, L"settings");
    WritePrivateProfileInt(g_bDebugOutput, L"bDebugOutput", pIni, L"settings");

    //WritePrivateProfileInt(m_bShowPresetInfo, "bShowPresetInfo", pIni, "settings");
    //WritePrivateProfileInt(m_bShowSongInfo, "bShowSongInfo", pIni, "settings");
    //WritePrivateProfileInt(m_bFixPinkBug, "bFixPinkBug", pIni, "settings");

    WritePrivateProfileInt(m_bShowPressF1ForHelp, L"bShowPressF1ForHelp", pIni, L"settings");
    //WritePrivateProfileInt(m_bShowMenuToolTips, "bShowMenuToolTips", pIni, "settings");
    WritePrivateProfileInt(m_n16BitGamma, L"n16BitGamma", pIni, L"settings");
    WritePrivateProfileInt(m_bAutoGamma, L"bAutoGamma", pIni, L"settings");

    //WritePrivateProfileInt(m_bAlways3D, "bAlways3D", pIni, "settings");
    //WritePrivateProfileFloat(m_fStereoSep, "fStereoSep", pIni, "settings");
    //WritePrivateProfileInt(m_bFixSlowText, "bFixSlowText", pIni, "settings");
    //itePrivateProfileInt(m_bAlwaysOnTop, "bAlwaysOnTop", pIni, "settings");
    //WritePrivateProfileInt(m_bWarningsDisabled, "bWarningsDisabled", pIni, "settings");
    WritePrivateProfileInt(m_bWarningsDisabled2, L"bWarningsDisabled2", pIni, L"settings");
    //WritePrivateProfileInt(m_bAnisotropicFiltering, "bAnisotropicFiltering", pIni, "settings");
    WritePrivateProfileInt(m_bPresetLockOnAtStartup, L"bPresetLockOnAtStartup", pIni, L"settings");
    WritePrivateProfileInt(m_bPreventScollLockHandling, L"m_bPreventScollLockHandling", pIni, L"settings");
    // note: this is also written at exit of the visualizer

    WritePrivateProfileInt(m_nCanvasStretch, L"nCanvasStretch", pIni, L"settings");
    WritePrivateProfileInt(m_nTexSizeX, L"nTexSize", pIni, L"settings");
    WritePrivateProfileInt(m_nTexBitsPerCh, L"nTexBitsPerCh", pIni, L"settings");
    WritePrivateProfileInt(m_nGridX, L"nMeshSize", pIni, L"settings");
    WritePrivateProfileInt(m_nMaxPSVersion_ConfigPanel, L"MaxPSVersion", pIni, L"settings");
    WritePrivateProfileInt(m_nMaxImages, L"MaxImages", pIni, L"settings");
    WritePrivateProfileInt(m_nMaxBytes, L"MaxBytes", pIni, L"settings");

    WritePrivateProfileFloat(m_fBlendTimeAuto, L"fBlendTimeAuto", pIni, L"settings");
    WritePrivateProfileFloat(m_fBlendTimeUser, L"fBlendTimeUser", pIni, L"settings");
    WritePrivateProfileFloat(m_fTimeBetweenPresets, L"fTimeBetweenPresets", pIni, L"settings");
    WritePrivateProfileFloat(m_fTimeBetweenPresetsRand, L"fTimeBetweenPresetsRand", pIni, L"settings");
    WritePrivateProfileFloat(m_fHardCutLoudnessThresh, L"fHardCutLoudnessThresh", pIni, L"settings");
    WritePrivateProfileFloat(m_fHardCutHalflife, L"fHardCutHalflife", pIni, L"settings");
    WritePrivateProfileFloat(m_fSongTitleAnimDuration, L"fSongTitleAnimDuration", pIni, L"settings");
    WritePrivateProfileFloat(m_fTimeBetweenRandomSongTitles, L"fTimeBetweenRandomSongTitles", pIni, L"settings");
    WritePrivateProfileFloat(m_fTimeBetweenRandomCustomMsgs, L"fTimeBetweenRandomCustomMsgs", pIni, L"settings");
#endif
}

#ifdef _FOOBAR
bool CPlugin::PanelSettings(plugin_config* settings)
{
    // CPluginShell::ReadConfig()
    m_multisample_fs = {settings->m_multisample_fs.Count, settings->m_multisample_fs.Quality};
    //m_multisample_w = {settings->m_multisample_w.Count, 0U};

    //m_start_fullscreen = settings->m_start_fullscreen;
    m_max_fps_fs = settings->m_max_fps_fs;
    m_show_press_f1_msg = settings->m_show_press_f1_msg;
    m_allow_page_tearing_fs = settings->m_allow_page_tearing_fs;
    //m_minimize_winamp = settings->m_minimize_winamp;
    //m_dualhead_horz = settings->m_dualhead_horz;
    //m_dualhead_vert = settings->m_dualhead_vert;
    //m_save_cpu = settings->m_save_cpu;
    //m_skin = settings->m_skin;
    //m_fix_slow_text = settings->m_fix_slow_text;

    // CPlugin::MilkDropReadConfig()
    m_bEnableRating = settings->m_bEnableRating;
    m_bHardCutsDisabled = settings->m_bHardCutsDisabled;
    g_bDebugOutput = settings->g_bDebugOutput;
    //m_bShowSongInfo = settings->m_bShowSongInfo;
    //m_bShowPressF1ForHelp = settings->m_bShowPressF1ForHelp;
    //m_bShowMenuToolTips = settings->m_bShowMenuToolTips;
    m_bSongTitleAnims = settings->m_bSongTitleAnims;

    m_bShowFPS = settings->m_bShowFPS;
    m_bShowRating = settings->m_bShowRating;
    m_bShowPresetInfo = settings->m_bShowPresetInfo;
    //m_bShowDebugInfo = settings->m_bShowDebugInfo;
    m_bShowSongTitle = settings->m_bShowSongTitle;
    m_bShowSongTime = settings->m_bShowSongTime;
    m_bShowSongLen = settings->m_bShowSongLen;
    m_bShowShaderHelp = settings->m_bShowShaderHelp;

    //m_bFixPinkBug = settings->m_bFixPinkBug;
    m_n16BitGamma = settings->m_n16BitGamma;
    m_bAutoGamma = settings->m_bAutoGamma;
    //m_bAlways3D = settings->m_bAlways3D;
    //m_fStereoSep = settings->m_fStereoSep;
    //m_bFixSlowText = settings->m_bFixSlowText;
    //m_bAlwaysOnTop = settings->m_bAlwaysOnTop;
    //m_bWarningsDisabled = settings->m_bWarningsDisabled;
    m_bWarningsDisabled2 = settings->m_bWarningsDisabled2;
    //m_bAnisotropicFiltering = settings->m_bAnisotropicFiltering;
    m_bPresetLockOnAtStartup = settings->m_bPresetLockOnAtStartup;
    m_bPreventScollLockHandling = settings->m_bPreventScollLockHandling;

    m_nCanvasStretch = settings->m_nCanvasStretch;
    m_nTexSizeX = settings->m_nTexSizeX;
    m_nTexSizeY = settings->m_nTexSizeY;
    m_bTexSizeWasAutoPow2 = settings->m_bTexSizeWasAutoPow2;
    m_bTexSizeWasAutoExact = settings->m_bTexSizeWasAutoExact;
    m_nTexBitsPerCh = settings->m_nTexBitsPerCh;
    m_nGridX = settings->m_nGridX;
    m_nGridY = settings->m_nGridY;
    m_nMaxPSVersion_ConfigPanel = settings->m_nMaxPSVersion;
    m_nMaxImages = settings->m_nMaxImages;
    m_nMaxBytes = settings->m_nMaxBytes;

    m_fBlendTimeUser = settings->m_fBlendTimeUser;
    m_fBlendTimeAuto = settings->m_fBlendTimeAuto;
    m_fTimeBetweenPresets = settings->m_fTimeBetweenPresets;
    m_fTimeBetweenPresetsRand = settings->m_fTimeBetweenPresetsRand;
    m_fHardCutLoudnessThresh = settings->m_fHardCutLoudnessThresh;
    m_fHardCutHalflife = settings->m_fHardCutHalflife;
    m_fSongTitleAnimDuration = settings->m_fSongTitleAnimDuration;
    m_fTimeBetweenRandomSongTitles = settings->m_fTimeBetweenRandomSongTitles;
    m_fTimeBetweenRandomCustomMsgs = settings->m_fTimeBetweenRandomCustomMsgs;

    m_bPresetLockedByUser = settings->m_bPresetLockedByUser;
    //m_bMilkdropScrollLockState = settings->m_bMilkdropScrollLockState;

    m_enable_downmix = static_cast<int>(settings->m_bEnableDownmix);
    m_show_album = static_cast<int>(settings->m_bShowAlbum);
    m_enable_hdr = static_cast<int>(settings->m_bEnableHDR);
    m_back_buffer_format = settings->m_nBackBufferFormat;
    m_depth_buffer_format = settings->m_nDepthBufferFormat;
    m_back_buffer_count = settings->m_nBackBufferCount;
    m_min_feature_level = settings->m_nMinFeatureLevel;
    m_skip_comp_shaders = settings->m_bSkipCompShader;

    wcscpy_s(m_szPresetDir, settings->m_szPresetDir);
    //wcscpy_s(m_szConfigIniFile, settings->m_szConfigIniFile);
    wcscpy_s(m_szMsgIniFile, settings->m_szMsgIniFile);
    wcscpy_s(m_szImgIniFile, settings->m_szImgIniFile);

    memcpy_s(m_fontinfo, sizeof(m_fontinfo), settings->m_fontinfo, sizeof(settings->m_fontinfo));

    return true;
}
#endif

//----------------------------------------------------------------------

void StripComments(char* str)
{
    if (!str || !str[0] || !str[1])
        return;

    char c0 = str[0];
    char c1 = str[1];
    char* dest = str;
    char* p = &str[1];
    bool bIgnoreTilEndOfLine = false;
    bool bIgnoreTilCloseComment = false; //this one takes precedence
    int nCharsToSkip = 0;
    while (1)
    {
        // Handle "//" comments.
        if (!bIgnoreTilCloseComment && c0 == '/' && c1 == '/')
            bIgnoreTilEndOfLine = true;
        if (bIgnoreTilEndOfLine && (c0 == '\n' || c0 == '\r'))
        {
            bIgnoreTilEndOfLine = false;
            nCharsToSkip = 0;
        }

        // Handle "/* */" comments.
        if (!bIgnoreTilEndOfLine && c0 == '/' && c1 == '*')
            bIgnoreTilCloseComment = true;
        if (bIgnoreTilCloseComment && c0 == '*' && c1 == '/')
        {
            bIgnoreTilCloseComment = false;
            nCharsToSkip = 2;
        }

        if (!bIgnoreTilEndOfLine && !bIgnoreTilCloseComment)
        {
            if (nCharsToSkip > 0)
                nCharsToSkip--;
            else
                *dest++ = c0;
        }

        if (c1 == '\0')
            break;

        p++;
        c0 = c1;
        c1 = *p;
    }

    *dest++ = '\0';
}

// This gets called only once, when your plugin is actually launched.
// If only the config panel is launched, this does NOT get called.
// (whereas `MilkDropPreInitialize()` still does).
// If anything fails here, return FALSE to safely exit the plugin,
// but only after displaying a message box giving the user some information
// about what went wrong.
int CPlugin::AllocateMilkDropNonDX11()
{
    /*   
    if (!m_hBlackBrush)
        m_hBlackBrush = CreateSolidBrush(RGB(0,0,0));
    */

    g_hThread = INVALID_HANDLE_VALUE;
    g_bThreadAlive = false;
    g_bThreadShouldQuit = false;
    InitializeCriticalSection(&g_cs);

    // Read in `m_szShaderIncludeText`.
    // clang-format off
    bool bSuccess = true;
    bSuccess = ReadFileToString(L"data\\include.fx", m_szShaderIncludeText, sizeof(m_szShaderIncludeText) - 4, false);
    if (!bSuccess) return false;
    StripComments(m_szShaderIncludeText);
    m_nShaderIncludeTextLen = strlen(m_szShaderIncludeText);

    bSuccess |= ReadFileToString(L"data\\warp_vs.fx", m_szDefaultWarpVShaderText, sizeof(m_szDefaultWarpVShaderText), true);
    if (!bSuccess) return false;
    bSuccess |= ReadFileToString(L"data\\warp_ps.fx", m_szDefaultWarpPShaderText, sizeof(m_szDefaultWarpPShaderText), true);
    if (!bSuccess) return false;
    bSuccess |= ReadFileToString(L"data\\comp_vs.fx", m_szDefaultCompVShaderText, sizeof(m_szDefaultCompVShaderText), true);
    if (!bSuccess) return false;
    bSuccess |= ReadFileToString(L"data\\comp_ps.fx", m_szDefaultCompPShaderText, sizeof(m_szDefaultCompPShaderText), true);
    if (!bSuccess) return false;
    bSuccess |= ReadFileToString(L"data\\blur_vs.fx", m_szBlurVS, sizeof(m_szBlurVS), true);
    if (!bSuccess) return false;
    bSuccess |= ReadFileToString(L"data\\blur1_ps.fx", m_szBlurPSX, sizeof(m_szBlurPSX), true);
    if (!bSuccess) return false;
    bSuccess |= ReadFileToString(L"data\\blur2_ps.fx", m_szBlurPSY, sizeof(m_szBlurPSY), true);
    if (!bSuccess) return false;
    // clang-format on

    BuildMenus();

    m_pState->Default();
    m_pOldState->Default();
    m_pNewState->Default();

    //LoadRandomPreset(0.0f); -avoid this here; causes some DX9 stuff to happen.

    return true;
}

void CancelThread(int max_wait_time_ms)
{
    g_bThreadShouldQuit = true;
    int waited = 0;
    while (g_bThreadAlive && waited < max_wait_time_ms)
    {
        Sleep(30);
        waited += 30;
    }

    if (g_bThreadAlive)
    {
#ifdef TARGET_WINDOWS_DESKTOP
        TerminateThread(g_hThread, 0);
#endif
        g_bThreadAlive = false;
    }

    if (g_hThread != INVALID_HANDLE_VALUE)
        CloseHandle(g_hThread);
    g_hThread = INVALID_HANDLE_VALUE;
}

// This gets called only once, when the plugin exits.
// Clean up any objects here that were
// created or initialized in `AllocateMilkDropNonDX11()`.
void CPlugin::CleanUpMilkDropNonDX11()
{
    DeleteCriticalSection(&g_cs);

    CancelThread(0);

    m_menuPreset.Finish();
    m_menuWave.Finish();
    m_menuAugment.Finish();
    m_menuCustomWave.Finish();
    m_menuCustomShape.Finish();
    m_menuMotion.Finish();
    m_menuPost.Finish();
    for (int i = 0; i < MAX_CUSTOM_WAVES; i++)
        m_menuWavecode[i].Finish();
    for (int i = 0; i < MAX_CUSTOM_SHAPES; i++)
        m_menuShapecode[i].Finish();

    SetScrollLock(m_bOrigScrollLockState, m_bPreventScollLockHandling);

    //DumpDebugMessage("Finish: cleanup complete.");
}

float SquishToCenter(float x, float fExp)
{
    if (x > 0.5f)
        return powf(x * 2 - 1, fExp) * 0.5f + 0.5f;

    return (1 - powf(1 - x * 2, fExp)) * 0.5f;
}

int GetNearestPow2Size(int w, int h)
{
    float fExp = logf(std::max(w, h) * 0.75f + 0.25f * std::min(w, h)) / logf(2.0f);
    float bias = 0.55f;
    if (fExp + bias >= 11.0f) // ..don't jump to 2048x2048 quite as readily
        bias = 0.5f;
    int nExp = (int)(fExp + bias);
    int log2size = (int)powf(2.0f, (float)nExp);
    return log2size;
}

// Allocate and initialize all the DX11 stuff here: textures,
// surfaces, vertex/index buffers, fonts, and so on.
// If anything fails here, return FALSE to safely exit the plugin,
// but only after displaying a messagebox giving the user some information
// about what went wrong.  If the error is NON-CRITICAL, you don't *have*
// to return; just make sure that the rest of the code will be still safely
// run (albeit with degraded features).
// If you run out of video memory, you might want to show a short messagebox
// saying what failed to allocate and that the reason is a lack of video
// memory, and then call `SuggestHowToFreeSomeMem()`, which will show them
// a *second* messagebox that (intelligently) suggests how they can free up
// some video memory.
// Don't forget to account for each object created or allocated here by cleaning
// it up in `CleanUpMilkDropDX11()`!
// IMPORTANT:
// Note that the code here isn't just run at program startup!
// When the user toggles between fullscreen and windowed modes
// or resizes the window, the base class calls this function before
// destroying & recreating the plugin window and DirectX object, and then
// calls `AllocateMilkDropDX11()` afterwards, to get your plugin running again.
// (...aka OnUserResizeWindow)
// (...aka OnToggleFullscreen)
int CPlugin::AllocateMilkDropDX11()
{
    //wchar_t buf[32768], title[64];

    m_nFramesSinceResize = 0;

    int nNewCanvasStretch = (m_nCanvasStretch == 0) ? 100 : m_nCanvasStretch;

    D3D_FEATURE_LEVEL featureLevel = GetDevice()->GetFeatureLevel();
    if (featureLevel >= D3D_FEATURE_LEVEL_11_0)
        m_nMaxPSVersion_DX = MD2_PS_5_0;
    if (featureLevel >= D3D_FEATURE_LEVEL_10_0)
        m_nMaxPSVersion_DX = MD2_PS_4_0;
    else if (featureLevel >= D3D_FEATURE_LEVEL_9_3)
        m_nMaxPSVersion_DX = MD2_PS_2_X;
    else if (featureLevel >= D3D_FEATURE_LEVEL_9_1)
        m_nMaxPSVersion_DX = MD2_PS_2_0;
    else
        m_nMaxPSVersion_DX = MD2_PS_NONE;

    if (m_nMaxPSVersion_ConfigPanel == -1)
        m_nMaxPSVersion = m_nMaxPSVersion_DX;
    else
    {
        // To limit user choice by what hardware reports.
        //m_nMaxPSVersion = std::min(m_nMaxPSVersion_DX, m_nMaxPSVersion_ConfigPanel);

        // To allow users to override.
        m_nMaxPSVersion = m_nMaxPSVersion_ConfigPanel;
    }

    // SHADERS
    // GREY LIST (slow ps_2_0 cards) and BLACK LIST (bad ps_2_0 support)
    // not needed for DirectX 11.1.
    //------------------------------------------------------------------
    if (m_nMaxPSVersion > MD2_PS_NONE)
    {
        /* DX11: vertex declarations not required. D3D11Shim uses needed layout.
        // Create vertex declarations (since not using FVF anymore).
        if (D3D_OK != GetDevice()->CreateVertexDeclaration(g_MilkDropVertDecl, &m_pMilkDropVertDecl))
        {
            //WASABI_API_LNGSTRINGW_BUF(IDS_COULD_NOT_CREATE_MD_VERTEX_DECLARATION, buf, sizeof(buf));
            //DumpDebugMessage(buf);
            //MessageBox(GetPluginWindow(), buf, WASABI_API_LNGSTRINGW_BUF(IDS_MILKDROP_ERROR, title, sizeof(title)), MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
            return false;
        }
        if (D3D_OK != GetDevice()->CreateVertexDeclaration(g_WfVertDecl, &m_pWfVertDecl))
        {
            //WASABI_API_LNGSTRINGW_BUF(IDS_COULD_NOT_CREATE_WF_VERTEX_DECLARATION, buf, sizeof(buf));
            //DumpDebugMessage(buf);
            //MessageBox(GetPluginWindow(), buf, WASABI_API_LNGSTRINGW_BUF(IDS_MILKDROP_ERROR, title, sizeof(title)), MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
            return false;
        }
        if (D3D_OK != GetDevice()->CreateVertexDeclaration(g_SpriteVertDecl, &m_pSpriteVertDecl))
        {
            //WASABI_API_LNGSTRINGW_BUF(IDS_COULD_NOT_CREATE_SPRITE_VERTEX_DECLARATION, buf, sizeof(buf));
            //DumpDebugMessage(buf);
            //MessageBox(GetPluginWindow(), buf, WASABI_API_LNGSTRINGW_BUF(IDS_MILKDROP_ERROR, title, sizeof(title)), MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
            return false;
        }*/

        // Load the FALLBACK shaders...
        if (!RecompilePShader(m_szDefaultWarpPShaderText, &m_fallbackShaders_ps.warp, SHADER_WARP, true, 2))
        {
            /*
            wchar_t szSM[64];
            switch(m_nMaxPSVersion_DX)
            {
            case MD2_PS_2_0:
            case MD2_PS_2_X:
                WASABI_API_LNGSTRINGW_BUF(IDS_SHADER_MODEL_2, szSM, 64); break;
            case MD2_PS_3_0: WASABI_API_LNGSTRINGW_BUF(IDS_SHADER_MODEL_3, szSM, 64); break;
            case MD2_PS_4_0: WASABI_API_LNGSTRINGW_BUF(IDS_SHADER_MODEL_4, szSM, 64); break;
            default:
                swprintf_s(szSM, WASABI_API_LNGSTRINGW(IDS_UKNOWN_CASE_X), m_nMaxPSVersion_DX);
                break;
            }
            if (m_nMaxPSVersion_ConfigPanel >= MD2_PS_NONE && m_nMaxPSVersion_DX < m_nMaxPSVersion_ConfigPanel)
                swprintf_s(buf, WASABI_API_LNGSTRINGW(IDS_FAILED_TO_COMPILE_PIXEL_SHADERS_USING_X), szSM, PSVersion);
            else
                swprintf_s(buf, WASABI_API_LNGSTRINGW(IDS_FAILED_TO_COMPILE_PIXEL_SHADERS_HARDWARE_MIS_REPORT), szSM, PSVersion);
            DumpDebugMessage(buf);
            MessageBox(GetPluginWindow(), buf, WASABI_API_LNGSTRINGW_BUF(IDS_MILKDROP_ERROR, title, 64), MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
            */
            return false;
        }
        if (!RecompileVShader(m_szDefaultWarpVShaderText, &m_fallbackShaders_vs.warp, SHADER_WARP, true))
        {
            /*
            WASABI_API_LNGSTRINGW_BUF(IDS_COULD_NOT_COMPILE_FALLBACK_WV_SHADER, buf, sizeof(buf));
            DumpDebugMessage(buf);
            MessageBox(GetPluginWindow(), buf, WASABI_API_LNGSTRINGW_BUF(IDS_MILKDROP_ERROR, title, 64), MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
            */
            return false;
        }
        if (!RecompileVShader(m_szDefaultCompVShaderText, &m_fallbackShaders_vs.comp, SHADER_COMP, true))
        {
            /*
            WASABI_API_LNGSTRINGW_BUF(IDS_COULD_NOT_COMPILE_FALLBACK_CV_SHADER, buf, sizeof(buf));
            DumpDebugMessage(buf);
            MessageBox(GetPluginWindow(), buf, WASABI_API_LNGSTRINGW_BUF(IDS_MILKDROP_ERROR, title, 64), MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
            */
            return false;
        }
        if (!RecompilePShader(m_szDefaultCompPShaderText, &m_fallbackShaders_ps.comp, SHADER_COMP, true, 2))
        {
            /*
            WASABI_API_LNGSTRINGW_BUF(IDS_COULD_NOT_COMPILE_FALLBACK_CP_SHADER, buf, sizeof(buf));
            DumpDebugMessage(buf);
            MessageBox(GetPluginWindow(), buf, WASABI_API_LNGSTRINGW_BUF(IDS_MILKDROP_ERROR, title, 64), MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
            */
            return false;
        }

        // Load the BLUR shaders...
        if (!RecompileVShader(m_szBlurVS, &m_BlurShaders[0].vs, SHADER_BLUR, true))
        {
            /*
            WASABI_API_LNGSTRINGW_BUF(IDS_COULD_NOT_COMPILE_BLUR1_VERTEX_SHADER, buf, sizeof(buf));
            DumpDebugMessage(buf);
            MessageBox(GetPluginWindow(), buf, WASABI_API_LNGSTRINGW_BUF(IDS_MILKDROP_ERROR, title, 64), MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
            */
            return false;
        }
        if (!RecompilePShader(m_szBlurPSX, &m_BlurShaders[0].ps, SHADER_BLUR, true, 2))
        {
            /*
            WASABI_API_LNGSTRINGW_BUF(IDS_COULD_NOT_COMPILE_BLUR1_PIXEL_SHADER, buf, sizeof(buf));
            DumpDebugMessage(buf);
            MessageBox(GetPluginWindow(), buf, WASABI_API_LNGSTRINGW_BUF(IDS_MILKDROP_ERROR, title, 64), MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
            */
            return false;
        }
        if (!RecompileVShader(m_szBlurVS, &m_BlurShaders[1].vs, SHADER_BLUR, true))
        {
            /*
            WASABI_API_LNGSTRINGW_BUF(IDS_COULD_NOT_COMPILE_BLUR2_VERTEX_SHADER, buf, sizeof(buf));
            DumpDebugMessage(buf);
            MessageBox(GetPluginWindow(), buf, WASABI_API_LNGSTRINGW_BUF(IDS_MILKDROP_ERROR, title, 64), MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
            */
            return false;
        }
        if (!RecompilePShader(m_szBlurPSY, &m_BlurShaders[1].ps, SHADER_BLUR, true, 2))
        {
            /*
            WASABI_API_LNGSTRINGW_BUF(IDS_COULD_NOT_COMPILE_BLUR2_PIXEL_SHADER, buf, sizeof(buf));
            DumpDebugMessage(buf);
            MessageBox(GetPluginWindow(), buf, WASABI_API_LNGSTRINGW_BUF(IDS_MILKDROP_ERROR, title, 64), MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
            */
            return false;
        }
    }

    // Create `m_lpVS[2]`.
    {
        int log2texsize = GetNearestPow2Size(GetWidth(), GetHeight());

        // Auto-guess texsize.
        if (m_bTexSizeWasAutoExact)
        {
            // Note: In windowed mode, the winamp window could be weird sizes,
            //       so the plugin shell now gives us a slightly enlarged size,
            //       which pads it out to the nearest 32x32 block size,
            //       and then on display, it intelligently crops the image.
            //       This is pretty likely to work on non-shitty GPUs.
            //       but some shitty ones will still only do powers of 2!
            //       So if we are running out of video memory here or experience
            //       other problems, though, we can make our VS's smaller;
            //       which will work, although it will lead to stretching.
            m_nTexSizeX = GetWidth();
            m_nTexSizeY = GetHeight();
        }
        else if (m_bTexSizeWasAutoPow2)
        {
            m_nTexSizeX = log2texsize;
            m_nTexSizeY = log2texsize;
        }

        // Apply canvas stretch.
        m_nTexSizeX = (m_nTexSizeX * 100) / nNewCanvasStretch;
        m_nTexSizeY = (m_nTexSizeY * 100) / nNewCanvasStretch;

        // Re-compute closest power-of-2 size, now that we've factored in the stretching...
        log2texsize = GetNearestPow2Size(m_nTexSizeX, m_nTexSizeY);
        if (m_bTexSizeWasAutoPow2)
        {
            m_nTexSizeX = log2texsize;
            m_nTexSizeY = log2texsize;
        }

        // Snap to 16x16 blocks.
        // TODO: DX11 or use own zBuffer.

        // Determine format for VS1/VS2.
        DXGI_FORMAT fmt;
        switch (m_nTexBitsPerCh)
        {
            case 8: fmt = DXGI_FORMAT_B8G8R8A8_UNORM; break;
            default: fmt = DXGI_FORMAT_B8G8R8A8_UNORM; break;
        }

        // Reallocate.
        bool bSuccess = false;
        //DWORD vs_flags = D3DUSAGE_RENDERTARGET;// | D3DUSAGE_AUTOGENMIPMAP;//FIXME! (make automipgen optional)
        DWORD vs_flags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE; // | D3DUSAGE_AUTOGENMIPMAP;//FIXME! (make automipgen optional)
        //bool bRevertedBitDepth = false;
        do
        {
            SafeRelease(m_lpVS[0]);
            SafeRelease(m_lpVS[1]);

            // Create VS1.
            bSuccess = (GetDevice()->CreateTexture(m_nTexSizeX, m_nTexSizeY, 1, vs_flags, fmt, &m_lpVS[0]));
            if (!bSuccess)
            {
                bSuccess = (GetDevice()->CreateTexture(m_nTexSizeX, m_nTexSizeY, 1, vs_flags, DXGI_FORMAT_B8G8R8A8_UNORM, &m_lpVS[0]));
                if (bSuccess)
                    fmt = DXGI_FORMAT_B8G8R8A8_UNORM /* TODO: DX11 GetBackBufFormat() */;
            }

            // Create VS2
            if (bSuccess)
                bSuccess = (GetDevice()->CreateTexture(m_nTexSizeX, m_nTexSizeY, 1, vs_flags, fmt, &m_lpVS[1]));

            if (!bSuccess)
            {
                if (m_bTexSizeWasAutoExact)
                {
                    if (m_nTexSizeX > 256 || m_nTexSizeY > 256)
                    {
                        m_nTexSizeX /= 2;
                        m_nTexSizeY /= 2;
                        m_nTexSizeX = ((m_nTexSizeX + 15) / 16) * 16;
                        m_nTexSizeY = ((m_nTexSizeY + 15) / 16) * 16;
                    }
                    else
                    {
                        m_nTexSizeX = log2texsize;
                        m_nTexSizeY = log2texsize;
                        m_bTexSizeWasAutoExact = false;
                        m_bTexSizeWasAutoPow2 = true;
                    }
                }
                else if (m_bTexSizeWasAutoPow2)
                {
                    if (m_nTexSizeX > 256)
                    {
                        m_nTexSizeX /= 2;
                        m_nTexSizeY /= 2;
                    }
                    else
                        break;
                }
            }
        } while (!bSuccess); // && m_nTexSizeX >= 256 && (m_bTexSizeWasAutoExact || m_bTexSizeWasAutoPow2));

        if (!bSuccess)
        {
            /*
            wchar_t buf[2048];
            UINT err_id = IDS_COULD_NOT_CREATE_INTERNAL_CANVAS_TEXTURE_NOT_ENOUGH_VID_MEM;
            if (GetScreenMode() == FULLSCREEN)
                err_id = IDS_COULD_NOT_CREATE_INTERNAL_CANVAS_TEXTURE_SMALLER_DISPLAY;
            else if (!(m_bTexSizeWasAutoExact || m_bTexSizeWasAutoPow2))
                err_id = IDS_COULD_NOT_CREATE_INTERNAL_CANVAS_TEXTURE_NOT_ENOUGH_VID_MEM_RECOMMENDATION;

            WASABI_API_LNGSTRINGW_BUF(err_id, buf, sizeof(buf));
            DumpDebugMessage(buf);
            MessageBox(GetPluginWindow(), buf, WASABI_API_LNGSTRINGW_BUF(IDS_MILKDROP_ERROR, title, 64), MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
            */
            return false;
        }
        else
        {
            //swprintf_s(buf, WASABI_API_LNGSTRINGW(IDS_SUCCESSFULLY_CREATED_VS0_VS1), m_nTexSizeX, m_nTexSizeY, GetWidth(), GetHeight());
            //DumpDebugMessage(buf);
        }

        /*
        if (m_nTexSizeX != GetWidth() || m_nTexSizeY != GetHeight())
        {
            swprintf_s(buf, "warning - canvas size adjusted from %dx%d to %dx%d.", GetWidth(), GetHeight(), m_nTexSizeX, m_nTexSizeY);
            DumpDebugMessage(buf);
            AddError(buf, 3.2f, ERR_INIT, true);
        }
        */

        // Create blur textures with same format. A complete MIP chain costs 33% more video memory than 1 full-sized VS.
#if (NUM_BLUR_TEX > 0)
        int w = m_nTexSizeX;
        int h = m_nTexSizeY;
        DWORD blurtex_flags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        for (int i = 0; i < NUM_BLUR_TEX; i++)
        {
            // Main VS = 1024
            // blur0 = 512
            // blur1 = 256  <-  user sees this as "blur1"
            // blur2 = 128
            // blur3 = 128  <-  user sees this as "blur2"
            // blur4 =  64
            // blur5 =  64  <-  user sees this as "blur3"
            if (!(i & 1) || (i < 2))
            {
                w = std::max(16, w / 2);
                h = std::max(16, h / 2);
            }
            int w2 = ((w + 3) / 16) * 16;
            int h2 = ((h + 3) / 4) * 4;
            bSuccess = (GetDevice()->CreateTexture(w2, h2, 1, blurtex_flags, fmt, &m_lpBlur[i]));
            m_nBlurTexW[i] = w2;
            m_nBlurTexH[i] = h2;
            if (!bSuccess)
            {
                m_nBlurTexW[i] = 1;
                m_nBlurTexH[i] = 1;
                /*
                MessageBox(GetPluginWindow(), WASABI_API_LNGSTRINGW_BUF(IDS_ERROR_CREATING_BLUR_TEXTURES, buf, sizeof(buf)),
                WASABI_API_LNGSTRINGW_BUF(IDS_MILKDROP_WARNING, title, sizeof(title)), MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
                */
                break;
            }

            // Add it to `m_textures[]`.
            TexInfo x;
            swprintf_s(x.texname, L"blur%d%s", i / 2 + 1, (i % 2) ? L"" : L"doNOTuseME");
            x.texptr = m_lpBlur[i];
            //x.texsize_param = NULL;
            x.w = w2;
            x.h = h2;
            x.d = 1;
            x.bEvictable = false;
            x.nAge = m_nPresetsLoadedTotal;
            x.nSizeInBytes = 0;
            m_textures.push_back(x);
        }
#endif
    }

    m_fAspectX = (m_nTexSizeY > m_nTexSizeX) ? m_nTexSizeX / (float)m_nTexSizeY : 1.0f;
    m_fAspectY = (m_nTexSizeX > m_nTexSizeY) ? m_nTexSizeY / (float)m_nTexSizeX : 1.0f;
    m_fInvAspectX = 1.0f / m_fAspectX;
    m_fInvAspectY = 1.0f / m_fAspectY;

    // BUILD VERTEX LIST for final composite blit
    // note the +0.5-texel offset!
    // (otherwise, a 1-pixel-wide line of the image would wrap at the top and left edges).
    ZeroMemory(m_comp_verts, sizeof(MDVERTEX) * FCGSX * FCGSY);
    //float fOnePlusInvWidth  = 1.0f + 1.0f / (float)GetWidth();
    //float fOnePlusInvHeight = 1.0f + 1.0f / (float)GetHeight();
    float fHalfTexelW = 0.5f / static_cast<float>(std::max(1, GetWidth())); // 2.5: 2 pixels bad @ bottom right
    float fHalfTexelH = 0.5f / static_cast<float>(std::max(1, GetHeight()));
    float fDivX = 1.0f / (float)(FCGSX - 2);
    float fDivY = 1.0f / (float)(FCGSY - 2);
    for (int j = 0; j < FCGSY; j++)
    {
        int j2 = j - j / (FCGSY / 2);
        float v = j2 * fDivY;
        v = SquishToCenter(v, 3.0f);
        float sy = -((v - fHalfTexelH) * 2 - 1); //fOnePlusInvHeight*v*2-1;
        for (int i = 0; i < FCGSX; i++)
        {
            int i2 = i - i / (FCGSX / 2);
            float u = i2 * fDivX;
            u = SquishToCenter(u, 3.0f);
            float sx = (u - fHalfTexelW) * 2 - 1; //fOnePlusInvWidth*u*2-1;
            MDVERTEX* p = &m_comp_verts[i + j * FCGSX];
            p->x = sx;
            p->y = sy;
            p->z = 0;
            float rad, ang;
            UvToMathSpace(u, v, &rad, &ang);
            // Fix-ups.
            if (i == FCGSX / 2 - 1)
            {
                if (j < FCGSY / 2 - 1)
                    ang = 3.1415926535898f * 1.5f;
                else if (j == FCGSY / 2 - 1)
                    ang = 3.1415926535898f * 1.25f;
                else if (j == FCGSY / 2)
                    ang = 3.1415926535898f * 0.75f;
                else
                    ang = 3.1415926535898f * 0.5f;
            }
            else if (i == FCGSX / 2)
            {
                if (j < FCGSY / 2 - 1)
                    ang = 3.1415926535898f * 1.5f;
                else if (j == FCGSY / 2 - 1)
                    ang = 3.1415926535898f * 1.75f;
                else if (j == FCGSY / 2)
                    ang = 3.1415926535898f * 0.25f;
                else
                    ang = 3.1415926535898f * 0.5f;
            }
            else if (j == FCGSY / 2 - 1)
            {
                if (i < FCGSX / 2 - 1)
                    ang = 3.1415926535898f * 1.0f;
                else if (i == FCGSX / 2 - 1)
                    ang = 3.1415926535898f * 1.25f;
                else if (i == FCGSX / 2)
                    ang = 3.1415926535898f * 1.75f;
                else
                    ang = 3.1415926535898f * 2.0f;
            }
            else if (j == FCGSY / 2)
            {
                if (i < FCGSX / 2 - 1)
                    ang = 3.1415926535898f * 1.0f;
                else if (i == FCGSX / 2 - 1)
                    ang = 3.1415926535898f * 0.75f;
                else if (i == FCGSX / 2)
                    ang = 3.1415926535898f * 0.25f;
                else
                    ang = 3.1415926535898f * 0.0f;
            }
            p->tu = u;
            p->tv = v;
            //p->tu0 = u;
            //p->tv0 = v;
            p->rad = rad;
            p->ang = ang;
            p->a = 1.0f;
            p->r = 1.0f;
            p->g = 1.0f;
            p->b = 1.0f;
        }
    }

    // Build index list for final composite blit.
    // Order should be friendly for interpolation of 'ang' value!
    int* cur_index = &m_comp_indices[0];
    for (int y = 0; y < FCGSY - 1; y++)
    {
        if (y == FCGSY / 2 - 1)
            continue;
        for (int x = 0; x < FCGSX - 1; x++)
        {
            if (x == FCGSX / 2 - 1)
                continue;
            bool left_half = (x < FCGSX / 2);
            bool top_half = (y < FCGSY / 2);
            bool center_4 = ((x == FCGSX / 2 || x == FCGSX / 2 - 1) && (y == FCGSY / 2 || y == FCGSY / 2 - 1));

            if (((int)left_half + (int)top_half + (int)center_4) % 2)
            {
                *(cur_index + 0) = (y) * FCGSX + (x);
                *(cur_index + 1) = (y) * FCGSX + (x + 1);
                *(cur_index + 2) = (y + 1) * FCGSX + (x + 1);
                *(cur_index + 3) = (y + 1) * FCGSX + (x + 1);
                *(cur_index + 4) = (y + 1) * FCGSX + (x);
                *(cur_index + 5) = (y) * FCGSX + (x);
            }
            else
            {
                *(cur_index + 0) = (y + 1) * FCGSX + (x);
                *(cur_index + 1) = (y) * FCGSX + (x);
                *(cur_index + 2) = (y) * FCGSX + (x + 1);
                *(cur_index + 3) = (y) * FCGSX + (x + 1);
                *(cur_index + 4) = (y + 1) * FCGSX + (x + 1);
                *(cur_index + 5) = (y + 1) * FCGSX + (x);
            }
            cur_index += 6;
        }
    }

    /*
    if (m_bFixSlowText && !m_bSeparateTextWindow)
    {
        if (pCreateTexture(GetDevice(), GetWidth(), GetHeight(), 1, D3DUSAGE_RENDERTARGET, GetBackBufFormat(), D3DPOOL_DEFAULT, &m_lpDDSText) != D3D_OK)
        {
            char buf[2048];
            DumpDebugMessage("Init: -WARNING-:");
            sprintf(buf, "WARNING: Not enough video memory to make a dedicated text surface; \rtext will still be drawn directly to the back buffer.\r\rTo avoid seeing this error again, uncheck the 'fix slow text' option.");
            DumpDebugMessage(buf);
            if (!m_bWarningsDisabled)
                MessageBox(GetPluginWindow(), buf, "WARNING", MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
            m_lpDDSText = NULL;
        }
    }
    */

    // Reallocate the texture for font titles and custom messages (`m_lpDDSTitle`).
    {
        m_nTitleTexSizeX = std::max(m_nTexSizeX, m_nTexSizeY);
        m_nTitleTexSizeY = m_nTitleTexSizeX / 4;

        bool bSuccess;
        do
        {
            bSuccess = GetDevice()->CreateTexture(m_nTitleTexSizeX, m_nTitleTexSizeY, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, DXGI_FORMAT_B8G8R8A8_UNORM, &m_lpDDSTitle);
            if (!bSuccess)
            {
                if (m_nTitleTexSizeY < m_nTitleTexSizeX)
                {
                    m_nTitleTexSizeY *= 2;
                }
                else
                {
                    m_nTitleTexSizeX /= 2;
                    m_nTitleTexSizeY /= 2;
                }
            }
        } while (!bSuccess && m_nTitleTexSizeX > 16);

        if (!bSuccess)
        {
            //DumpDebugMessage("Init: -WARNING-: Title texture could not be created!");
            m_lpDDSTitle = NULL;
            //SafeRelease(m_lpDDSTitle);
            //return true;
        }
        else
        {
            //sprintf(buf, "Init: title texture size is %dx%d (ideal size was %dx%d)", m_nTitleTexSizeX, m_nTitleTexSizeY, m_nTexSize, m_nTexSize / 4);
            //DumpDebugMessage(buf);
            m_supertext.bRedrawSuperText = true;
        }
    }

    m_texmgr.Init(GetDevice());

    //DumpDebugMessage("Init: mesh allocation");
    m_verts = new MDVERTEX[(m_nGridX + 1) * (m_nGridY + 1)];
    m_verts_temp = new MDVERTEX[(m_nGridX + 2) * 4];
    m_vertinfo = new td_vertinfo[(m_nGridX + 1) * (m_nGridY + 1)];
    m_indices_strip = new int[(m_nGridX + 2) * (m_nGridY * 2)];
    m_indices_list = new int[m_nGridX * m_nGridY * 6];
    if (!m_verts || !m_vertinfo)
    {
        /*
        swprintf_s(buf, L"Could not allocate mesh - out of memory.");
        DumpDebugMessage(buf);
        MessageBox(GetPluginWindow(), buf, WASABI_API_LNGSTRINGW_BUF(IDS_MILKDROP_ERROR, title, 64), MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
        */
        return false;
    }

    int nVert = 0;
    float texel_offset_x = 0.5f / (float)m_nTexSizeX;
    float texel_offset_y = 0.5f / (float)m_nTexSizeY;
    for (int y = 0; y <= m_nGridY; y++)
    {
        for (int x = 0; x <= m_nGridX; x++)
        {
            // Precompute x, y, z.
            m_verts[nVert].x = x / (float)m_nGridX * 2.0f - 1.0f;
            m_verts[nVert].y = y / (float)m_nGridY * 2.0f - 1.0f;
            m_verts[nVert].z = 0.0f;

            // Precompute rad, ang, being conscious of aspect ratio.
            m_vertinfo[nVert].rad = sqrtf(m_verts[nVert].x * m_verts[nVert].x * m_fAspectX * m_fAspectX +
                                          m_verts[nVert].y * m_verts[nVert].y * m_fAspectY * m_fAspectY);
            if (y == m_nGridY / 2 && x == m_nGridX / 2)
                m_vertinfo[nVert].ang = 0.0f;
            else
                m_vertinfo[nVert].ang = atan2f(m_verts[nVert].y * m_fAspectY, m_verts[nVert].x * m_fAspectX);
            m_vertinfo[nVert].a = 1;
            m_vertinfo[nVert].c = 0;

            m_verts[nVert].rad = m_vertinfo[nVert].rad;
            m_verts[nVert].ang = m_vertinfo[nVert].ang;
            m_verts[nVert].tu0 =  m_verts[nVert].x * 0.5f + 0.5f + texel_offset_x;
            m_verts[nVert].tv0 = -m_verts[nVert].y * 0.5f + 0.5f + texel_offset_y;

            nVert++;
        }
    }

    // Generate triangle strips for the 4 quadrants.
    // Each quadrant has m_nGridY/2 strips.
    // Each strip has m_nGridX+2 *points* in it, or m_nGridX/2 polygons.
    int xref, yref;
    int nVert_strip = 0;
    for (int quadrant = 0; quadrant < 4; quadrant++)
    {
        for (int slice = 0; slice < m_nGridY / 2; slice++)
        {
            for (int i = 0; i < m_nGridX + 2; i++)
            {
                // quadrants: 2 3
                //            0 1
                xref = i / 2;
                yref = (i % 2) + slice;

                if (quadrant & 1)
                    xref = m_nGridX - xref;
                if (quadrant & 2)
                    yref = m_nGridY - yref;

                int v = xref + (yref) * (m_nGridX + 1);

                m_indices_strip[nVert_strip++] = v;
            }
        }
    }

    // Also generate triangle lists for drawing the main warp mesh.
    int nVert_list = 0;
    for (int quadrant = 0; quadrant < 4; quadrant++)
    {
        for (int slice = 0; slice < m_nGridY / 2; slice++)
        {
            for (int i = 0; i < m_nGridX / 2; i++)
            {
                // quadrants: 2 3
                //            0 1
                xref = i;
                yref = slice;

                if (quadrant & 1)
                    xref = m_nGridX - 1 - xref;
                if (quadrant & 2)
                    yref = m_nGridY - 1 - yref;

                int v = xref + (yref) * (m_nGridX + 1);

                m_indices_list[nVert_list++] = v;
                m_indices_list[nVert_list++] = v + 1;
                m_indices_list[nVert_list++] = v + m_nGridX + 1;
                m_indices_list[nVert_list++] = v + 1;
                m_indices_list[nVert_list++] = v + m_nGridX + 1;
                m_indices_list[nVert_list++] = v + m_nGridX + 1 + 1;
            }
        }
    }

    // GENERATED TEXTURES FOR SHADERS
    //-------------------------------
    if (m_nMaxPSVersion > 0)
    {
        // Generate noise textures
        if (!AddNoiseTex(L"noise_lq", 256, 1)) return false;
        if (!AddNoiseTex(L"noise_lq_lite", 32, 1)) return false;
        if (!AddNoiseTex(L"noise_mq", 256, 4)) return false;
        if (!AddNoiseTex(L"noise_hq", 256, 8)) return false;

        if (!AddNoiseVol(L"noisevol_lq", 32, 1)) return false;
        if (!AddNoiseVol(L"noisevol_hq", 32, 4)) return false;
    }

    if (!m_bInitialPresetSelected)
    {
        UpdatePresetList(true); //...just does its initial burst!
        LoadRandomPreset(0.0f);
        m_bInitialPresetSelected = true;
    }
    else
        LoadShaders(&m_shaders, m_pState, false); // also force-load the shaders - otherwise they'd only get compiled on a preset switch.

    return true;
}

float fCubicInterpolate(float y0, float y1, float y2, float y3, float t)
{
    float a0, a1, a2, a3, t2;

    t2 = t * t;
    a0 = y3 - y2 - y0 + y1;
    a1 = y0 - y1 - a0;
    a2 = y2 - y0;
    a3 = y1;

    return (a0 * t * t2 + a1 * t2 + a2 * t + a3);
}

DWORD dwCubicInterpolate(DWORD y0, DWORD y1, DWORD y2, DWORD y3, float t)
{
    // performs cubic interpolation on a D3DCOLOR value.
    DWORD ret = 0;
    DWORD shift = 0;
    for (int i = 0; i < 4; i++)
    {
        float f = fCubicInterpolate(((y0 >> shift) & 0xFF) / 255.0f,
                                    ((y1 >> shift) & 0xFF) / 255.0f,
                                    ((y2 >> shift) & 0xFF) / 255.0f,
                                    ((y3 >> shift) & 0xFF) / 255.0f,
                                    t);
        if (f < 0)
            f = 0;
        if (f > 1)
            f = 1;
        ret |= ((DWORD)(f * 255)) << shift;
        shift += 8;
    }

    return ret;
}

// size = width and height of the texture;
// zoom_factor = how zoomed-in the texture features should be.
//   1 = random noise
//   2 = smoothed (interp)
//   4/8/16... = cubic interp.
bool CPlugin::AddNoiseTex(const wchar_t* szTexName, int size, int zoom_factor)
{
    //wchar_t buf[2048], title[64];
    D3D11Shim* lpDevice = GetDevice();
    if (!lpDevice)
        return false;

    // Synthesize noise texture(s)
    ID3D11Texture2D *pNoiseTex = NULL, *pStaging = NULL;

    // Try twice: once with mips, once without.
    //for (int i=0; i<2; i++)
    {
        if (!lpDevice->CreateTexture(size, size, 1, D3D11_BIND_SHADER_RESOURCE, DXGI_FORMAT_R8G8B8A8_UNORM, &pNoiseTex, 0, D3D11_USAGE_DYNAMIC))
        {
            //if (i==1)
            {
                /*
                WASABI_API_LNGSTRINGW_BUF(IDS_COULD_NOT_CREATE_NOISE_TEXTURE, buf, sizeof(buf));
                DumpDebugMessage(buf);
                MessageBox(GetPluginWindow(), buf, WASABI_API_LNGSTRINGW_BUF(IDS_MILKDROP_ERROR, title, 64), MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
                */
                return false;
            }
        }
        //else
        //    break;
    }

    //if (!lpDevice->CreateTexture(size, size, i, 0, DXGI_FORMAT_R8G8B8A8_UNORM, &pStaging, 0, D3D11_USAGE_STAGING))
    //  return false;

    //D3DLOCKED_RECT r;
    D3D11_MAPPED_SUBRESOURCE r;
    if (!lpDevice->LockRect(pNoiseTex, 0, D3D11_MAP_WRITE_DISCARD, &r))
    //if (D3D_OK != pNoiseTex->LockRect(0, &r, NULL, D3DLOCK_DISCARD))
    {
        /*
        WASABI_API_LNGSTRINGW_BUF(IDS_COULD_NOT_LOCK_NOISE_TEXTURE, buf, sizeof(buf));
        DumpDebugMessage(buf);
        MessageBox(GetPluginWindow(), buf, WASABI_API_LNGSTRINGW_BUF(IDS_MILKDROP_ERROR, title, 64), MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
        */
        return false;
    }

    if (r.RowPitch < (unsigned)(size * 4))
    {
        /*
        WASABI_API_LNGSTRINGW_BUF(IDS_NOISE_TEXTURE_BYTE_LAYOUT_NOT_RECOGNISED, buf, sizeof(buf));
        DumpDebugMessage(buf);
        MessageBox(GetPluginWindow(), buf, WASABI_API_LNGSTRINGW_BUF(IDS_MILKDROP_ERROR, title, 64), MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
        */
        return false;
    }

    // Write to the bits...
    DWORD* dst = (DWORD*)r.pData;
    int dwords_per_line = r.RowPitch / sizeof(DWORD);
    int RANGE = (zoom_factor > 1) ? 216 : 256;
    for (int y = 0; y < size; y++)
    {
        LARGE_INTEGER q;
        if (!QueryPerformanceCounter(&q))
            throw std::exception();
        srand(q.LowPart ^ q.HighPart ^ warand());
        for (int x = 0; x < size; x++)
        {
            dst[x] = (((DWORD)(warand() % RANGE) + RANGE / 2) << 24) |
                     (((DWORD)(warand() % RANGE) + RANGE / 2) << 16) |
                     (((DWORD)(warand() % RANGE) + RANGE / 2) << 8) |
                     (((DWORD)(warand() % RANGE) + RANGE / 2));
        }
        // Swap some pixels randomly, to improve "randomness".
        for (int x = 0; x < size; x++)
        {
            int x1 = (warand() ^ q.LowPart) % size;
            int x2 = (warand() ^ q.HighPart) % size;
            DWORD temp = dst[x2];
            dst[x2] = dst[x1];
            dst[x1] = temp;
        }
        dst += dwords_per_line;
    }

    // Smoothing.
    if (zoom_factor > 1)
    {
        // First go across, blending cubically on 'X', but only on the main lines.
        DWORD* dstZF = (DWORD*)r.pData;
        for (int y = 0; y < size; y += zoom_factor)
            for (int x = 0; x < size; x++)
                if (x % zoom_factor)
                {
                    int base_x = (x / zoom_factor) * zoom_factor + size;
                    int base_y = y * dwords_per_line;
                    DWORD y0 = dstZF[base_y + ((base_x - zoom_factor) % size)];
                    DWORD y1 = dstZF[base_y + ((base_x) % size)];
                    DWORD y2 = dstZF[base_y + ((base_x + zoom_factor) % size)];
                    DWORD y3 = dstZF[base_y + ((base_x + zoom_factor * 2) % size)];

                    float t = (x % zoom_factor) / (float)zoom_factor;

                    DWORD result = dwCubicInterpolate(y0, y1, y2, y3, t);

                    dstZF[y * dwords_per_line + x] = result;
                }

        // Next go down, doing cubic interpolation along 'Y', on every line.
        for (int x = 0; x < size; x++)
            for (int y = 0; y < size; y++)
                if (y % zoom_factor)
                {
                    int base_y = (y / zoom_factor) * zoom_factor + size;
                    DWORD y0 = dstZF[((base_y - zoom_factor) % size) * dwords_per_line + x];
                    DWORD y1 = dstZF[((base_y) % size) * dwords_per_line + x];
                    DWORD y2 = dstZF[((base_y + zoom_factor) % size) * dwords_per_line + x];
                    DWORD y3 = dstZF[((base_y + zoom_factor * 2) % size) * dwords_per_line + x];

                    float t = (y % zoom_factor) / (float)zoom_factor;

                    DWORD result = dwCubicInterpolate(y0, y1, y2, y3, t);

                    dstZF[y * dwords_per_line + x] = result;
                }
    }

    // Unlock texture.
    lpDevice->UnlockRect(pNoiseTex, 0);
    //lpDevice->CopyResource(pNoiseTex, pStaging);
    SafeRelease(pStaging);

    // Add it to `m_textures[]`.
    TexInfo x;
    wcscpy_s(x.texname, szTexName);
    x.texptr = pNoiseTex;
    //x.texsize_param = NULL;
    x.w = size;
    x.h = size;
    x.d = 1;
    x.bEvictable = false;
    x.nAge = m_nPresetsLoadedTotal;
    x.nSizeInBytes = 0;
    m_textures.push_back(x);

    return true;
}

// size = width and height and depth of the texture;
// zoom_factor = how zoomed-in the texture features should be.
//   1 = random noise
//   2 = smoothed (interp)
//   4/8/16... = cubic interp.
bool CPlugin::AddNoiseVol(const wchar_t* szTexName, int size, int zoom_factor)
{
    //wchar_t buf[2048], title[64];
    D3D11Shim* lpDevice = GetDevice();
    if (!lpDevice)
        return false;

    // Synthesize noise texture(s)
    ID3D11Texture3D *pNoiseTex = NULL, *pStaging = NULL;
    // try twice - once with mips, once without.
    // NO, TRY JUST ONCE - DX9 doesn't do auto mipgen w/volume textures.  (Debug runtime complains.)
    //for (int i=1; i<2; i++)
    {
        //if (D3D_OK != GetDevice()->CreateVolumeTexture(size, size, size, i, D3DUSAGE_DYNAMIC | (i ? 0 : D3DUSAGE_AUTOGENMIPMAP), D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pNoiseTex, NULL))
        if (!GetDevice()->CreateVolumeTexture(size, size, size, 1, D3D11_BIND_SHADER_RESOURCE, DXGI_FORMAT_R8G8B8A8_UNORM, &pNoiseTex, 0, D3D11_USAGE_DYNAMIC))
        {
            //if (i==1)
            {
                /*
                WASABI_API_LNGSTRINGW_BUF(IDS_COULD_NOT_CREATE_3D_NOISE_TEXTURE, buf, sizeof(buf));
                DumpDebugMessage(buf);
                MessageBox(GetPluginWindow(), buf, WASABI_API_LNGSTRINGW_BUF(IDS_MILKDROP_ERROR, title, 64), MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
                */
                return false;
            }
        }
        //else
        //    break;
    }

    //if (!lpDevice->CreateVolumeTexture(size, size, size, i, 0, DXGI_FORMAT_R8G8B8A8_UNORM, &pStaging, 0, D3D11_USAGE_STAGING))
    //  return false;

    //D3DLOCKED_BOX r;
    D3D11_MAPPED_SUBRESOURCE r;
    if (!lpDevice->LockRect(pNoiseTex, 0, D3D11_MAP_WRITE_DISCARD, &r))
    {
        PopupMessage(IDS_UNABLE_TO_INIT_DXCONTEXT, IDS_MILKDROP_ERROR, true);
        return false;
    }
    if (r.RowPitch < (unsigned)(size * 4) || r.DepthPitch < (unsigned)(size * size * 4))
    {
        /*
        WASABI_API_LNGSTRINGW_BUF(IDS_3D_NOISE_TEXTURE_BYTE_LAYOUT_NOT_RECOGNISED, buf, sizeof(buf));
        DumpDebugMessage(buf);
        MessageBox(GetPluginWindow(), buf, WASABI_API_LNGSTRINGW_BUF(IDS_MILKDROP_ERROR, title, 64), MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
        */
        return false;
    }

    // Write to the bits...
    int dwords_per_slice = r.DepthPitch / sizeof(DWORD);
    int dwords_per_line = r.RowPitch / sizeof(DWORD);
    int RANGE = (zoom_factor > 1) ? 216 : 256;
    for (int z = 0; z < size; z++)
    {
        DWORD* dst = (DWORD*)r.pData + z * dwords_per_slice;
        for (int y = 0; y < size; y++)
        {
            LARGE_INTEGER q;
            if (!QueryPerformanceCounter(&q))
                throw std::exception();
            srand(q.LowPart ^ q.HighPart ^ warand());
            for (int x = 0; x < size; x++)
            {
                dst[x] = (((DWORD)(warand() % RANGE) + RANGE / 2) << 24) |
                         (((DWORD)(warand() % RANGE) + RANGE / 2) << 16) |
                         (((DWORD)(warand() % RANGE) + RANGE / 2) << 8) |
                         (((DWORD)(warand() % RANGE) + RANGE / 2));
            }
            // swap some pixels randomly, to improve 'randomness'
            for (int x = 0; x < size; x++)
            {
                int x1 = (warand() ^ q.LowPart) % size;
                int x2 = (warand() ^ q.HighPart) % size;
                DWORD temp = dst[x2];
                dst[x2] = dst[x1];
                dst[x1] = temp;
            }
            dst += dwords_per_line;
        }
    }

    // Smoothing.
    if (zoom_factor > 1)
    {
        // First go ACROSS, blending cubically on X, but only on the main lines.
        DWORD* dst = (DWORD*)r.pData;
        for (int z = 0; z < size; z += zoom_factor)
            for (int y = 0; y < size; y += zoom_factor)
                for (int x = 0; x < size; x++)
                    if (x % zoom_factor)
                    {
                        int base_x = (x / zoom_factor) * zoom_factor + size;
                        int base_y = z * dwords_per_slice + y * dwords_per_line;
                        DWORD y0 = dst[base_y + ((base_x - zoom_factor) % size)];
                        DWORD y1 = dst[base_y + ((base_x) % size)];
                        DWORD y2 = dst[base_y + ((base_x + zoom_factor) % size)];
                        DWORD y3 = dst[base_y + ((base_x + zoom_factor * 2) % size)];

                        float t = (x % zoom_factor) / (float)zoom_factor;

                        DWORD result = dwCubicInterpolate(y0, y1, y2, y3, t);

                        dst[z * dwords_per_slice + y * dwords_per_line + x] = result;
                    }

        // next go down, doing cubic interp along Y, on the main slices.
        for (int z = 0; z < size; z += zoom_factor)
            for (int x = 0; x < size; x++)
                for (int y = 0; y < size; y++)
                    if (y % zoom_factor)
                    {
                        int base_y = (y / zoom_factor) * zoom_factor + size;
                        int base_z = z * dwords_per_slice;
                        DWORD y0 = dst[((base_y - zoom_factor) % size) * dwords_per_line + base_z + x];
                        DWORD y1 = dst[((base_y) % size) * dwords_per_line + base_z + x];
                        DWORD y2 = dst[((base_y + zoom_factor) % size) * dwords_per_line + base_z + x];
                        DWORD y3 = dst[((base_y + zoom_factor * 2) % size) * dwords_per_line + base_z + x];

                        float t = (y % zoom_factor) / (float)zoom_factor;

                        DWORD result = dwCubicInterpolate(y0, y1, y2, y3, t);

                        dst[y * dwords_per_line + base_z + x] = result;
                    }

        // next go through, doing cubic interp along Z, everywhere.
        for (int x = 0; x < size; x++)
            for (int y = 0; y < size; y++)
                for (int z = 0; z < size; z++)
                    if (z % zoom_factor)
                    {
                        int base_y = y * dwords_per_line;
                        int base_z = (z / zoom_factor) * zoom_factor + size;
                        DWORD y0 = dst[((base_z - zoom_factor) % size) * dwords_per_slice + base_y + x];
                        DWORD y1 = dst[((base_z) % size) * dwords_per_slice + base_y + x];
                        DWORD y2 = dst[((base_z + zoom_factor) % size) * dwords_per_slice + base_y + x];
                        DWORD y3 = dst[((base_z + zoom_factor * 2) % size) * dwords_per_slice + base_y + x];

                        float t = (z % zoom_factor) / (float)zoom_factor;

                        DWORD result = dwCubicInterpolate(y0, y1, y2, y3, t);

                        dst[z * dwords_per_slice + base_y + x] = result;
                    }
    }

    // Unlock texture.
    lpDevice->UnlockRect(pNoiseTex, 0);
    //lpDevice->CopyResource(pNoiseTex, pStaging);
    SafeRelease(pStaging);

    // Add it to `m_textures[]`.
    TexInfo x;
    wcscpy_s(x.texname, szTexName);
    x.texptr = pNoiseTex;
    //x.texsize_param = NULL;
    x.w = size;
    x.h = size;
    x.d = size;
    x.bEvictable = false;
    x.nAge = m_nPresetsLoadedTotal;
    x.nSizeInBytes = 0;
    m_textures.push_back(x);

    return true;
}

void VShaderInfo::Clear()
{
    SafeRelease(ptr);
    SafeRelease(CT);
    params.Clear();
}

void PShaderInfo::Clear()
{
    SafeRelease(ptr);
    SafeRelease(CT);
    params.Clear();
}

// `global_CShaderParams_master_list`: a master list of all CShaderParams classes in existence.
// ** When we evict a texture, we need to NULL out any texptrs these guys have! **
CShaderParamsList global_CShaderParams_master_list;
CShaderParams::CShaderParams()
{
    global_CShaderParams_master_list.shrink_to_fit(); // HACK!!! Exception thrown on 7th allocation. [read access violation. _Pnext was 0x4.]
    global_CShaderParams_master_list.push_back(this);
}

CShaderParams::~CShaderParams()
{
    for (auto it = global_CShaderParams_master_list.begin(); it != global_CShaderParams_master_list.end();)
        if (*it == this)
            global_CShaderParams_master_list.erase(it);
    texsize_params.clear();
}

void CShaderParams::OnTextureEvict(ID3D11Resource* texptr)
{
    for (int i = 0; i < sizeof(m_texture_bindings) / sizeof(m_texture_bindings[0]); i++)
        if (m_texture_bindings[i].texptr == texptr)
            m_texture_bindings[i].texptr = NULL;
}

void CShaderParams::Clear()
{
    // `float4` handles.
    rand_frame = NULL;
    rand_preset = NULL;

    ZeroMemory(rot_mat, sizeof(rot_mat));
    ZeroMemory(const_handles, sizeof(const_handles));
    ZeroMemory(q_const_handles, sizeof(q_const_handles));
    texsize_params.clear();

    // Sampler stages for various PS texture bindings.
    for (int i = 0; i < sizeof(m_texture_bindings) / sizeof(m_texture_bindings[0]); i++)
    {
        m_texture_bindings[i].texptr = NULL;
        m_texcode[i] = TEX_DISK;
    }
}

bool CPlugin::EvictSomeTexture()
{
#if _DEBUG
    // Note: this won't evict a texture whose age is zero,
    //       or whose reported size is zero!
    {
        int nEvictableFiles = 0;
        int nEvictableBytes = 0;
        size_t N = m_textures.size();
        for (size_t i = 0; i < N; i++)
            if (m_textures[i].bEvictable && m_textures[i].texptr)
            {
                nEvictableFiles++;
                nEvictableBytes += m_textures[i].nSizeInBytes;
            }
        wchar_t buf[1024];
        swprintf_s(buf, L"Evicting at %d textures, %.1f MB\n", nEvictableFiles, nEvictableBytes * 0.000001f);
        //OutputDebugString(buf);
    }
#endif

    size_t N = m_textures.size();

    // find age gap
    int newest = 99999999;
    int oldest = 0;
    bool bAtLeastOneFound = false;
    for (size_t i = 0; i < N; i++)
        if (m_textures[i].bEvictable && m_textures[i].nSizeInBytes > 0 && m_textures[i].nAge < m_nPresetsLoadedTotal - 1) // note: -1 here keeps images around for the blend-from preset, too...
        {
            newest = std::min(newest, m_textures[i].nAge);
            oldest = std::max(oldest, m_textures[i].nAge);
            bAtLeastOneFound = true;
        }
    if (!bAtLeastOneFound)
        return false;

    // find the "biggest" texture, but dilate things so that the newest textures
    // are HALF as big as the oldest textures, and thus, less likely to get booted.
    int biggest_bytes = 0;
    int biggest_index = -1;
    for (size_t i = 0; i < N; i++)
        if (m_textures[i].bEvictable && m_textures[i].nSizeInBytes > 0 && m_textures[i].nAge < m_nPresetsLoadedTotal - 1) // note: -1 here keeps images around for the blend-from preset, too...
        {
            float size_mult = 1.0f + (m_textures[i].nAge - newest) / (float)(oldest - newest);
            int bytes = static_cast<int>(m_textures[i].nSizeInBytes * size_mult);
            if (bytes > biggest_bytes)
            {
                biggest_bytes = bytes;
                biggest_index = static_cast<int>(i);
            }
        }
    if (biggest_index == -1)
        return false;

    // Evict that sucker.
    assert(m_textures[biggest_index].texptr);

    // Notify all CShaderParams classes that we're releasing a bindable texture!!
    for (auto const& i : global_CShaderParams_master_list)
        i->OnTextureEvict(m_textures[biggest_index].texptr);

    // 2. Erase the texture itself.
    SafeRelease(m_textures[biggest_index].texptr);
    m_textures.erase(m_textures.begin() + biggest_index);

    return true;
}

std::wstring texture_exts[] = {L"jpg", L"png", L"dds", L"tga", L"bmp", L"dib"};
const wchar_t szExtsWithSlashes[] = L"jpg|png|dds|etc.";
typedef std::vector<std::wstring> StringVec;
bool PickRandomTexture(const wchar_t* prefix, wchar_t* szRetTextureFilename) // should be MAX_PATH chars
{
    static StringVec texfiles;
    static DWORD texfiles_timestamp = 0; // update this a max of every ~2 seconds or so

    // If it's been more than a few seconds since the last textures dir scan, redo it.
    // (..just enough to make sure we don't do it more than once per preset load)
    //DWORD t = timeGetTime(); // in milliseconds
    //if (abs(t - texfiles_timestamp) > 2000)
    if (g_plugin.m_bNeedRescanTexturesDir)
    {
        g_plugin.m_bNeedRescanTexturesDir = false; //texfiles_timestamp = t;
        texfiles.clear();

        wchar_t szMask[MAX_PATH];
        swprintf_s(szMask, L"%stextures\\*.*", g_plugin.m_szMilkdrop2Path);

        WIN32_FIND_DATAW ffd = {0};

        HANDLE hFindFile = INVALID_HANDLE_VALUE;
        if ((hFindFile = FindFirstFile(szMask, &ffd)) == INVALID_HANDLE_VALUE) // note: returns filename without path
            return false;

        // First, count valid texture files.
        do
        {
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;

            wchar_t* ext = wcsrchr(ffd.cFileName, L'.');
            if (!ext)
                continue;

            for (int i = 0; i < sizeof(texture_exts) / sizeof(texture_exts[0]); i++)
                if (!_wcsicmp(texture_exts[i].c_str(), ext + 1))
                {
                    // Valid texture found - add it to the list. ("heart.jpg", for example).
                    texfiles.push_back(ffd.cFileName);
                    continue;
                }
        } while (FindNextFileW(hFindFile, &ffd));
        FindClose(hFindFile);
    }

    if (texfiles.size() == 0)
        return false;

    // Then randomly pick one.
    if (prefix == NULL || prefix[0] == 0)
    {
        // Pick randomly from entire list.
        int i = warand() % texfiles.size();
        wcscpy_s(szRetTextureFilename, MAX_PATH, texfiles[i].c_str());
    }
    else
    {
        // Only pick from files with the right prefix.
        StringVec temp_list;
        size_t N = texfiles.size();
        size_t len = wcslen(prefix);
        for (size_t i = 0; i < N; i++)
            if (!_wcsnicmp(prefix, texfiles[i].c_str(), len))
                temp_list.push_back(texfiles[i]);
        N = temp_list.size();
        if (N == 0)
            return false;
        // Pick randomly from the subset.
        int j = warand() % temp_list.size();
        wcscpy_s(szRetTextureFilename, MAX_PATH, temp_list[j].c_str());
    }
    return true;
}

void CShaderParams::CacheParams(CConstantTable* pCT, bool /* bHardErrors */)
{
    Clear();

    if (!pCT)
        return;

#define MAX_RAND_TEX 16
    std::wstring RandTexName[MAX_RAND_TEX];

    // pass 1: find all the samplers (and texture bindings).
    for (UINT i = 0; i < pCT->ShaderDesc.BoundResources; i++)
    {
        ShaderBinding* binding = pCT->GetBindingByIndex(i);
        D3D11_SHADER_INPUT_BIND_DESC cd = binding->Description;
        LPCSTR h = cd.Name;
        //unsigned int count = 1;

        //cd.Name = VS_Sampler
        //cd.RegisterSet = D3DXRS_SAMPLER
        //cd.RegisterIndex = 3
        if (cd.Type == D3D_SIT_SAMPLER && cd.BindPoint >= 0 && cd.BindPoint < sizeof(m_texture_bindings) / sizeof(m_texture_bindings[0]))
        {
            assert(m_texture_bindings[cd.BindPoint].texptr == NULL);

            // Remove "sampler_" prefix to create root file name. Could still have "FW_" prefix or something like that.
            wchar_t szRootName[MAX_PATH];
            if (!strncmp(cd.Name, "sampler_", 8))
                wcscpy_s(szRootName, AutoWide(&cd.Name[8]));
            else
                wcscpy_s(szRootName, AutoWide(cd.Name));

            // Also peel off "XY_" prefix, if it's there, to specify filtering & wrap mode.
            bool bBilinear = true;
            bool bWrap = true;
            bool bWrapFilterSpecified = false;
            if (wcslen(szRootName) > 3 && szRootName[2] == L'_')
            {
                wchar_t temp[3];
                temp[0] = szRootName[0];
                temp[1] = szRootName[1];
                temp[2] = 0;
                // Convert to uppercase.
                if (temp[0] >= L'a' && temp[0] <= L'z')
                    temp[0] -= L'a' - L'A';
                if (temp[1] >= L'a' && temp[1] <= L'z')
                    temp[1] -= L'a' - L'A';

                // clang-format off
                if      (!wcscmp(temp, L"FW")) { bWrapFilterSpecified = true; bBilinear = true;  bWrap = true; }
                else if (!wcscmp(temp, L"FC")) { bWrapFilterSpecified = true; bBilinear = true;  bWrap = false; }
                else if (!wcscmp(temp, L"PW")) { bWrapFilterSpecified = true; bBilinear = false; bWrap = true; }
                else if (!wcscmp(temp, L"PC")) { bWrapFilterSpecified = true; bBilinear = false; bWrap = false; }
                // Also allow reverses.
                else if (!wcscmp(temp, L"WF")) { bWrapFilterSpecified = true; bBilinear = true;  bWrap = true; }
                else if (!wcscmp(temp, L"CF")) { bWrapFilterSpecified = true; bBilinear = true;  bWrap = false; }
                else if (!wcscmp(temp, L"WP")) { bWrapFilterSpecified = true; bBilinear = false; bWrap = true; }
                else if (!wcscmp(temp, L"CP")) { bWrapFilterSpecified = true; bBilinear = false; bWrap = false; }
                // clang-format on

                // Peel off the prefix.
                int j = 0;
                while (szRootName[j + 3])
                {
                    szRootName[j] = szRootName[j + 3];
                    j++;
                }
                szRootName[j] = 0;
            }
            std::string strName(h);
            m_texture_bindings[cd.BindPoint].bWrap = bWrap;
            m_texture_bindings[cd.BindPoint].bBilinear = bBilinear;
            m_texture_bindings[cd.BindPoint].bindPoint = pCT->GetTextureSlot(strName);

            // if <szFileName> is "main", map it to the VS...
            if (!wcscmp(L"main", szRootName))
            {
                m_texture_bindings[cd.BindPoint].texptr = NULL;
                m_texcode[cd.BindPoint] = TEX_VS;
            }
#if (NUM_BLUR_TEX >= 2)
            else if (!wcscmp(L"blur1", szRootName))
            {
                m_texture_bindings[cd.BindPoint].texptr = g_plugin.m_lpBlur[1];
                m_texcode[cd.BindPoint] = TEX_BLUR1;
                if (!bWrapFilterSpecified)
                { // when sampling blur textures, default is CLAMP
                    m_texture_bindings[cd.BindPoint].bWrap = false;
                    m_texture_bindings[cd.BindPoint].bBilinear = true;
                }
            }
#endif
#if (NUM_BLUR_TEX >= 4)
            else if (!wcscmp(L"blur2", szRootName))
            {
                m_texture_bindings[cd.BindPoint].texptr = g_plugin.m_lpBlur[3];
                m_texcode[cd.BindPoint] = TEX_BLUR2;
                if (!bWrapFilterSpecified)
                { // when sampling blur textures, default is CLAMP
                    m_texture_bindings[cd.BindPoint].bWrap = false;
                    m_texture_bindings[cd.BindPoint].bBilinear = true;
                }
            }
#endif
#if (NUM_BLUR_TEX >= 6)
            else if (!wcscmp(L"blur3", szRootName))
            {
                m_texture_bindings[cd.BindPoint].texptr = g_plugin.m_lpBlur[5];
                m_texcode[cd.BindPoint] = TEX_BLUR3;
                if (!bWrapFilterSpecified)
                { // when sampling blur textures, default is CLAMP
                    m_texture_bindings[cd.BindPoint].bWrap = false;
                    m_texture_bindings[cd.BindPoint].bBilinear = true;
                }
            }
#endif
#if (NUM_BLUR_TEX >= 8)
            else if (!wcscmp("blur4", szRootName))
            {
                m_texture_bindings[cd.RegisterIndex].texptr = g_plugin.m_lpBlur[7];
                m_texcode[cd.RegisterIndex] = TEX_BLUR4;
                if (!bWrapFilterSpecified)
                { // when sampling blur textures, default is CLAMP
                    m_texture_bindings[cd.RegisterIndex].bWrap = false;
                    m_texture_bindings[cd.RegisterIndex].bBilinear = true;
                }
            }
#endif
#if (NUM_BLUR_TEX >= 10)
            else if (!wcscmp("blur5", szRootName))
            {
                m_texture_bindings[cd.RegisterIndex].texptr = g_plugin.m_lpBlur[9];
                m_texcode[cd.RegisterIndex] = TEX_BLUR5;
                if (!bWrapFilterSpecified)
                { // when sampling blur textures, default is CLAMP
                    m_texture_bindings[cd.RegisterIndex].bWrap = false;
                    m_texture_bindings[cd.RegisterIndex].bBilinear = true;
                }
            }
#endif
#if (NUM_BLUR_TEX >= 12)
            else if (!wcscmp("blur6", szRootName))
            {
                m_texture_bindings[cd.RegisterIndex].texptr = g_plugin.m_lpBlur[11];
                m_texcode[cd.RegisterIndex] = TEX_BLUR6;
                if (!bWrapFilterSpecified)
                { // when sampling blur textures, default is CLAMP
                    m_texture_bindings[cd.RegisterIndex].bWrap = false;
                    m_texture_bindings[cd.RegisterIndex].bBilinear = true;
                }
            }
#endif
            else
            {
                m_texcode[cd.BindPoint] = TEX_DISK;

                // Check for request for random texture.
                if (!wcsncmp(L"rand", szRootName, 4) &&
                    IsNumericChar(szRootName[4]) &&
                    IsNumericChar(szRootName[5]) &&
                    (szRootName[6] == 0 || szRootName[6] == '_'))
                {
                    int rand_slot = -1;

                    // Peel off filename prefix ("rand13_smalltiled", for example).
                    wchar_t prefix[MAX_PATH];
                    if (szRootName[6] == L'_')
                        wcscpy_s(prefix, &szRootName[7]);
                    else
                        prefix[0] = 0;
                    szRootName[6] = 0;

                    swscanf_s(&szRootName[4], L"%d", &rand_slot);
                    if (rand_slot >= 0 && rand_slot <= 15) // otherwise, not a special filename - ignore it
                    {
                        if (!PickRandomTexture(prefix, szRootName))
                        {
                            if (prefix[0])
                                swprintf_s(szRootName, L"[rand%02d] %s*", rand_slot, prefix);
                            else
                                swprintf_s(szRootName, L"[rand%02d] *", rand_slot);
                        }
                        else
                        {
                            // Chop off extension
                            wchar_t* p = wcsrchr(szRootName, L'.');
                            if (p)
                                *p = 0;
                        }

                        assert(RandTexName[rand_slot].length() == 0);
                        RandTexName[rand_slot] = szRootName; // we'll need to remember this for texsize_ params!
                    }
                }

                // See if <szRootName>.tga or .jpg has already been loaded.
                //   (if so, grab a pointer to it)
                //   (if NOT, create & load it).
                size_t N1 = g_plugin.m_textures.size();
                for (size_t n = 0; n < N1; n++)
                {
                    if (!wcscmp(g_plugin.m_textures[n].texname, szRootName))
                    {
                        // Found a match - texture was already loaded.
                        m_texture_bindings[cd.BindPoint].texptr = g_plugin.m_textures[n].texptr;
                        // Also bump its age down to zero! (for cache management)
                        g_plugin.m_textures[n].nAge = g_plugin.m_nPresetsLoadedTotal;
                        break;
                    }
                }
                // If still not found, load it up / make a new texture.
                if (!m_texture_bindings[cd.BindPoint].texptr)
                {
                    TexInfo x;
                    wcsncpy_s(x.texname, szRootName, 254);
                    x.texptr = NULL;
                    //x.texsize_param = NULL;

                    // Check if we need to evict anything from the cache,
                    // due to our own cache constraints...
                    while (1)
                    {
                        int nTexturesCached = 0;
                        int nBytesCached = 0;
                        size_t N2 = g_plugin.m_textures.size();
                        for (size_t n = 0; n < N2; n++)
                            if (g_plugin.m_textures[n].bEvictable && g_plugin.m_textures[n].texptr)
                            {
                                nBytesCached += g_plugin.m_textures[n].nSizeInBytes;
                                nTexturesCached++;
                            }
                        if (nTexturesCached < g_plugin.m_nMaxImages && nBytesCached < g_plugin.m_nMaxBytes)
                            break;
                        // Otherwise, evict now - and loop until within the constraints.
                        if (!g_plugin.EvictSomeTexture())
                            break; // or if there was nothing to evict, just give up
                    }

                    // Load the texture.
                    wchar_t szFilename[MAX_PATH];
                    for (int z = 0; z < sizeof(texture_exts) / sizeof(texture_exts[0]); z++)
                    {
                        swprintf_s(szFilename, L"%stextures\\%s.%s", g_plugin.m_szMilkdrop2Path, szRootName, texture_exts[z].c_str());
                        if (GetFileAttributes(szFilename) == INVALID_FILE_ATTRIBUTES)
                        {
                            // Try again, but in presets directory.
                            swprintf_s(szFilename, L"%s%s.%s", g_plugin.m_szPresetDir, szRootName, texture_exts[z].c_str());
                            if (GetFileAttributes(szFilename) == INVALID_FILE_ATTRIBUTES)
                                continue;
                        }

                        // Keep trying to load it - if it fails due to memory, evict something and try again.
                        while (1)
                        {
                            HRESULT hr = g_plugin.GetDevice()->CreateTextureFromFile(szFilename, &x.texptr);
                            if (hr == E_OUTOFMEMORY)
                            {
                                // Out of memory - try evicting something old and/or big.
                                if (g_plugin.EvictSomeTexture())
                                    continue;
                            }

                            if (hr == S_OK)
                            {
                                D3D11_RESOURCE_DIMENSION type;
                                x.texptr->GetType(&type);
                                if (type == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
                                {
                                    D3D11_TEXTURE2D_DESC texDesc;
                                    reinterpret_cast<ID3D11Texture2D*>(x.texptr)->GetDesc(&texDesc);
                                    x.w = texDesc.Width;
                                    x.h = texDesc.Height;
                                    x.d = 1;
                                    int nPixels = texDesc.Width * texDesc.Height;
                                    int BitsPerPixel = GetDX11TexFormatBitsPerPixel(texDesc.Format);
                                    x.nSizeInBytes = nPixels * BitsPerPixel / 8 + 16384; //plus some overhead
                                }
                                if (type == D3D11_RESOURCE_DIMENSION_TEXTURE3D)
                                {
                                    D3D11_TEXTURE3D_DESC texDesc;
                                    reinterpret_cast<ID3D11Texture3D*>(x.texptr)->GetDesc(&texDesc);
                                    x.w = texDesc.Width;
                                    x.h = texDesc.Height;
                                    x.d = texDesc.Depth;
                                    x.bEvictable = true;
                                    x.nAge = g_plugin.m_nPresetsLoadedTotal;
                                    int nPixels = texDesc.Width * texDesc.Height * std::max(static_cast<UINT>(1), texDesc.Depth);
                                    int BitsPerPixel = GetDX11TexFormatBitsPerPixel(texDesc.Format);
                                    x.nSizeInBytes = nPixels * BitsPerPixel / 8 + 16384; // plus some overhead
                                }
                            }
                            break;
                        }
                    }

                    if (!x.texptr)
                    {
                        /*
                        wchar_t buf[2048] = {0}, title[64] = {0};
                        swprintf_s(buf, WASABI_API_LNGSTRINGW(IDS_COULD_NOT_LOAD_TEXTURE_X), szRootName, szExtsWithSlashes);
                        DumpDebugMessage(buf);
                        if (bHardErrors)
                            MessageBox(g_plugin.GetPluginWindow(), buf, WASABI_API_LNGSTRINGW_BUF(IDS_MILKDROP_ERROR, title, 64), MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
                        else
                            AddError(buf, 6.0f, ERR_PRESET, true);
                        */
                        return;
                    }

                    g_plugin.m_textures.push_back(x);
                    m_texture_bindings[cd.BindPoint].texptr = x.texptr;
                }
            }
        }
    }

    // Pass 2: bind all the float4's."texsize_XYZ" params will be filled out via knowledge of loaded texture sizes.
    for (size_t i = 0; i < pCT->GetVariablesCount(); i++)
    {
        ShaderVariable* var = pCT->GetVariableByIndex(i);
        LPCSTR h = var->Description.Name;
        //unsigned int count = 1;
        D3D11_SHADER_VARIABLE_DESC cd = var->Description;
        D3D11_SHADER_TYPE_DESC ct = var->Type;

        if (cd.uFlags == 0) // DX11 do not process unused variables.
            continue;
        //pCT->GetConstantDesc(h, &cd, &count);

        if (ct.Type == D3D_SVT_FLOAT)
        {
            if (ct.Class == D3D_SVC_MATRIX_COLUMNS)
            {
                if      (!strcmp(cd.Name, "rot_s1"))  rot_mat[0]  = h;
                else if (!strcmp(cd.Name, "rot_s2"))  rot_mat[1]  = h;
                else if (!strcmp(cd.Name, "rot_s3"))  rot_mat[2]  = h;
                else if (!strcmp(cd.Name, "rot_s4"))  rot_mat[3]  = h;
                else if (!strcmp(cd.Name, "rot_d1"))  rot_mat[4]  = h;
                else if (!strcmp(cd.Name, "rot_d2"))  rot_mat[5]  = h;
                else if (!strcmp(cd.Name, "rot_d3"))  rot_mat[6]  = h;
                else if (!strcmp(cd.Name, "rot_d4"))  rot_mat[7]  = h;
                else if (!strcmp(cd.Name, "rot_f1"))  rot_mat[8]  = h;
                else if (!strcmp(cd.Name, "rot_f2"))  rot_mat[9]  = h;
                else if (!strcmp(cd.Name, "rot_f3"))  rot_mat[10] = h;
                else if (!strcmp(cd.Name, "rot_f4"))  rot_mat[11] = h;
                else if (!strcmp(cd.Name, "rot_vf1")) rot_mat[12] = h;
                else if (!strcmp(cd.Name, "rot_vf2")) rot_mat[13] = h;
                else if (!strcmp(cd.Name, "rot_vf3")) rot_mat[14] = h;
                else if (!strcmp(cd.Name, "rot_vf4")) rot_mat[15] = h;
                else if (!strcmp(cd.Name, "rot_uf1")) rot_mat[16] = h;
                else if (!strcmp(cd.Name, "rot_uf2")) rot_mat[17] = h;
                else if (!strcmp(cd.Name, "rot_uf3")) rot_mat[18] = h;
                else if (!strcmp(cd.Name, "rot_uf4")) rot_mat[19] = h;
                else if (!strcmp(cd.Name, "rot_rand1")) rot_mat[20] = h;
                else if (!strcmp(cd.Name, "rot_rand2")) rot_mat[21] = h;
                else if (!strcmp(cd.Name, "rot_rand3")) rot_mat[22] = h;
                else if (!strcmp(cd.Name, "rot_rand4")) rot_mat[23] = h;
            }
            else if (ct.Class == D3D_SVC_VECTOR)
            {
                if (!strcmp(cd.Name, "rand_frame"))
                    rand_frame = h;
                else if (!strcmp(cd.Name, "rand_preset"))
                    rand_preset = h;
                else if (!strncmp(cd.Name, "texsize_", 8))
                {
                    // Remove "texsize_" prefix to find root file name.
                    wchar_t szRootName[MAX_PATH] = {0};
                    if (!strncmp(cd.Name, "texsize_", 8))
                        wcscpy_s(szRootName, AutoWide(&cd.Name[8]));
                    else
                        wcscpy_s(szRootName, AutoWide(cd.Name));

                    // Check for request for random texture.
                    // It should be a previously-seen random index - just fetch/reuse the name.
                    if (!wcsncmp(L"rand", szRootName, 4) &&
                        IsNumericChar(szRootName[4]) &&
                        IsNumericChar(szRootName[5]) &&
                        (szRootName[6] == L'\0' || szRootName[6] == L'_'))
                    {
                        int rand_slot = -1;

                        // Ditch filename prefix ("rand13_smalltiled", for example)
                        // and just go by the slot.
                        if (szRootName[6] == L'_')
                            szRootName[6] = L'\0';

                        swscanf_s(&szRootName[4], L"%d", &rand_slot);
                        if (rand_slot >= 0 && rand_slot <= 15) // otherwise, not a special filename - ignore it
                            if (RandTexName[rand_slot].length() > 0)
                                wcscpy_s(szRootName, RandTexName[rand_slot].c_str());
                    }

                    // see if <szRootName>.tga or .jpg has already been loaded.
                    bool bTexFound = false;
                    size_t N = g_plugin.m_textures.size();
                    for (size_t n = 0; n < N; n++)
                    {
                        if (!wcscmp(g_plugin.m_textures[n].texname, szRootName))
                        {
                            // Found a match - texture was loaded.
                            TexSizeParamInfo y;
                            y.texname = szRootName; // for debugging
                            y.texsize_param = h;
                            y.w = g_plugin.m_textures[n].w;
                            y.h = g_plugin.m_textures[n].h;
                            texsize_params.push_back(y);

                            bTexFound = true;
                            break;
                        }
                    }

                    if (!bTexFound)
                    {
                        /*
                        wchar_t buf[1024] = {0};
                        swprintf_s(buf, WASABI_API_LNGSTRINGW(IDS_UNABLE_TO_RESOLVE_TEXSIZE_FOR_A_TEXTURE_NOT_IN_USE), cd.Name);
                        g_plugin.AddError(buf, 6.0f, ERR_PRESET, true);
                        */
                    }
                }
                else if (cd.Name[0] == '_' && cd.Name[1] == 'c')
                {
                    int z;
                    if (sscanf_s(&cd.Name[2], "%d", &z) == 1)
                        if (z >= 0 && z < sizeof(const_handles) / sizeof(const_handles[0]))
                            const_handles[z] = h;
                }
                else if (cd.Name[0] == '_' && cd.Name[1] == 'q')
                {
                    int z = cd.Name[2] - 'a';
                    if (z >= 0 && z < sizeof(q_const_handles) / sizeof(q_const_handles[0]))
                        q_const_handles[z] = h;
                }
            }
        }
    }
}

bool CPlugin::RecompileVShader(const char* szShadersText, VShaderInfo* si, int shaderType, bool bHardErrors)
{
    SafeRelease(si->ptr);
    ZeroMemory(si, sizeof(VShaderInfo));

    // LOAD SHADER
    if (!LoadShaderFromMemory(szShadersText, "VS", "vs_4_0_level_9_1", &si->CT, (void**)&si->ptr, shaderType, bHardErrors && (GetScreenMode() == WINDOWED)))
        return false;

    // Track down texture and float4 param bindings for this shader.
    // Also loads any textures that need loaded.
    si->params.CacheParams(si->CT, bHardErrors);

    return true;
}

bool CPlugin::RecompilePShader(const char* szShadersText, PShaderInfo* si, int shaderType, bool bHardErrors, int PSVersion)
{
    assert(m_nMaxPSVersion > 0);

    SafeRelease(si->ptr);
    ZeroMemory(si, sizeof(PShaderInfo));

    // Load shader.
    // Note: ps_1_4 required for dependent texture lookups.
    //       ps_2_0 required for tex2Dbias.
    char ver[32] = {0};
    strcpy_s(ver, "ps_0_0");
    switch (PSVersion)
    {
        case MD2_PS_NONE: strcpy_s(ver, "ps_4_0_level_9_1"); break; // was ps_2_0; even though the PRESET doesn't use shaders, if MilkDrop is running where it CAN do shaders,
                                                                    //             run all the old presets through(shader) emulation.
                                                                    //             This way, MilkDrop is always calling either `WarpedBlit()` or `WarpedBlit_NoPixelShaders()`,
                                                                    //             and blending always works.
        case MD2_PS_2_0: strcpy_s(ver, "ps_4_0_level_9_1"); break; // was ps_2_a
        case MD2_PS_2_X: strcpy_s(ver, "ps_4_0_level_9_3"); break; // was ps_3_0; try ps_2_a first, `LoadShaderFromMemory()` will try ps_2_b if compilation fails
        case MD2_PS_3_0: strcpy_s(ver, "ps_4_0_level_9_3"); break; // was ps_3_0
        case MD2_PS_4_0: strcpy_s(ver, "ps_4_0"); break;
        case MD2_PS_5_0: strcpy_s(ver, "ps_5_0"); break;
        default: assert(0); break;
    }

    if (!LoadShaderFromMemory(szShadersText, "PS", ver, &si->CT, (void**)&si->ptr, shaderType, bHardErrors && (GetScreenMode() == WINDOWED)))
        return false;

    // Track down texture & float4 param bindings for this shader.
    // Also loads any textures that need loaded.
    si->params.CacheParams(si->CT, bHardErrors);

    return true;
}

bool CPlugin::LoadShaders(PShaderSet* sh, CState* pState, bool bTick)
{
    if (m_nMaxPSVersion <= 0)
        return true;

    // Load one of the pixel shaders.
    if (!sh->warp.ptr && pState->m_nWarpPSVersion > 0)
    {
        bool bOK = RecompilePShader(pState->m_szWarpShadersText, &sh->warp, SHADER_WARP, false, pState->m_nWarpPSVersion);
        if (!bOK)
        {
            // Switch to fallback shader.
            m_fallbackShaders_ps.warp.ptr->AddRef();
            m_fallbackShaders_ps.warp.CT->AddRef();
            memcpy_s(&sh->warp, sizeof(PShaderInfo), &m_fallbackShaders_ps.warp, sizeof(PShaderInfo));
            // Cancel any slow-preset-load.
            //m_nLoadingPreset = 1000;
        }

        if (bTick)
            return true;
    }

    if (!sh->comp.ptr && pState->m_nCompPSVersion > 0)
    {
        bool bOK = RecompilePShader(pState->m_szCompShadersText, &sh->comp, SHADER_COMP, false, pState->m_nCompPSVersion);
        if (!bOK)
        {
            // Switch to fallback shader.
            m_fallbackShaders_ps.comp.ptr->AddRef();
            m_fallbackShaders_ps.comp.CT->AddRef();
            memcpy(&sh->comp, &m_fallbackShaders_ps.comp, sizeof(PShaderInfo));
            // Cancel any slow-preset-load.
            //m_nLoadingPreset = 1000;
        }
    }

    return true;
}

//----------------------------------------------------------------------

bool CPlugin::LoadShaderFromMemory(const char* szOrigShaderText, const char* szFn, const char* szProfile, CConstantTable** ppConstTable, void** ppShader, const int shaderType, const bool /*bHardErrors*/)
{
    // clang-format off
    const char szWarpDefines[] = "#define rad _rad_ang.x\n"
                                 "#define ang _rad_ang.y\n"
                                 "#define uv _uv.xy\n"
                                 "#define uv_orig _uv.zw\n";
    const char szCompDefines[] = "#define rad _rad_ang.x\n"
                                 "#define ang _rad_ang.y\n"
                                 "#define uv _uv.xy\n"
                                 "#define uv_orig _uv.xy\n" //[sic]
                                 "#define hue_shader _vDiffuse.xyz\n";
    const char szWarpParams[]  = "float4 _vDiffuse : COLOR, float4 _uv : TEXCOORD0, float2 _rad_ang : TEXCOORD1, out float4 _return_value : COLOR0";
    const char szCompParams[]  = "float4 _vDiffuse : COLOR, float2 _uv : TEXCOORD0, float2 _rad_ang : TEXCOORD1, out float4 _return_value : COLOR0";
    const char szFirstLine[]   = "    float3 ret = 0;";
    const char szLastLine[]    = "    _return_value = float4(ret.xyz, _vDiffuse.w);";
    // clang-format on

    char szWhichShader[64] = {0};
    switch (shaderType)
    {
        case SHADER_WARP: strcpy_s(szWhichShader, "warp"); break;
        case SHADER_COMP: strcpy_s(szWhichShader, "composite"); break;
        case SHADER_BLUR: strcpy_s(szWhichShader, "blur"); break;
        case SHADER_OTHER: strcpy_s(szWhichShader, "(other)"); break;
        default: strcpy_s(szWhichShader, "(unknown)"); break;
    }

    ID3DBlob* pShaderByteCode;
    //wchar_t title[64] = {0};

    *ppShader = NULL;
    *ppConstTable = NULL;

    char szShaderText[128000];
    char temp[128000];
    size_t writePos = 0;

    // Paste the universal `#include`.
    strcpy_s(&szShaderText[writePos], ARRAYSIZE(szShaderText), m_szShaderIncludeText); // first, paste in the contents of "include.fx" before the actual shader text. Has 13's and 10's.
    writePos += m_nShaderIncludeTextLen;

    // Paste in any custom #defines for this shader type.
    if (shaderType == SHADER_WARP && szProfile[0] == 'p')
    {
        strcpy_s(&szShaderText[writePos], ARRAYSIZE(szShaderText) - writePos, szWarpDefines);
        writePos += strlen(szWarpDefines);
    }
    else if (shaderType == SHADER_COMP && szProfile[0] == 'p')
    {
        strcpy_s(&szShaderText[writePos], ARRAYSIZE(szShaderText) - writePos, szCompDefines);
        writePos += strlen(szCompDefines);
    }

    // Paste in the shader itself - converting LCCs to 13+10s.
    // Avoid `lstrcpy()` because it might not handle the linefeed stuff...?
    size_t shaderStartPos = writePos;
    {
        const char* s = szOrigShaderText;
        char* d = &szShaderText[writePos];
        while (*s)
        {
            if (*s == LINEFEED_CONTROL_CHAR)
            {
                *d++ = '\r';
                writePos++;
                *d++ = '\n';
                writePos++;
            }
            else
            {
                *d++ = *s;
                writePos++;
            }
            s++;
        }
        *d = '\0';
        writePos++;
    }

    // Strip out all comments - but cheat a little - start at the shader test.
    // (the include file was already stripped of comments)
    StripComments(&szShaderText[shaderStartPos]);

    /*{
        char* p = szShaderText;
        while (*p)
        {
            char buf[32];
            buf[0] = *p;
            buf[1] = '\0';
            OutputDebugString(buf);
            if ((rand() % 9) == 0)
                Sleep(1);
            p++;
        }
        OutputDebugString("\n");
    }/**/

    // Note: Only do this if type is WARP or COMP shader... not for BLUR, etc!
    // FIXME - hints on the inputs / output / samplers etc.
    //         can go in the menu header, NOT the preset! =)
    // then update presets.
    //   -> be sure to update the presets on disk AND THE DEFAULT SHADERS (for loading MD1 presets)
    // FIXME - then update auth. guide w/new examples,
    //         and a list of the invisible inputs (and one output) to each shader!
    //         warp: float2 uv, float2 uv_orig, rad, ang
    //         comp: float2 uv, rad, ang, float3 hue_shader
    // Test all this string code in Debug mode - make sure nothing bad is happening.

    /*
    1. Paste warp or comp #defines
    2. Search for "void" + whitespace + szFn + [whitespace] + '('
    3. Insert parameters
    4. Search for [whitespace] + ')'.
    5. Search for final '}' (strrchr)
    6. Back up one char, insert the Last Line, and add '}' and that's it.
    */
    if ((shaderType == SHADER_WARP || shaderType == SHADER_COMP) && szProfile[0] == 'p')
    {
        char* p = &szShaderText[shaderStartPos];

        // Seek to "shader_body" and replace it with spaces.
        while (*p && strncmp(p, "shader_body", 11))
            p++;
        if (p)
        {
            for (int i = 0; i < 11; i++)
                *p++ = ' ';
        }

        if (p)
        {
            // Insert "void PS(...params...)\n".
            strcpy_s(temp, p);
            const char* params = (shaderType == SHADER_WARP) ? szWarpParams : szCompParams;
            size_t remains = ARRAYSIZE(szShaderText) - (p - &szShaderText[0] + 1) + 1;
            int length = sprintf_s(p, remains, "void %s( %s )\n", szFn, params);
            p += length;
            strcpy_s(p, remains - length, temp);

            // Find the starting curly brace.
            p = strchr(p, '{');
            if (p)
            {
                // Skip over it.
                p++;
                // Then insert "float3 ret = 0;".
                strcpy_s(temp, p);
                remains = ARRAYSIZE(szShaderText) - (p - &szShaderText[0] + 1) + 1;
                length = sprintf_s(p, remains, "%s\n", szFirstLine);
                p += length;
                strcpy_s(p, remains - length, temp);

                // Find the ending curly brace.
                p = strrchr(p, '}');
                // Add the last line - "    _return_value = float4(ret.xyz, _vDiffuse.w);".
                if (p)
                {
                    remains = ARRAYSIZE(szShaderText) - (p - &szShaderText[0] + 1) + 1;
                    sprintf_s(p, remains, " %s\n}\n", szLastLine);
                }
            }
        }

        if (!p)
        {
            /*
            wchar_t temp[512] = {0};
            swprintf_s(err, WASABI_API_LNGSTRINGW(IDS_ERROR_PARSING_X_X_SHADER), szProfile, szWhichShader);
            DumpDebugMessage(temp);
            AddError(temp, 8.0f, ERR_PRESET, true);
            */
            return false;
        }
    }

    // Now really try to compile the shader.
    bool failed = false;
    size_t len = strlen(szShaderText);
    int flags = D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY;
    if (S_OK != D3DCompile(szShaderText, len, NULL, NULL, NULL, szFn, szProfile, flags, 0, &pShaderByteCode, &m_pShaderCompileErrors))
    {
        failed = true;
    }

    if (failed && !strcmp(szProfile, "ps_4_0_level_9_1"))
    {
        SafeRelease(m_pShaderCompileErrors);
        if (S_OK == D3DCompile(szShaderText, len, NULL, NULL, NULL, szFn, "ps_4_0_level_9_3", flags, 0, &pShaderByteCode, &m_pShaderCompileErrors))
        {
            failed = false;
        }
    }

    if (failed)
    {
        /*
        wchar_t temp[1024] = {0};
        swprintf_s(err, WASABI_API_LNGSTRINGW(IDS_ERROR_COMPILING_X_X_SHADER), strcmp(szProfile, "ps_4_0_level_9_1") ? szProfile : "ps_4_0_level_9_3", szWhichShader);
        if (m_pShaderCompileErrors && m_pShaderCompileErrors->GetBufferSize() < sizeof(temp) - 256)
        {
            //strcat_s(tempw, L"\n\n");
            wcscat_s(err, AutoWide(reinterpret_cast<char*>(m_pShaderCompileErrors->GetBufferPointer())));
        }
        */
        SafeRelease(m_pShaderCompileErrors);
        //DumpDebugMessage(temp);
        //if (bHardErrors)
        //    MessageBox(GetPluginWindow(), temp, WASABI_API_LNGSTRINGW_BUF(IDS_MILKDROP_ERROR, title, 64), MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
        //else
        //    AddError(temp, 8.0f, ERR_PRESET, true);
        return false;
    }

    ID3D11ShaderReflection* pReflection = nullptr;
    if (S_OK != D3DReflect(pShaderByteCode->GetBufferPointer(), pShaderByteCode->GetBufferSize(), IID_ID3D11ShaderReflection, reinterpret_cast<void**>(&pReflection)))
    {
        SafeRelease(m_pShaderCompileErrors);
        SafeRelease(pShaderByteCode);
        return false;
    }

    *ppConstTable = new CConstantTable(pReflection);

    HRESULT hr = 1;
    if (szProfile[0] == 'v')
    {
        hr = GetDevice()->CreateVertexShader(pShaderByteCode->GetBufferPointer(), pShaderByteCode->GetBufferSize(), reinterpret_cast<ID3D11VertexShader**>(ppShader), *ppConstTable);
    }
    else if (szProfile[0] == 'p')
    {
        hr = GetDevice()->CreatePixelShader(pShaderByteCode->GetBufferPointer(), pShaderByteCode->GetBufferSize(), reinterpret_cast<ID3D11PixelShader**>(ppShader), *ppConstTable);
    }

    if (hr != S_OK)
    {
        /*
        wchar_t temp[512] = {0};
        WASABI_API_LNGSTRINGW_BUF(IDS_ERROR_CREATING_SHADER, temp, sizeof(temp));
        DumpDebugMessage(temp);
        if (bHardErrors)
            MessageBox(GetPluginWindow(), temp, WASABI_API_LNGSTRINGW_BUF(IDS_MILKDROP_ERROR, title, 64), MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
        else
            AddError(temp, 6.0f, ERR_PRESET, true);
        */
        return false;
    }

    pShaderByteCode->Release();

    return true;
}

// Clean up all the DX11 textures, fonts, buffers, etc...
// EVERYTHING CREATED IN `AllocateMilkDropDX11()` SHOULD BE CLEANED UP HERE.
// The input parameter, `final_cleanup`, will be 0 if this is
// a routine cleanup (part of a window resize or switch between
// fullscreen/windowed modes), or 1 if this is the final cleanup
// and the plugin is exiting. Note that even if it is a routine
// cleanup, *still release ALL the DirectX stuff, because the
// DirectX device is being destroyed and recreated!*
// Also set all the pointers back to NULL;
// this is important because if reallocating the DX11 stuff later,
// and something fails, then CleanUp will get called,
// but it will then be trying to clean up invalid pointers.
// The `SafeRelease()` and `SafeDelete()` macros make the code prettier;
// they are defined here in "utility.h" as follows:
//   #define SafeRelease(x) if (x) { x->Release(); x = NULL; }
//   #define SafeDelete(x)  if (x) { delete x; x = NULL; }
// IMPORTANT:
// This function ISN'T only called when the plugin exits!
// It is also called whenever the user toggles between fullscreen and
// windowed modes, or resizes the window. Basically, on these events,
// the base class calls `CleanUpMilkDropDX11()` before resetting the DirectX
// device, and then calls `AllocateMilkDropDX11()` afterwards.
// One funky thing here: when switching between fullscreen and windowed,
// or doing any other thing that causes all this stuff to get reloaded in a second,
// then if blending 2 presets, jump fully to the new preset. Otherwise,
// the old preset wouldn't get all reloaded, and it app would crash
// when trying to use its stuff.
void CPlugin::CleanUpMilkDropDX11(int /* final_cleanup */)
{
    if (m_nLoadingPreset != 0)
    {
        // Finish up the pre-load & start the official blend.
        m_nLoadingPreset = 8;
        LoadPresetTick();
    }

    // Force this.
    m_pState->m_bBlending = false;

    for (size_t i = 0; i < m_textures.size(); i++)
        if (m_textures[i].texptr)
        {
            // Notify all CShaderParams classes that we're releasing a bindable texture!!
            for (auto const& j : global_CShaderParams_master_list)
                j->OnTextureEvict(m_textures[i].texptr);

            SafeRelease(m_textures[i].texptr);
        }
    m_textures.clear();

    // DON'T RELEASE blur textures - they were already released because they're in m_textures[].
#if (NUM_BLUR_TEX > 0)
    for (int i = 0; i < NUM_BLUR_TEX; i++)
        m_lpBlur[i] = NULL; //SafeRelease(m_lpBlur[i]);
#endif

    // NOTE: not necessary; shell does this for us.
    /*if (GetDevice())
    {
        GetDevice()->SetTexture(0, NULL);
        GetDevice()->SetTexture(1, NULL);
    }*/

    //SafeRelease(m_pMilkDropVertDecl);
    //SafeRelease(m_pWfVertDecl);
    //SafeRelease(m_pSpriteVertDecl);

    m_shaders.comp.Clear();
    m_shaders.warp.Clear();
    m_OldShaders.comp.Clear();
    m_OldShaders.warp.Clear();
    m_NewShaders.comp.Clear();
    m_NewShaders.warp.Clear();
    m_fallbackShaders_vs.comp.Clear();
    m_fallbackShaders_ps.comp.Clear();
    m_fallbackShaders_vs.warp.Clear();
    m_fallbackShaders_ps.warp.Clear();
    m_BlurShaders[0].vs.Clear();
    m_BlurShaders[0].ps.Clear();
    m_BlurShaders[1].vs.Clear();
    m_BlurShaders[1].ps.Clear();

    //SafeRelease(m_shaders.comp.ptr);
    //SafeRelease(m_shaders.warp.ptr);
    //SafeRelease(m_OldShaders.comp.ptr);
    //SafeRelease(m_OldShaders.warp.ptr);
    //SafeRelease(m_NewShaders.comp.ptr);
    //SafeRelease(m_NewShaders.warp.ptr);
    //SafeRelease(m_fallbackShaders_vs.comp.ptr);
    //SafeRelease(m_fallbackShaders_ps.comp.ptr);
    //SafeRelease(m_fallbackShaders_vs.warp.ptr);
    //SafeRelease(m_fallbackShaders_ps.warp.ptr);
    SafeRelease(m_pShaderCompileErrors);
    //SafeRelease(m_pCompiledFragments);
    //SafeRelease(m_pFragmentLinker);

    // 2. Release stuff.
    SafeRelease(m_lpVS[0]);
    SafeRelease(m_lpVS[1]);
    SafeRelease(m_lpDDSTitle);

    m_texmgr.Finish();

    if (m_verts != NULL)
    {
        delete m_verts;
        m_verts = NULL;
    }

    if (m_verts_temp != NULL)
    {
        delete m_verts_temp;
        m_verts_temp = NULL;
    }

    if (m_vertinfo != NULL)
    {
        delete m_vertinfo;
        m_vertinfo = NULL;
    }

    if (m_indices_list != NULL)
    {
        delete m_indices_list;
        m_indices_list = NULL;
    }

    if (m_indices_strip != NULL)
    {
        delete m_indices_strip;
        m_indices_strip = NULL;
    }

    //ClearErrors();

    // This setting is closely tied to the modern skin "random" button.
    // The "random" state should be preserved from session to session.
    // It's pretty safe to do, because the Scroll Lock key is hard to
    // accidentally click... :)
#ifndef _FOOBAR
    WritePrivateProfileInt(m_bPresetLockedByUser, L"bPresetLockOnAtStartup", GetConfigIniFile(), L"settings");
#endif
}

// Renders a frame of animation.
// This function is called each frame just AFTER `BeginScene()`.
// For timing information, call `GetTime()` and `GetFps()`.
// The usual formula is like this (but doesn't have to be):
//   1. Take care of timing, other paperwork, etc... for new frame
//   2. Clear the background
//   3. Get ready for 3D drawing
//   4. Draw 3D stuff
//   5. Call `PrepareFor2DDrawing()`
//   6. Draw your 2D stuff (overtop of the 3D scene).
// If the `redraw` flag is 1, try to redraw the last frame;
// `GetTime()`, `GetFps()`, and `GetFrame()` should all return the
// same values as they did on the last call to
// `MilkDropRenderFrame()`. Otherwise, the `redraw` flag will
// be zero and draw a new frame. The flag is
// used to force the desktop to repaint itself when
// running in desktop mode and Winamp is paused or stopped.
void CPlugin::MilkDropRenderFrame(int redraw)
{
    EnterCriticalSection(&g_cs);

    // 1a. Take care of timing, other paperwork, etc... for new frame.
    if (!redraw)
    {
        //float dt = GetTime() - m_prev_time;
        m_prev_time = GetTime(); // note: m_prev_time is not for general use!
        m_bPresetLockedByCode = (m_UI_mode != UI_REGULAR);
        if (m_bPresetLockedByUser || m_bPresetLockedByCode)
        {
            // To freeze time (at current preset time value) when menus are up or Scroll Lock is on.
            //m_fPresetStartTime += dt;
            //m_fNextPresetTime += dt;
            // OR, to freeze time @ [preset] zero, so that when you exit menus,
            //   you don't run the risk of it changing the preset on you right away:
            m_fPresetStartTime = GetTime();
            m_fNextPresetTime = -1.0f; // flags UpdateTime() to recompute this.
        }

        //if (!m_bPresetListReady)
        //    UpdatePresetList(true);//UpdatePresetRatings(); // read in a few each frame, til they're all in
    }

    // 1b. Check for lost or gained keyboard focus.
    // (note: can't use wm_setfocus or wm_killfocus because they don't work w/embedwnd)
    /*
    if (GetFrame() == 0)
    {
        // NOTE: we skip this if we've already gotten a WM_COMMAND/ID_VIS_RANDOM message
        //       from the skin - if that happened, we're running windowed with a fancy
        //       skin with a 'rand' button.

        SetScrollLock(m_bPresetLockOnAtStartup, m_bPreventScollLockHandling);

        // Make sure the 'random' button on the skin shows the right thing.
        // NEVERMIND - if it's a fancy skin, it'll send us WM_COMMAND/ID_VIS_RANDOM
        //   and we'll match the skin's Random button state.
        //SendMessage(GetWinampWindow(),WM_WA_IPC,m_bMilkdropScrollLockState, IPC_CB_VISRANDOM);
    }
    else
    {
        m_bHadFocus = m_bHasFocus;

        HWND winamp = GetWinampWindow();
        HWND plugin = GetPluginWindow();
        HWND focus = GetFocus();
        HWND cur = plugin;

        m_bHasFocus = false;
        do
        {
            m_bHasFocus = (focus == cur);
            if (m_bHasFocus)
                break;
            cur = GetParent(cur);
        } while (cur != NULL && cur != winamp);

        if (m_hTextWnd && focus == m_hTextWnd)
            m_bHasFocus = 1;

        if (GetFocus() == NULL)
            m_bHasFocus = 0;

        //HWND t1 = GetFocus();
        //HWND t2 = GetPluginWindow();
        //HWND t3 = GetParent(t2);

        if (m_bHadFocus == 1 && m_bHasFocus == 0)
        {
            //m_bMilkdropScrollLockState = GetKeyState(VK_SCROLL) & 1;
            SetScrollLock(m_bOrigScrollLockState, m_bPreventScollLockHandling);
        }
        else if (m_bHadFocus == 0 && m_bHasFocus == 1)
        {
            m_bOrigScrollLockState = GetKeyState(VK_SCROLL) & 1;
            SetScrollLock(m_bPresetLockedByUser, m_bPreventScollLockHandling);
        }
    }
    */

    // 2. Clear the background.
    //DWORD clear_color = (m_fog_enabled) ? FOG_COLOR : 0xFF000000;
    //GetDevice()->Clear(0, 0, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, clear_color, 1.0f, 0);

    // 5. Switch to 2D drawing mode.
    //    2D-coordinate system:
    //         +--------+ Y=-1
    //         |        |
    //         | screen |             Z=0: front of scene
    //         |        |             Z=1: back of scene
    //         +--------+ Y=1
    //       X=-1      X=1
    PrepareFor2DDrawing(GetDevice());

    if (!redraw)
        DoCustomSoundAnalysis(); // emulates old pre-VMS milkdrop sound analysis

    RenderFrame(redraw); // see "milkdropfs.cpp"

    if (!redraw)
    {
        m_nFramesSinceResize++;
        if (m_nLoadingPreset > 0)
        {
            LoadPresetTick();
        }
    }

    LeaveCriticalSection(&g_cs);
}

// clang-format off
#define MTO_UPPER_RIGHT 0
#define MTO_UPPER_LEFT 1
#define MTO_LOWER_RIGHT 2
#define MTO_LOWER_LEFT 3

#define SelectFont(n) { \
    pFont = GetFont(n); \
    h = GetFontHeight(n); \
}

#define MilkDropTextOut_Box(str, element, color, corner, bDarkBox, boxColor) { \
    D2D1_COLOR_F fText = D2D1::ColorF(color, GetAlpha(color)); \
    D2D1_COLOR_F fBox = D2D1::ColorF(boxColor, GetAlpha(boxColor)); \
    if (!element.IsVisible()) element.Initialize(m_lpDX->GetD2DDeviceContext()); \
    element.SetAlignment(AlignCenter, AlignCenter); \
    element.SetTextColor(fText); \
    element.SetTextOpacity(fText.a); \
    /* Calculate rendered rectangle size. */ \
    r = D2D1::RectF(0.0f, 0.0f, static_cast<FLOAT>(xR - xL), 2048.0f); \
    element.SetContainer(r); \
    element.SetText(str); \
    element.SetTextStyle(pFont); \
    element.SetTextShadow(false); \
    if (m_text.DrawD2DText(pFont, &element, static_cast<wchar_t*>(str), &r, DT_NOPREFIX | (corner == MTO_UPPER_RIGHT ? 0 : DT_SINGLELINE) | DT_WORD_ELLIPSIS | DT_CALCRECT | (corner == MTO_UPPER_RIGHT ? DT_RIGHT : 0), color, false, boxColor) != 0) { \
        int w = static_cast<int>(r.right - r.left); \
        if constexpr      (corner == MTO_UPPER_LEFT)  r = D2D1::RectF(static_cast<FLOAT>(xL), static_cast<FLOAT>(*upper_left_corner_y), static_cast<FLOAT>(xL + w), static_cast<FLOAT>(*upper_left_corner_y + h)); \
        else if constexpr (corner == MTO_UPPER_RIGHT) r = D2D1::RectF(static_cast<FLOAT>(xR - w), static_cast<FLOAT>(*upper_right_corner_y), static_cast<FLOAT>(xR), static_cast<FLOAT>(*upper_right_corner_y + h)); \
        else if constexpr (corner == MTO_LOWER_LEFT)  r = D2D1::RectF(static_cast<FLOAT>(xL), static_cast<FLOAT>(*lower_left_corner_y - h), static_cast<FLOAT>(xL + w), static_cast<FLOAT>(*lower_left_corner_y)); \
        else if constexpr (corner == MTO_LOWER_RIGHT) r = D2D1::RectF(static_cast<FLOAT>(xR - w), static_cast<FLOAT>(*lower_right_corner_y - h), static_cast<FLOAT>(xR), static_cast<FLOAT>(*lower_right_corner_y)); \
    } \
    /* Draw the text. */ \
    element.SetContainer(r); \
    element.SetTextBox(fBox, r); \
    if (m_text.DrawD2DText(pFont, &element, static_cast<wchar_t*>(str), &r, DT_NOPREFIX | (corner == MTO_UPPER_RIGHT ? 0 : DT_SINGLELINE) | DT_WORD_ELLIPSIS | (corner == MTO_UPPER_RIGHT ? DT_RIGHT : 0), color, bDarkBox, boxColor) != 0) { \
        if (!element.IsVisible()) m_text.RegisterElement(&element); \
        element.SetVisible(true); \
        if constexpr      (corner == MTO_UPPER_LEFT)  *upper_left_corner_y  += h; \
        else if constexpr (corner == MTO_UPPER_RIGHT) *upper_right_corner_y += h; \
        else if constexpr (corner == MTO_LOWER_LEFT)  *lower_left_corner_y  -= h; \
        else if constexpr (corner == MTO_LOWER_RIGHT) *lower_right_corner_y -= h; \
    } \
}

#define MilkDropTextOut(str, element, corner, bDarkBox) MilkDropTextOut_Box(str, element, 0xFFFFFFFF, corner, bDarkBox, 0xFF000000)

#define MilkDropTextOut_Shadow(str, element, color, corner) { \
    D2D1_COLOR_F fText = D2D1::ColorF(color, GetAlpha(color)); \
    if (!element.IsVisible()) element.Initialize(m_lpDX->GetD2DDeviceContext()); \
    element.SetAlignment(AlignCenter, AlignCenter); \
    element.SetTextColor(fText); \
    element.SetTextOpacity(fText.a); \
    /* Calculate rendered rectangle size. */ \
    r = D2D1::RectF(0.0f, 0.0f, static_cast<FLOAT>(xR - xL), 2048.0f); \
    element.SetContainer(r); \
    element.SetText(str); \
    element.SetTextStyle(pFont); \
    element.SetTextShadow(true); \
    if (m_text.DrawD2DText(pFont, &element, static_cast<wchar_t*>(str), &r, DT_NOPREFIX | DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_CALCRECT, color, false, 0xFF000000) != 0) { \
        int w = static_cast<int>(r.right - r.left); \
        if constexpr      (corner == MTO_UPPER_LEFT)  r = D2D1::RectF(static_cast<FLOAT>(xL), static_cast<FLOAT>(*upper_left_corner_y), static_cast<FLOAT>(xL + w), static_cast<FLOAT>(*upper_left_corner_y + h)); \
        else if constexpr (corner == MTO_UPPER_RIGHT) r = D2D1::RectF(static_cast<FLOAT>(xR - w), static_cast<FLOAT>(*upper_right_corner_y), static_cast<FLOAT>(xR), static_cast<FLOAT>(*upper_right_corner_y + h)); \
        else if constexpr (corner == MTO_LOWER_LEFT)  r = D2D1::RectF(static_cast<FLOAT>(xL), static_cast<FLOAT>(*lower_left_corner_y - h), static_cast<FLOAT>(xL + w), static_cast<FLOAT>(*lower_left_corner_y)); \
        else if constexpr (corner == MTO_LOWER_RIGHT) r = D2D1::RectF(static_cast<FLOAT>(xR - w), static_cast<FLOAT>(*lower_right_corner_y - h), static_cast<FLOAT>(xR), static_cast<FLOAT>(*lower_right_corner_y)); \
    } \
    /* Draw the text. */ \
    element.SetContainer(r); \
    if (m_text.DrawD2DText(pFont, &element, static_cast<wchar_t*>(str), &r, DT_NOPREFIX | DT_SINGLELINE | DT_WORD_ELLIPSIS, color, false, 0xFF000000) != 0) { \
        if (!element.IsVisible()) m_text.RegisterElement(&element); \
        element.SetVisible(true); \
        if constexpr      (corner == MTO_UPPER_LEFT)  *upper_left_corner_y  += h; \
        else if constexpr (corner == MTO_UPPER_RIGHT) *upper_right_corner_y += h; \
        else if constexpr (corner == MTO_LOWER_LEFT)  *lower_left_corner_y  -= h; \
        else if constexpr (corner == MTO_LOWER_RIGHT) *lower_right_corner_y -= h; \
    } \
}

#define MilkDropMenuOut_Box(top, line, font, str, r, flags, color, bDarkBox, boxColor) { \
    D2D1_RECT_F r2 = r; \
    D2D1_COLOR_F fText = D2D1::ColorF(color, GetAlpha(color)); \
    D2D1_COLOR_F fBox = D2D1::ColorF(boxColor, GetAlpha(boxColor)); \
    if (!m_menuText[line].IsVisible()) m_menuText[line].Initialize(m_lpDX->GetD2DDeviceContext()); \
    m_menuText[line].SetAlignment(AlignNear, AlignNear); \
    m_menuText[line].SetTextColor(fText); \
    m_menuText[line].SetTextOpacity(fText.a); \
    m_menuText[line].SetContainer(r); \
    m_menuText[line].SetText(str); \
    m_menuText[line].SetTextStyle(font); \
    m_menuText[line].SetTextShadow(false); \
    top += m_text.DrawD2DText(font, &m_menuText[line], static_cast<wchar_t*>(str), &r2, flags | DT_CALCRECT, color, bDarkBox, boxColor); \
    m_menuText[line].SetTextBox(fBox, r2); \
    if (!m_menuText[line].IsVisible()) { m_text.RegisterElement(&m_menuText[line]); m_menuText[line].SetVisible(true); } \
}

#define MilkDropStringOut_Box(top, element, font, str, r, flags, color, bDarkBox) { \
    D2D1_COLOR_F fText = D2D1::ColorF(color, GetAlpha(color)); \
    if (!element.IsVisible()) element.Initialize(m_lpDX->GetD2DDeviceContext()); \
    element.SetAlignment(AlignNear, AlignNear); \
    element.SetTextColor(fText); \
    element.SetTextOpacity(fText.a); \
    element.SetContainer(r); \
    element.SetText(str); \
    element.SetTextStyle(font); \
    element.SetTextShadow(false); \
    top += m_text.DrawD2DText(font, &element, static_cast<wchar_t*>(str), &r, flags, color, bDarkBox); \
    if (!element.IsVisible()) m_text.RegisterElement(&element); \
    element.SetVisible(true); \
}
// clang-format on

// Draws a string in the lower-right corner of the screen.
// Note: ID3DXFont handles `DT_RIGHT` and `DT_BOTTOM` *very poorly*.
//       It is best to calculate the size of the text first,
//       then place it in the right location.
// Note: Use `DT_WORDBREAK` instead of `DT_WORD_ELLIPSES`, otherwise
//       certain fonts' `DT_CALCRECT` (for the dark box) will be wrong.
void CPlugin::DrawTooltip(wchar_t* str, int xR, int yB)
{
    DWORD baseColor = 0xFFFFFFFF;
    D2D1_COLOR_F fgColor = D2D1::ColorF(baseColor);
    D2D1_COLOR_F bgColor = D2D1::ColorF(0x000000, GetAlpha(0xD0000000));

    m_toolTip.Initialize(m_lpDX->GetD2DDeviceContext());
    m_toolTip.SetAlignment(AlignCenter, AlignCenter);
    m_toolTip.SetTextColor(fgColor);
    m_toolTip.SetTextOpacity(fgColor.a);
    m_toolTip.SetVisible(true);
    m_toolTip.SetText(str);
    m_toolTip.SetTextStyle(GetFont(TOOLTIP_FONT));
    m_toolTip.SetTextShadow(false);
    D2D1_RECT_F r = D2D1::RectF(0.0f, 0.0f, static_cast<FLOAT>(xR - TEXT_MARGIN * 2), 2048.0f);
    m_toolTip.SetContainer(r);
    m_text.DrawD2DText(GetFont(TOOLTIP_FONT), &m_toolTip, str, &r, DT_CALCRECT, baseColor, false);
    D2D1_RECT_F r2{};
    r2.bottom = static_cast<FLOAT>(yB - TEXT_MARGIN);
    r2.right = static_cast<FLOAT>(xR - TEXT_MARGIN);
    r2.left = static_cast<FLOAT>(r2.right - (r.right - r.left));
    r2.top = static_cast<FLOAT>(r2.bottom - (r.bottom - r.top));
    D2D1_RECT_F r3 = r2;
    r3.left -= 4.0f;
    r3.top -= 2.0f;
    r3.right += 2.0f;
    r3.bottom += 2.0f;
    DrawDarkTranslucentBox(&r3);
    m_toolTip.SetTextBox(bgColor, r3);
    m_toolTip.SetContainer(r2);
    m_text.DrawD2DText(GetFont(TOOLTIP_FONT), &m_toolTip, str, &r2, 0, baseColor, false);
    m_text.RegisterElement(&m_toolTip);
}

void CPlugin::ClearTooltip()
{
    if (m_toolTip.IsVisible())
    {
        m_toolTip.SetVisible(false);
        m_text.UnregisterElement(&m_toolTip);
    }
}

void CPlugin::OnAltK()
{
    AddError(WASABI_API_LNGSTRINGW(IDS_PLEASE_EXIT_VIS_BEFORE_RUNNING_CONFIG_PANEL), 3.0f, ERR_NOTIFY, true);
}

void CPlugin::AddError(wchar_t* szMsg, float fDuration, ErrorCategory category, bool bBold)
{
    if (category == ERR_NOTIFY)
        ClearErrors(category);

    assert(category != ERR_ALL);
    ErrorMsg x;
    x.msg = szMsg;
    x.birthTime = GetTime();
    x.expireTime = GetTime() + fDuration;
    x.category = category;
    x.bold = bBold;
    x.printed = false;
    m_errors.push_back(x);
}

void CPlugin::ClearErrors(int category) // 0 = all categories
{
    for (ErrorMsgList::iterator it = m_errors.begin(); it != m_errors.end();)
        if (category == ERR_ALL || it->category == category)
        {
            if (it->text.IsVisible())
            {
                it->text.SetVisible(false);
                m_text.UnregisterElement(&it->text);
            }
            it = m_errors.erase(it);
        }
        else
            ++it;
}

// Draws text messages directly to the back buffer.
// When drawing text into one of the four corners,
// draw the text at the current 'y' value for that corner
// (one of the first 4 params to this function),
// and then adjust that 'y' value so that the next time
// text is drawn in that corner, it gets drawn above/below
// the previous text (instead of overtop of it).
// When drawing into the upper or lower LEFT corners,
// left-align the text to 'xL'.
// When drawing into the upper or lower RIGHT corners,
// right-align the text to 'xR'.
// Note: Try to keep the bounding rectangles on the text small;
//       the smaller the area that has to be locked (to draw the text),
//       the faster it will be.
// Note: To have some text be on the screen often that will not be
//       changing every frame, consider the poor folks whose video cards
//       hate that; in that case should probably draw the text just once,
//       to a texture, and then display the texture each frame. This is
//       how the help screen is done; see "pluginshell.cpp" for example.
void CPlugin::MilkDropRenderUI(int* upper_left_corner_y, int* upper_right_corner_y, int* lower_left_corner_y, int* lower_right_corner_y, int xL, int xR)
{
    D2D1_RECT_F r{};
    wchar_t buf[512] = {0};
    TextStyle* pFont = GetFont(DECORATIVE_FONT);
    int h = GetFontHeight(DECORATIVE_FONT);

    // 1. Render text in upper-right corner EXCEPT USER MESSAGE.
    //    The User Message goes last because it draws a box under itself
    //    and it should be visible over everything else (usually an error message).
    {
        // a) Preset name.
        if (m_bShowPresetInfo)
        {
            SelectFont(DECORATIVE_FONT);
            swprintf_s(buf, L"%s ", (m_nLoadingPreset != 0) ? m_pNewState->m_szDesc : m_pState->m_szDesc);
            MilkDropTextOut_Shadow(buf, m_presetName, 0xFFFFFFFF, MTO_UPPER_RIGHT);
        }
        else
        {
            if (m_presetName.IsVisible())
            {
                m_presetName.SetVisible(false);
                m_text.UnregisterElement(&m_presetName);
            }
        }

        // b) Preset rating.
        if (m_bShowRating || GetTime() < m_fShowRatingUntilThisTime)
        {
            // See also: `SetCurrentPresetRating()` in "milkdrop.cpp"
            SelectFont(DECORATIVE_FONT);
            swprintf_s(buf, L" %s: %d ", WASABI_API_LNGSTRINGW(IDS_RATING), (int)m_pState->m_fRating);
            if (!m_bEnableRating)
                wcscat_s(buf, WASABI_API_LNGSTRINGW(IDS_DISABLED));
            MilkDropTextOut_Shadow(buf, m_presetRating, 0xFFFFFFFF, MTO_UPPER_RIGHT);
        }
        else
        {
            if (m_presetRating.IsVisible())
            {
                m_presetRating.SetVisible(false);
                m_text.UnregisterElement(&m_presetRating);
            }
        }

        // c) FPS display.
        if (m_bShowFPS)
        {
            SelectFont(DECORATIVE_FONT);
            swprintf_s(buf, L"%s: %4.2f ", WASABI_API_LNGSTRINGW(IDS_FPS), GetFps()); // leave extra space at end, so italicized fonts do not get clipped
            MilkDropTextOut_Shadow(buf, m_fpsDisplay, 0xFFFFFFFF, MTO_UPPER_RIGHT);
        }
        else
        {
            if (m_fpsDisplay.IsVisible())
            {
                m_fpsDisplay.SetVisible(false);
                m_text.UnregisterElement(&m_fpsDisplay);
            }
        }

        // d) Debug information.
        if (m_bShowDebugInfo)
        {
            SelectFont(SIMPLE_FONT);
            swprintf_s(buf, L" %s: %6.4f ", WASABI_API_LNGSTRINGW(IDS_PF_MONITOR), static_cast<float>(*m_pState->var_pf_monitor));
            MilkDropTextOut_Shadow(buf, m_debugInfo, 0xFFFFFFFF, MTO_UPPER_RIGHT);
        }
        else
        {
            if (m_debugInfo.IsVisible())
            {
                m_debugInfo.SetVisible(false);
                m_text.UnregisterElement(&m_debugInfo);
            }
        }

        // NOTE: Custom timed message comes at the end!!
    }

    // 2. Render text in lower-right corner.
    {
        // "WaitString" tooltip.
        if (m_waitstring.bActive && m_bShowMenuToolTips && m_waitstring.szToolTip[0])
        {
            DrawTooltip(m_waitstring.szToolTip, xR, *lower_right_corner_y);
        }
        else
        {
            ClearTooltip();
        }
    }

    // 3. Render text in lower-left corner.
    {
        // Render song title in lower-left corner.
        if (m_bShowSongTitle)
        {
            wchar_t buf4[512] = {0};
            SelectFont(DECORATIVE_FONT);
            GetWinampSongTitle(GetWinampWindow(), buf4, ARRAYSIZE(buf4)); // defined in "support.h/cpp"
            MilkDropTextOut_Shadow(buf4, m_songTitle, 0xFFFFFFFF, MTO_LOWER_LEFT);
        }
        else
        {
            if (m_songTitle.IsVisible())
            {
                m_songTitle.SetVisible(false);
                m_text.UnregisterElement(&m_songTitle);
            }
        }

        // Render song time and length above that.
        if (m_bShowSongTime || m_bShowSongLen)
        {
            wchar_t buf2[511] = {0};
            wchar_t buf3[512] = {0}; // add extra space to end, so italicized fonts do not get clipped
            GetWinampSongPosAsText(GetWinampWindow(), buf); // defined in "support.h/cpp"
            GetWinampSongLenAsText(GetWinampWindow(), buf2); // defined in "support.h/cpp"
            if (m_bShowSongTime && m_bShowSongLen)
            {
                // Only show playing position and track length if it is playing (buffer is valid).
                if (buf[0])
                    swprintf_s(buf3, L"%s / %s ", buf, buf2);
                else
                    wcsncpy_s(buf3, buf2, ARRAYSIZE(buf2));
            }
            else if (m_bShowSongTime)
                wcsncpy_s(buf3, buf, ARRAYSIZE(buf2));
            else
                wcsncpy_s(buf3, buf2, ARRAYSIZE(buf2));

            SelectFont(DECORATIVE_FONT);
            MilkDropTextOut_Shadow(buf3, m_songStats, 0xFFFFFFFF, MTO_LOWER_LEFT);
        }
        else
        {
            if (m_songStats.IsVisible())
            {
                m_songStats.SetVisible(false);
                m_text.UnregisterElement(&m_songStats);
            }
        }
    }

    // 4. Render text in upper-left corner.
    {
        wchar_t buf0[65536] = {0}; // Must fit the longest strings (code strings are 32768 chars)
        char buf0A[65536] = {0};   // and leave extra space for &->&&, and [,[,& insertion
        size_t last_wait = 0;

        SelectFont(SIMPLE_FONT);

        // Loading presets, menus, etc.
        if (m_waitstring.bActive)
        {
            // 1. Draw the prompt string.
            MilkDropTextOut(m_waitstring.szPrompt, m_waitText[last_wait], MTO_UPPER_LEFT, true); last_wait++;

            // Extra instructions.
            bool bIsWarp = m_waitstring.bDisplayAsCode && (m_pCurMenu == &m_menuPreset) && !wcscmp(m_menuPreset.GetCurItem()->m_szName, L"[ edit warp shader ]");
            bool bIsComp = m_waitstring.bDisplayAsCode && (m_pCurMenu == &m_menuPreset) && !wcscmp(m_menuPreset.GetCurItem()->m_szName, L"[ edit composite shader ]");
            if (bIsWarp || bIsComp)
            {
                if (m_bShowShaderHelp)
                {
                    MilkDropTextOut(WASABI_API_LNGSTRINGW(IDS_PRESS_F9_TO_HIDE_SHADER_QREF), m_waitText[last_wait], MTO_UPPER_LEFT, true); last_wait++;
                }
                else
                {
                    MilkDropTextOut(WASABI_API_LNGSTRINGW(IDS_PRESS_F9_TO_SHOW_SHADER_QREF), m_waitText[last_wait], MTO_UPPER_LEFT, true); last_wait++;
                }
                *upper_left_corner_y += h * 2 / 3;

                if (m_bShowShaderHelp)
                {
                    // Draw dark box based on longest line and number of lines...
                    r = D2D1::RectF(0.0f, 0.0f, 2048.0f, 2048.0f);
                    D2D1_COLOR_F fgColor = D2D1::ColorF(0xFFFFFFFF, GetAlpha(0xFFFFFFFF));
                    D2D1_COLOR_F bgColor = D2D1::ColorF(0x000000, 0xD0 / 255.0f);
                    if (!m_waitText[last_wait].IsVisible())
                        m_waitText[last_wait].Initialize(m_lpDX->GetD2DDeviceContext());
                    m_waitText[last_wait].SetAlignment(AlignCenter, AlignCenter);
                    m_waitText[last_wait].SetTextColor(fgColor);
                    m_waitText[last_wait].SetTextOpacity(fgColor.a);
                    m_waitText[last_wait].SetContainer(r);
                    m_waitText[last_wait].SetText(WASABI_API_LNGSTRINGW(IDS_SHADER_HELP_LONG));
                    m_waitText[last_wait].SetTextStyle(pFont);
                    m_waitText[last_wait].SetTextShadow(false);
                    m_text.DrawD2DText(pFont, &m_waitText[last_wait], WASABI_API_LNGSTRINGW(IDS_SHADER_HELP_LONG), &r, DT_NOPREFIX | DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_CALCRECT, 0xFFFFFFFF, false);
                    D2D1_RECT_F darkbox = D2D1::RectF(static_cast<FLOAT>(xL), static_cast<FLOAT>(*upper_left_corner_y - 2), static_cast<FLOAT>(xL + r.right - r.left), static_cast<FLOAT>(*upper_left_corner_y + (r.bottom - r.top) * 13 + 2));
                    DrawDarkTranslucentBox(&darkbox);
                    m_waitText[last_wait].ReleaseDeviceDependentResources();

                    MilkDropTextOut(WASABI_API_LNGSTRINGW(IDS_SHADER_HELP0), m_waitText[last_wait], MTO_UPPER_LEFT, false); last_wait++;
                    MilkDropTextOut(WASABI_API_LNGSTRINGW(IDS_SHADER_HELP1), m_waitText[last_wait], MTO_UPPER_LEFT, false); last_wait++;
                    MilkDropTextOut(WASABI_API_LNGSTRINGW(IDS_SHADER_HELP2), m_waitText[last_wait], MTO_UPPER_LEFT, false); last_wait++;
                    MilkDropTextOut(WASABI_API_LNGSTRINGW(IDS_SHADER_HELP3), m_waitText[last_wait], MTO_UPPER_LEFT, false); last_wait++;
                    MilkDropTextOut(WASABI_API_LNGSTRINGW(IDS_SHADER_HELP4), m_waitText[last_wait], MTO_UPPER_LEFT, false); last_wait++;
                    MilkDropTextOut(WASABI_API_LNGSTRINGW(IDS_SHADER_HELP5), m_waitText[last_wait], MTO_UPPER_LEFT, false); last_wait++;
                    if (bIsWarp)
                    {
                        MilkDropTextOut(WASABI_API_LNGSTRINGW(IDS_WARP_HELP0), m_waitText[last_wait], MTO_UPPER_LEFT, false); last_wait++;
                        MilkDropTextOut(WASABI_API_LNGSTRINGW(IDS_WARP_HELP1), m_waitText[last_wait], MTO_UPPER_LEFT, false); last_wait++;
                        MilkDropTextOut(WASABI_API_LNGSTRINGW(IDS_WARP_HELP2), m_waitText[last_wait], MTO_UPPER_LEFT, false); last_wait++;
                        MilkDropTextOut(WASABI_API_LNGSTRINGW(IDS_WARP_HELP3), m_waitText[last_wait], MTO_UPPER_LEFT, false); last_wait++;
                        MilkDropTextOut(WASABI_API_LNGSTRINGW(IDS_WARP_HELP4), m_waitText[last_wait], MTO_UPPER_LEFT, false); last_wait++;
                        MilkDropTextOut(WASABI_API_LNGSTRINGW(IDS_WARP_HELP5), m_waitText[last_wait], MTO_UPPER_LEFT, false); last_wait++;
                        MilkDropTextOut(WASABI_API_LNGSTRINGW(IDS_WARP_HELP6), m_waitText[last_wait], MTO_UPPER_LEFT, false); last_wait++;
                    }
                    else if (bIsComp)
                    {
                        MilkDropTextOut(WASABI_API_LNGSTRINGW(IDS_COMP_HELP0), m_waitText[last_wait], MTO_UPPER_LEFT, false); last_wait++;
                        MilkDropTextOut(WASABI_API_LNGSTRINGW(IDS_COMP_HELP1), m_waitText[last_wait], MTO_UPPER_LEFT, false); last_wait++;
                        MilkDropTextOut(WASABI_API_LNGSTRINGW(IDS_COMP_HELP2), m_waitText[last_wait], MTO_UPPER_LEFT, false); last_wait++;
                        MilkDropTextOut(WASABI_API_LNGSTRINGW(IDS_COMP_HELP3), m_waitText[last_wait], MTO_UPPER_LEFT, false); last_wait++;
                        MilkDropTextOut(WASABI_API_LNGSTRINGW(IDS_COMP_HELP4), m_waitText[last_wait], MTO_UPPER_LEFT, false); last_wait++;
                        MilkDropTextOut(WASABI_API_LNGSTRINGW(IDS_COMP_HELP5), m_waitText[last_wait], MTO_UPPER_LEFT, false); last_wait++;
                        MilkDropTextOut(WASABI_API_LNGSTRINGW(IDS_COMP_HELP6), m_waitText[last_wait], MTO_UPPER_LEFT, false); last_wait++;
                    }
                    *upper_left_corner_y += h * 2 / 3;
                }
            }
            else if (m_UI_mode == UI_SAVEAS && (m_bWarpShaderLock || m_bCompShaderLock))
            {
                //wchar_t buf[256] = {0};
                int shader_msg_id = IDS_COMPOSITE_SHADER_LOCKED;
                if (m_bWarpShaderLock && m_bCompShaderLock)
                    shader_msg_id = IDS_WARP_AND_COMPOSITE_SHADERS_LOCKED;
                else if (m_bWarpShaderLock && !m_bCompShaderLock)
                    shader_msg_id = IDS_WARP_SHADER_LOCKED;
                else
                    shader_msg_id = IDS_COMPOSITE_SHADER_LOCKED;

                WASABI_API_LNGSTRINGW_BUF(shader_msg_id, buf, 256);
                MilkDropTextOut_Box(buf, m_waitText[last_wait], 0xFFFFFFFF, MTO_UPPER_LEFT, true, 0xFF000000); last_wait++;
                *upper_left_corner_y += h * 2 / 3;
            }
            else
                *upper_left_corner_y += h * 2 / 3;

            // 2. Reformat the waitstring text for display.
            int bBrackets = m_waitstring.nSelAnchorPos != -1 && m_waitstring.nSelAnchorPos != static_cast<int>(m_waitstring.nCursorPos);
            int bCursorBlink = (!bBrackets && ((int)(GetTime() * 270.0f) % 100 > 50)); //((GetFrame() % 3) >= 2)

            wcscpy_s(buf0, m_waitstring.szText);
            strcpy_s(buf0A, reinterpret_cast<char*>(m_waitstring.szText));

            size_t temp_cursor_pos = m_waitstring.nCursorPos;
            size_t temp_anchor_pos = m_waitstring.nSelAnchorPos;

            if (bBrackets)
            {
                if (m_waitstring.bDisplayAsCode)
                {
                    // Insert "[]" around the selection.
                    size_t start = (temp_cursor_pos < temp_anchor_pos) ? temp_cursor_pos : temp_anchor_pos;
                    size_t end = (temp_cursor_pos > temp_anchor_pos) ? temp_cursor_pos - 1 : temp_anchor_pos - 1;
                    size_t len = strnlen_s(buf0A, 65536);
                    size_t i;

                    for (i = len; i > end; i--)
                        buf0A[i + 1] = buf0A[i];
                    buf0A[end + 1] = ']';
                    len++;

                    for (i = len; i >= start; i--)
                        buf0A[i + 1] = buf0A[i];
                    buf0A[start] = '[';
                    len++;
                }
                else
                {
                    // Insert "[]" around the selection.
                    size_t start = (temp_cursor_pos < temp_anchor_pos) ? temp_cursor_pos : temp_anchor_pos;
                    size_t end = (temp_cursor_pos > temp_anchor_pos) ? temp_cursor_pos - 1 : temp_anchor_pos - 1;
                    size_t len = wcsnlen_s(buf0, 65536);
                    size_t i;

                    for (i = len; i > end; i--)
                        buf0[i + 1] = buf0[i];
                    buf[end + 1] = L']';
                    len++;

                    for (i = len; i >= start; i--)
                        buf0[i + 1] = buf0[i];
                    buf0[start] = L'[';
                    len++;
                }
            }
            else
            {
                // Underline the current cursor position by rapidly toggling the character with an underscore.
                if (m_waitstring.bDisplayAsCode)
                {
                    if (bCursorBlink)
                    {
                        if (buf0A[temp_cursor_pos] == '\0')
                        {
                            buf0A[temp_cursor_pos] = '_';
                            buf0A[temp_cursor_pos + 1] = '\0';
                        }
                        else if (buf0A[temp_cursor_pos] == LINEFEED_CONTROL_CHAR)
                        {
                            for (size_t i = strnlen_s(buf0A, 65536); i >= temp_cursor_pos; i--)
                                buf0A[i + 1] = buf0A[i];
                            buf0A[temp_cursor_pos] = '_';
                        }
                        else if (buf0A[temp_cursor_pos] == '_')
                            buf0A[temp_cursor_pos] = ' ';
                        else // it's a space or symbol or alphanumeric.
                            buf0A[temp_cursor_pos] = '_';
                    }
                    else
                    {
                        if (buf0A[temp_cursor_pos] == '\0')
                        {
                            buf0A[temp_cursor_pos] = ' ';
                            buf0A[temp_cursor_pos + 1] = '\0';
                        }
                        else if (buf0A[temp_cursor_pos] == LINEFEED_CONTROL_CHAR)
                        {
                            for (size_t i = strnlen_s(buf0A, 65536); i >= temp_cursor_pos; i--)
                                buf0A[i + 1] = buf0A[i];
                            buf0A[temp_cursor_pos] = ' ';
                        }
                        //else if (buf[temp_cursor_pos] == '_')
                        //    do nothing
                        //else // it's a space or symbol or alphanumeric.
                        //    do nothing
                    }
                }
                else
                {
                    if (bCursorBlink)
                    {
                        if (buf0[temp_cursor_pos] == L'\0')
                        {
                            buf0[temp_cursor_pos] = L'_';
                            buf0[temp_cursor_pos + 1] = L'\0';
                        }
                        else if (buf0[temp_cursor_pos] == LINEFEED_CONTROL_CHAR)
                        {
                            for (size_t i = wcsnlen_s(buf0, 65536); i >= temp_cursor_pos; i--)
                                buf0[i + 1] = buf0[i];
                            buf0[temp_cursor_pos] = L'_';
                        }
                        else if (buf0[temp_cursor_pos] == L'_')
                            buf0[temp_cursor_pos] = L' ';
                        else // it's a space or symbol or alphanumeric.
                            buf0[temp_cursor_pos] = L'_';
                    }
                    else
                    {
                        if (buf0[temp_cursor_pos] == L'\0')
                        {
                            buf0[temp_cursor_pos] = L' ';
                            buf0[temp_cursor_pos + 1] = L'\0';
                        }
                        else if (buf0[temp_cursor_pos] == LINEFEED_CONTROL_CHAR)
                        {
                            for (size_t i = wcsnlen_s(buf0, 65536); i >= temp_cursor_pos; i--)
                                buf0[i + 1] = buf0[i];
                            buf0[temp_cursor_pos] = L' ';
                        }
                        //else if (buf[temp_cursor_pos] == '_')
                        //    do nothing
                        //else // it's a space or symbol or alphanumeric.
                        //    do nothing
                    }
                }
            }

            D2D1_RECT_F rect = D2D1::RectF(static_cast<FLOAT>(xL), static_cast<FLOAT>(*upper_left_corner_y), static_cast<FLOAT>(xR), static_cast<FLOAT>(*lower_left_corner_y));
            rect.top += PLAYLIST_INNER_MARGIN;
            rect.left += PLAYLIST_INNER_MARGIN;
            rect.right -= PLAYLIST_INNER_MARGIN;
            rect.bottom -= PLAYLIST_INNER_MARGIN;

            // Then draw the edit string.
            if (m_waitstring.bDisplayAsCode)
            {
                char buf2[8192] = {0};
                int top_of_page_pos = 0;

                // Compute `top_of_page_pos` so that the line the cursor is on will show.
                // Also compute dimensions of the black rectangle.
                {
                    unsigned int start = 0;
                    unsigned int pos = 0;
                    float ypixels = 0.0f;
                    int page = 1;
                    int exit_on_next_page = 0;

                    D2D1_RECT_F box = rect;
                    box.right = box.left;
                    box.bottom = box.top;

                    while (buf0A[pos] != '\0') // for each line of text... (note that it might wrap)
                    {
                        start = pos;
                        while (buf0A[pos] != LINEFEED_CONTROL_CHAR && buf0A[pos] != '\0')
                            pos++;

                        char ch = buf0A[pos];
                        buf0A[pos] = '\0';
                        sprintf_s(buf2, "   %sX", &buf0A[start]); // put a final 'X' instead of ' ' because CALCRECT returns w==0 if string is entirely whitespace!
                        D2D1_RECT_F r2 = rect;
                        r2.bottom = 4096.0f;
                        D2D1_COLOR_F fgColor = D2D1::ColorF(0xFFFFFFFF, GetAlpha(0xFFFFFFFF));
                        if (!m_waitText[last_wait].IsVisible())
                            m_waitText[last_wait].Initialize(m_lpDX->GetD2DDeviceContext());
                        m_waitText[last_wait].SetAlignment(AlignCenter, AlignCenter);
                        m_waitText[last_wait].SetTextColor(fgColor);
                        m_waitText[last_wait].SetTextOpacity(fgColor.a);
                        m_waitText[last_wait].SetContainer(r2);
                        m_waitText[last_wait].SetText(AutoWide(buf2));
                        m_waitText[last_wait].SetTextStyle(GetFont(SIMPLE_FONT));
                        m_waitText[last_wait].SetTextShadow(false);
                        m_text.DrawD2DText(GetFont(SIMPLE_FONT), &m_waitText[last_wait], AutoWide(buf2), &r2, DT_CALCRECT /*| DT_WORDBREAK*/, 0xFFFFFFFF, false);
                        m_waitText[last_wait].ReleaseDeviceDependentResources();
                        float fH = r2.bottom - r2.top;
                        ypixels += fH;
                        buf0A[pos] = ch;

                        if (start > m_waitstring.nCursorPos) // make sure 'box' gets updated for each line on this page
                            exit_on_next_page = 1;

                        if (ypixels > rect.bottom - rect.top) // this line belongs on the next page
                        {
                            if (exit_on_next_page)
                            {
                                buf0A[start] = '\0'; // so text stops where the box stops, when we draw the text
                                break;
                            }

                            ypixels = fH;
                            top_of_page_pos = start;
                            page++;

                            box = rect;
                            box.right = box.left;
                            box.bottom = box.top;
                        }
                        box.bottom += fH;
                        box.right = std::max(box.right, box.left + r2.right - r2.left);

                        if (buf0A[pos] == '\0')
                            break;
                        pos++;
                    }

                    // Use `r2` to draw a dark box.
                    box.top -= PLAYLIST_INNER_MARGIN;
                    box.left -= PLAYLIST_INNER_MARGIN;
                    box.right += PLAYLIST_INNER_MARGIN;
                    box.bottom += PLAYLIST_INNER_MARGIN;
                    //DrawDarkTranslucentBox(&box);
                    *upper_left_corner_y += static_cast<int>(box.bottom - box.top + PLAYLIST_INNER_MARGIN * 3.0f);
                    swprintf_s(m_waitstring.szToolTip, WASABI_API_LNGSTRINGW(IDS_PAGE_X), page);
                }

                // Display multiline (replace all character 13s with a CR)
                {
                    unsigned int start = top_of_page_pos;
                    unsigned int pos = top_of_page_pos;

                    while (buf0A[pos] != '\0')
                    {
                        while (buf0A[pos] != LINEFEED_CONTROL_CHAR && buf0A[pos] != '\0')
                            pos++;

                        char ch = buf0A[pos];
                        buf0A[pos] = '\0';
                        sprintf_s(buf2, "   %s ", &buf0A[start]);
                        DWORD color = MENU_COLOR;
                        if (m_waitstring.nCursorPos >= start && m_waitstring.nCursorPos <= pos)
                            color = MENU_HILITE_COLOR;
                        MilkDropStringOut_Box(rect.top, m_waitText[last_wait], GetFont(SIMPLE_FONT), AutoWide(buf2), rect, static_cast<DWORD>(0) /*| DT_WORDBREAK*/, color, false); last_wait++;
                        buf0A[pos] = ch;

                        if (rect.top > rect.bottom)
                            break;

                        if (buf0A[pos] != '\0')
                            pos++;
                        start = pos;
                    }
                }
                // Note: `*upper_left_corner_y` is updated above, when the dark box is drawn.
            }
            else
            {
                wchar_t buf2[8192] = {0};

                // Display on one line.
                D2D1_RECT_F box = rect;
                D2D1_COLOR_F fgColor = D2D1::ColorF(MENU_COLOR, GetAlpha(MENU_COLOR));
                box.bottom = 4096.0f;
                swprintf_s(buf2, L"    %sX", buf0); // put a final 'X' instead of ' ' because `CALCRECT` returns zero width if string is entirely whitespace!
                if (!m_waitText[last_wait].IsVisible())
                    m_waitText[last_wait].Initialize(m_lpDX->GetD2DDeviceContext());
                m_waitText[last_wait].SetAlignment(AlignNear, AlignNear);
                m_waitText[last_wait].SetTextColor(fgColor);
                m_waitText[last_wait].SetTextOpacity(fgColor.a);
                m_waitText[last_wait].SetContainer(box);
                m_waitText[last_wait].SetText(buf2);
                m_waitText[last_wait].SetTextStyle(GetFont(SIMPLE_FONT));
                m_waitText[last_wait].SetTextShadow(false);
                m_text.DrawD2DText(GetFont(SIMPLE_FONT), &m_waitText[last_wait], buf2, &box, DT_CALCRECT, MENU_COLOR, false);
                if (!m_waitText[last_wait].IsVisible())
                    m_text.RegisterElement(&m_waitText[last_wait]);
                m_waitText[last_wait].SetVisible(true);

                // Use `box` to draw a dark box.
                box.top -= PLAYLIST_INNER_MARGIN;
                box.left -= PLAYLIST_INNER_MARGIN;
                box.right += PLAYLIST_INNER_MARGIN;
                box.bottom += PLAYLIST_INNER_MARGIN;
                DrawDarkTranslucentBox(&box);
                *upper_left_corner_y += static_cast<int>(box.bottom - box.top + PLAYLIST_INNER_MARGIN * 3.0f);

                swprintf_s(buf2, L"    %s ", buf0);
                m_text.DrawD2DText(GetFont(SIMPLE_FONT), &m_waitText[last_wait++], buf2, &rect, static_cast<DWORD>(0), MENU_COLOR, false);
            }
        }
        // clang-format off
        else if (m_UI_mode == UI_MENU)
        {
            assert(m_pCurMenu);
            r = D2D1::RectF(static_cast<FLOAT>(xL), static_cast<FLOAT>(*upper_left_corner_y), static_cast<FLOAT>(xR), static_cast<FLOAT>(*lower_left_corner_y));

            D2D1_RECT_F darkbox{};
            m_pCurMenu->DrawMenu(r, xR, *lower_right_corner_y, 1, &darkbox);
            *upper_left_corner_y += static_cast<int>(darkbox.bottom - darkbox.top + PLAYLIST_INNER_MARGIN * 3.0f);

            darkbox.right += PLAYLIST_INNER_MARGIN * 2.0f;
            darkbox.bottom += PLAYLIST_INNER_MARGIN * 2.0f;
            DrawDarkTranslucentBox(&darkbox);

            r.top += PLAYLIST_INNER_MARGIN;
            r.left += PLAYLIST_INNER_MARGIN;
            r.right += PLAYLIST_INNER_MARGIN;
            r.bottom += PLAYLIST_INNER_MARGIN;
            m_pCurMenu->DrawMenu(r, xR, *lower_right_corner_y);
        }
        else if (m_UI_mode == UI_UPGRADE_PIXEL_SHADER)
        {
            D2D1_RECT_F rect{};
            rect = D2D1::RectF(static_cast<FLOAT>(xL), static_cast<FLOAT>(*upper_left_corner_y), static_cast<FLOAT>(xR), static_cast<FLOAT>(*lower_left_corner_y));

            if (m_pState->m_nWarpPSVersion >= m_nMaxPSVersion && m_pState->m_nCompPSVersion >= m_nMaxPSVersion)
            {
                assert(m_pState->m_nMaxPSVersion == m_nMaxPSVersion);
                swprintf_s(buf, WASABI_API_LNGSTRINGW(IDS_PRESET_USES_HIGHEST_PIXEL_SHADER_VERSION), m_nMaxPSVersion);
                MilkDropMenuOut_Box(rect.top, 0, GetFont(SIMPLE_FONT), buf, rect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, MENU_COLOR, true, 0xFF000000);
                MilkDropMenuOut_Box(rect.top, 1, GetFont(SIMPLE_FONT), WASABI_API_LNGSTRINGW(IDS_PRESS_ESC_TO_RETURN), rect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, MENU_COLOR, true, 0xFF000000);
            }
            else
            {
                if (m_pState->m_nMinPSVersion != m_pState->m_nMaxPSVersion)
                {
                    switch (m_pState->m_nMinPSVersion)
                    {
                        case MD2_PS_NONE:
                            MilkDropMenuOut_Box(rect.top, 0, GetFont(SIMPLE_FONT), WASABI_API_LNGSTRINGW(IDS_PRESET_HAS_MIXED_VERSIONS_OF_SHADERS), rect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, MENU_COLOR, true, 0xFF000000);
                            MilkDropMenuOut_Box(rect.top, 1, GetFont(SIMPLE_FONT), WASABI_API_LNGSTRINGW(IDS_UPGRADE_SHADERS_TO_USE_PS2), rect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, MENU_COLOR, true, 0xFF000000);
                            break;
                        case MD2_PS_2_0:
                            MilkDropMenuOut_Box(rect.top, 0, GetFont(SIMPLE_FONT), WASABI_API_LNGSTRINGW(IDS_PRESET_HAS_MIXED_VERSIONS_OF_SHADERS), rect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, MENU_COLOR, true, 0xFF000000);
                            MilkDropMenuOut_Box(rect.top, 1, GetFont(SIMPLE_FONT), WASABI_API_LNGSTRINGW(IDS_UPGRADE_SHADERS_TO_USE_PS2X), rect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, MENU_COLOR, true, 0xFF000000);
                            break;
                        case MD2_PS_2_X:
                            MilkDropMenuOut_Box(rect.top, 0, GetFont(SIMPLE_FONT), WASABI_API_LNGSTRINGW(IDS_PRESET_HAS_MIXED_VERSIONS_OF_SHADERS), rect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, MENU_COLOR, true, 0xFF000000);
                            MilkDropMenuOut_Box(rect.top, 1, GetFont(SIMPLE_FONT), WASABI_API_LNGSTRINGW(IDS_UPGRADE_SHADERS_TO_USE_PS4), rect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, MENU_COLOR, true, 0xFF000000);
                            break;
                        case MD2_PS_3_0:
                        case MD2_PS_4_0:
                        case MD2_PS_5_0:
                        default:
                            assert(false);
                            break;
                    }
                }
                else
                {
                    switch (m_pState->m_nMinPSVersion)
                    {
                        case MD2_PS_NONE:
                            MilkDropMenuOut_Box(rect.top, 0, GetFont(SIMPLE_FONT), WASABI_API_LNGSTRINGW(IDS_PRESET_DOES_NOT_USE_PIXEL_SHADERS), rect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, MENU_COLOR, true, 0xFF000000);
                            MilkDropMenuOut_Box(rect.top, 1, GetFont(SIMPLE_FONT), WASABI_API_LNGSTRINGW(IDS_UPGRADE_TO_USE_PS2), rect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, MENU_COLOR, true, 0xFF000000);
                            MilkDropMenuOut_Box(rect.top, 2, GetFont(SIMPLE_FONT), WASABI_API_LNGSTRINGW(IDS_WARNING_OLD_GPU_MIGHT_NOT_WORK_WITH_PRESET), rect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, MENU_COLOR, true, 0xFF000000);
                            break;
                        case MD2_PS_2_0:
                            MilkDropMenuOut_Box(rect.top, 0, GetFont(SIMPLE_FONT), WASABI_API_LNGSTRINGW(IDS_PRESET_CURRENTLY_USES_PS2), rect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, MENU_COLOR, true, 0xFF000000);
                            MilkDropMenuOut_Box(rect.top, 1, GetFont(SIMPLE_FONT), WASABI_API_LNGSTRINGW(IDS_UPGRADE_TO_USE_PS2X), rect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, MENU_COLOR, true, 0xFF000000);
                            MilkDropMenuOut_Box(rect.top, 2, GetFont(SIMPLE_FONT), WASABI_API_LNGSTRINGW(IDS_WARNING_OLD_GPU_MIGHT_NOT_WORK_WITH_PRESET), rect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, MENU_COLOR, true, 0xFF000000);
                            break;
                        case MD2_PS_2_X:
                            MilkDropMenuOut_Box(rect.top, 0, GetFont(SIMPLE_FONT), WASABI_API_LNGSTRINGW(IDS_PRESET_CURRENTLY_USES_PS2X), rect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, MENU_COLOR, true, 0xFF000000);
                            MilkDropMenuOut_Box(rect.top, 1, GetFont(SIMPLE_FONT), WASABI_API_LNGSTRINGW(IDS_UPGRADE_TO_USE_PS3), rect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, MENU_COLOR, true, 0xFF000000);
                            MilkDropMenuOut_Box(rect.top, 2, GetFont(SIMPLE_FONT), WASABI_API_LNGSTRINGW(IDS_WARNING_OLD_GPU_MIGHT_NOT_WORK_WITH_PRESET), rect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, MENU_COLOR, true, 0xFF000000);
                            break;
                        case MD2_PS_3_0:
                            MilkDropMenuOut_Box(rect.top, 0, GetFont(SIMPLE_FONT), WASABI_API_LNGSTRINGW(IDS_PRESET_CURRENTLY_USES_PS3), rect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, MENU_COLOR, true, 0xFF000000);
                            MilkDropMenuOut_Box(rect.top, 1, GetFont(SIMPLE_FONT), WASABI_API_LNGSTRINGW(IDS_UPGRADE_TO_USE_PS4), rect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, MENU_COLOR, true, 0xFF000000);
                            MilkDropMenuOut_Box(rect.top, 2, GetFont(SIMPLE_FONT), WASABI_API_LNGSTRINGW(IDS_WARNING_OLD_GPU_MIGHT_NOT_WORK_WITH_PRESET), rect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, MENU_COLOR, true, 0xFF000000);
                            break;
                        default:
                            assert(false);
                            break;
                    }
                }
            }
            *upper_left_corner_y = static_cast<int>(rect.top);
        }
        else if (m_UI_mode == UI_LOAD_DEL)
        {
            h = GetFontHeight(SIMPLE_FONT);
            D2D1_RECT_F rect = D2D1::RectF(static_cast<FLOAT>(xL), static_cast<FLOAT>(*upper_left_corner_y), static_cast<FLOAT>(xR), static_cast<FLOAT>(*lower_left_corner_y));
            MilkDropMenuOut_Box(rect.top, 0, GetFont(SIMPLE_FONT), WASABI_API_LNGSTRINGW(IDS_ARE_YOU_SURE_YOU_WANT_TO_DELETE_PRESET), rect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, MENU_COLOR, true, 0xFF000000);
            swprintf_s(buf, WASABI_API_LNGSTRINGW(IDS_PRESET_TO_DELETE), m_presets[m_nPresetListCurPos].szFilename.c_str());
            MilkDropMenuOut_Box(rect.top, 1, GetFont(SIMPLE_FONT), buf, rect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, MENU_COLOR, true, 0xFF000000);
            *upper_left_corner_y = static_cast<int>(rect.top);
        }
        else if (m_UI_mode == UI_SAVE_OVERWRITE)
        {
            D2D1_RECT_F rect{};
            rect = D2D1::RectF(static_cast<FLOAT>(xL), static_cast<FLOAT>(*upper_left_corner_y), static_cast<FLOAT>(xR), static_cast<FLOAT>(*lower_left_corner_y));
            MilkDropMenuOut_Box(rect.top, 0, GetFont(SIMPLE_FONT), WASABI_API_LNGSTRINGW(IDS_FILE_ALREADY_EXISTS_OVERWRITE_IT), rect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, MENU_COLOR, true, 0xFF000000);
            swprintf_s(buf, WASABI_API_LNGSTRINGW(IDS_FILE_IN_QUESTION_X_MILK), m_waitstring.szText);
            MilkDropMenuOut_Box(rect.top, 1, GetFont(SIMPLE_FONT), buf, rect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, MENU_COLOR, true, 0xFF000000);
            if (m_bWarpShaderLock)
                MilkDropMenuOut_Box(rect.top, 2, GetFont(SIMPLE_FONT), WASABI_API_LNGSTRINGW(IDS_WARNING_DO_NOT_FORGET_WARP_SHADER_WAS_LOCKED), rect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, 0xFFFFFFFF, true, 0xFFCC0000);
            if (m_bCompShaderLock)
                MilkDropMenuOut_Box(rect.top, 3, GetFont(SIMPLE_FONT), WASABI_API_LNGSTRINGW(IDS_WARNING_DO_NOT_FORGET_COMPOSITE_SHADER_WAS_LOCKED), rect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, 0xFFFFFFFF, true, 0xFFCC0000);
            *upper_left_corner_y = static_cast<int>(rect.top);
        }
        else if (m_UI_mode == UI_MASHUP)
        {
            if (m_nPresets - m_nDirs == 0)
            {
                // Note: This error message is repeated in "milkdropfs.cpp" in `LoadRandomPreset()`.
                swprintf_s(buf, WASABI_API_LNGSTRINGW(IDS_ERROR_NO_PRESET_FILE_FOUND_IN_X_MILK), m_szPresetDir);
                AddError(buf, 6.0f, ERR_MISC, true);
                m_UI_mode = UI_REGULAR;
            }
            else
            {
                UpdatePresetList(); // make sure list is completely ready

                // Quick checks.
                for (int mash = 0; mash < MASH_SLOTS; mash++)
                {
                    // Check validity.
                    if (m_nMashPreset[mash] < m_nDirs)
                        m_nMashPreset[mash] = m_nDirs;
                    if (m_nMashPreset[mash] >= m_nPresets)
                        m_nMashPreset[mash] = m_nPresets - 1;

                    // Apply changes, if it's time.
                    if (m_nLastMashChangeFrame[mash] + MASH_APPLY_DELAY_FRAMES + 1 == static_cast<int>(GetFrame()))
                    {
                        // Import just a fragment of a preset!!
                        DWORD ApplyFlags = 0;
                        switch (mash)
                        {
                            case 0: ApplyFlags = STATE_GENERAL; break;
                            case 1: ApplyFlags = STATE_MOTION; break;
                            case 2: ApplyFlags = STATE_WAVE; break;
                            case 3: ApplyFlags = STATE_WARP; break;
                            case 4: ApplyFlags = STATE_COMP; break;
                        }

                        wchar_t szFile[MAX_PATH] = {0};
                        swprintf_s(szFile, L"%s%s", m_szPresetDir, m_presets[m_nMashPreset[mash]].szFilename.c_str());

                        m_pState->Import(szFile, GetTime(), m_pState, ApplyFlags);

                        if (ApplyFlags & STATE_WARP)
                            SafeRelease(m_shaders.warp.ptr);
                        if (ApplyFlags & STATE_COMP)
                            SafeRelease(m_shaders.comp.ptr);
                        LoadShaders(&m_shaders, m_pState, false);

                        SetMenusForPresetVersion(m_pState->m_nWarpPSVersion, m_pState->m_nCompPSVersion);
                    }
                }

                MilkDropTextOut(WASABI_API_LNGSTRINGW(IDS_PRESET_MASH_UP_TEXT1), m_menuText[0], MTO_UPPER_LEFT, true);
                MilkDropTextOut(WASABI_API_LNGSTRINGW(IDS_PRESET_MASH_UP_TEXT2), m_menuText[1], MTO_UPPER_LEFT, true);
                MilkDropTextOut(WASABI_API_LNGSTRINGW(IDS_PRESET_MASH_UP_TEXT3), m_menuText[2], MTO_UPPER_LEFT, true);
                MilkDropTextOut(WASABI_API_LNGSTRINGW(IDS_PRESET_MASH_UP_TEXT4), m_menuText[3], MTO_UPPER_LEFT, true);
                *upper_left_corner_y += static_cast<int>(PLAYLIST_INNER_MARGIN);

                D2D1_RECT_F rect{};
                rect = D2D1::RectF(static_cast<FLOAT>(xL), static_cast<FLOAT>(*upper_left_corner_y), static_cast<FLOAT>(xR), static_cast<FLOAT>(*lower_left_corner_y));
                rect.top += PLAYLIST_INNER_MARGIN;
                rect.left += PLAYLIST_INNER_MARGIN;
                rect.right -= PLAYLIST_INNER_MARGIN;
                rect.bottom -= PLAYLIST_INNER_MARGIN;

                int lines_available = static_cast<int>((rect.bottom - rect.top - PLAYLIST_INNER_MARGIN * 2) / GetFontHeight(SIMPLE_FONT));
                lines_available -= MASH_SLOTS;

                if (lines_available < 10)
                {
                    // Force it.
                    rect.bottom = rect.top + GetFontHeight(SIMPLE_FONT) * 10 + 1;
                    lines_available = 10;
                }
                if (lines_available > 16)
                    lines_available = 16;

                if (m_bUserPagedDown)
                {
                    m_nMashPreset[m_nMashSlot] += lines_available;
                    if (m_nMashPreset[m_nMashSlot] >= m_nPresets)
                        m_nMashPreset[m_nMashSlot] = m_nPresets - 1;
                    m_bUserPagedDown = false;
                }
                if (m_bUserPagedUp)
                {
                    m_nMashPreset[m_nMashSlot] -= lines_available;
                    if (m_nMashPreset[m_nMashSlot] < m_nDirs)
                        m_nMashPreset[m_nMashSlot] = m_nDirs;
                    m_bUserPagedUp = false;
                }

                int first_line = m_nMashPreset[m_nMashSlot] - (m_nMashPreset[m_nMashSlot] % lines_available);
                int last_line = first_line + lines_available;
                wchar_t str[512] = {0}, str2[512] = {0};

                if (last_line > m_nPresets)
                    last_line = m_nPresets;

                // Tooltip.
                if (m_bShowMenuToolTips)
                {
                    swprintf_s(buf, WASABI_API_LNGSTRINGW(IDS_PAGE_X_OF_X), m_nMashPreset[m_nMashSlot] / lines_available + 1, (m_nPresets + lines_available - 1) / lines_available);
                    DrawTooltip(buf, xR, *lower_right_corner_y);
                }
                
                D2D1_RECT_F orig_rect = rect;

                D2D1_RECT_F box{};
                box.top = rect.top;
                box.left = rect.left;
                box.right = rect.left;
                box.bottom = rect.top;

                int mashNames[MASH_SLOTS] = {
                    IDS_MASHUP_GENERAL_POSTPROC,
                    IDS_MASHUP_MOTION_EQUATIONS,
                    IDS_MASHUP_WAVEFORMS_SHAPES,
                    IDS_MASHUP_WARP_SHADER,
                    IDS_MASHUP_COMP_SHADER,
                };

                for (int pass = 0; pass < 2; pass++)
                {
                    box = orig_rect;
                    int width = 0;
                    int height = 0;

                    //int start_y = orig_rect.top;
                    for (int mash = 0; mash < MASH_SLOTS; mash++)
                    {
                        int idx = m_nMashPreset[mash];

                        swprintf_s(buf, L"%s%s", WASABI_API_LNGSTRINGW(mashNames[mash]), m_presets[idx].szFilename.c_str());
                        D2D1_RECT_F r2 = orig_rect;
                        r2.top += height;
                        height += m_text.DrawD2DText(GetFont(SIMPLE_FONT), &m_menuText[mash], buf, &r2, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX | (pass == 0 ? DT_CALCRECT : 0), (static_cast<WPARAM>(mash) == m_nMashSlot) ? PLAYLIST_COLOR_HILITE_TRACK : PLAYLIST_COLOR_NORMAL, false);
                        width = std::max(width, static_cast<int>(r2.right - r2.left));
                    }
                    if (pass == 0)
                    {
                        box.right = box.left + width;
                        box.bottom = box.top + height;
                        DrawDarkTranslucentBox(&box);
                    }
                    else
                        orig_rect.top += static_cast<FLOAT>(h);
                }

                orig_rect.top += GetFontHeight(SIMPLE_FONT) + PLAYLIST_INNER_MARGIN;

                box = orig_rect;
                box.right = box.left;
                box.bottom = box.top;

                // Draw a directory listing box right after...
                for (int pass = 0; pass < 2; pass++)
                {
                    //if (pass == 1)
                    //    GetFont(SIMPLE_FONT)->Begin();

                    rect = orig_rect;
                    for (int i = first_line; i < last_line; i++)
                    {
                        // Remove the extension before displaying the filename. Also pad with spaces.
                        //wcscpy_s(str, m_pPresetAddr[i]);
                        bool bIsDir = (m_presets[i].szFilename.c_str()[0] == L'*');
                        bool bIsRunning = false;
                        bool bIsSelected = (i == m_nMashPreset[m_nMashSlot]);

                        if (bIsDir)
                        {
                            // Directory.
                            if (wcscmp(m_presets[i].szFilename.c_str() + 1, L"..") == 0)
                                swprintf_s(str2, L" [ %s ] (%s) ", m_presets[i].szFilename.c_str() + 1, WASABI_API_LNGSTRINGW(IDS_PARENT_DIRECTORY));
                            else
                                swprintf_s(str2, L" [ %s ] ", m_presets[i].szFilename.c_str() + 1);
                        }
                        else
                        {
                            // Preset file.
                            wcscpy_s(str, m_presets[i].szFilename.c_str());
                            RemoveExtension(str);
                            swprintf_s(str2, L" %s ", str);

                            if (wcscmp(m_presets[m_nMashPreset[m_nMashSlot]].szFilename.c_str(), str) == 0)
                                bIsRunning = true;
                        }

                        if (bIsRunning && m_bPresetLockedByUser)
                            wcscat_s(str2, WASABI_API_LNGSTRINGW(IDS_LOCKED));

                        DWORD color = bIsDir ? DIR_COLOR : PLAYLIST_COLOR_NORMAL;
                        if (bIsRunning)
                            color = bIsSelected ? PLAYLIST_COLOR_BOTH : PLAYLIST_COLOR_PLAYING_TRACK;
                        else if (bIsSelected)
                            color = PLAYLIST_COLOR_HILITE_TRACK;

                        D2D1_RECT_F r2 = rect;
                        rect.top += m_text.DrawD2DText(GetFont(SIMPLE_FONT), &m_menuText[i], str2, &r2, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX | (pass == 0 ? DT_CALCRECT : 0), color, false);
                        if (pass == 0) // calculating dark box
                        {
                            box.right = std::max(box.right, box.left + r2.right - r2.left);
                            box.bottom += r2.bottom - r2.top;
                        }
                    }

                    //if (pass == 1)
                    //    GetFont(SIMPLE_FONT)->End();

                    if (pass == 0) // calculating dark box
                    {
                        box.top -= PLAYLIST_INNER_MARGIN;
                        box.left -= PLAYLIST_INNER_MARGIN;
                        box.right += PLAYLIST_INNER_MARGIN;
                        box.bottom += PLAYLIST_INNER_MARGIN;
                        DrawDarkTranslucentBox(&box);
                        *upper_left_corner_y = static_cast<int>(box.bottom + PLAYLIST_INNER_MARGIN);
                    }
                    else
                        orig_rect.top += box.bottom - box.top;
                }

                orig_rect.top += PLAYLIST_INNER_MARGIN;
            }
        }
        else if (m_UI_mode == UI_LOAD)
        {
            if (m_nPresets == 0)
            {
                // Note: This error message is repeated in "milkdropfs.cpp" in `LoadRandomPreset()`.
                wchar_t buf2[1024] = {0};
                swprintf_s(buf2, WASABI_API_LNGSTRINGW(IDS_ERROR_NO_PRESET_FILE_FOUND_IN_X_MILK), m_szPresetDir);
                AddError(buf2, 6.0f, ERR_MISC, true);
                m_UI_mode = UI_REGULAR;
            }
            else
            {
                SelectFont(TOOLTIP_FONT);
                MilkDropTextOut(WASABI_API_LNGSTRINGW(IDS_LOAD_WHICH_PRESET_PLUS_COMMANDS), m_loadPresetInstruction, MTO_UPPER_LEFT, true);

                wchar_t buf2[MAX_PATH + 64] = {0};
                swprintf_s(buf2, WASABI_API_LNGSTRINGW(IDS_DIRECTORY_OF_X), m_szPresetDir);
                MilkDropTextOut(buf2, m_loadPresetDir, MTO_UPPER_LEFT, true);

                *upper_left_corner_y += h / 2;

                D2D1_RECT_F rect = {
                    static_cast<FLOAT>(xL),
                    static_cast<FLOAT>(*upper_left_corner_y),
                    static_cast<FLOAT>(xR),
                    static_cast<FLOAT>(*lower_left_corner_y),
                };
                rect.top += PLAYLIST_INNER_MARGIN;
                rect.left += PLAYLIST_INNER_MARGIN;
                rect.right -= PLAYLIST_INNER_MARGIN;
                rect.bottom -= PLAYLIST_INNER_MARGIN;

                int lines_available = static_cast<int>(rect.bottom - rect.top - PLAYLIST_INNER_MARGIN * 2.0f) / GetFontHeight(SIMPLE_FONT);

                if (lines_available < 1)
                {
                    // Force it.
                    rect.bottom = rect.top + GetFontHeight(SIMPLE_FONT) + 1;
                    lines_available = 1;
                }
                if (lines_available > MAX_PRESETS_PER_PAGE)
                    lines_available = MAX_PRESETS_PER_PAGE;

                if (m_bUserPagedDown)
                {
                    m_nPresetListCurPos += lines_available;
                    if (m_nPresetListCurPos >= m_nPresets)
                        m_nPresetListCurPos = m_nPresets - 1;

                    // Remember this preset's name so the next time they hit 'L' it jumps straight to it.
                    //wcscpy_s(m_szLastPresetSelected, m_presets[m_nPresetListCurPos].szFilename.c_str());

                    m_bUserPagedDown = false;
                }

                if (m_bUserPagedUp)
                {
                    m_nPresetListCurPos -= lines_available;
                    if (m_nPresetListCurPos < 0)
                        m_nPresetListCurPos = 0;

                    // Remember this preset's name so the next time they hit 'L' it jumps straight to it
                    //wcscpy_s(m_szLastPresetSelected, m_presets[m_nPresetListCurPos].szFilename.c_str());

                    m_bUserPagedUp = false;
                }

                int first_line = m_nPresetListCurPos - (m_nPresetListCurPos % lines_available);
                int last_line = first_line + lines_available;

                if (last_line > m_nPresets)
                    last_line = m_nPresets;

                // Tooltip.
                if (m_bShowMenuToolTips)
                {
                    swprintf_s(buf2, WASABI_API_LNGSTRINGW(IDS_PAGE_X_OF_X), m_nPresetListCurPos / lines_available + 1, (m_nPresets + lines_available - 1) / lines_available);
                    DrawTooltip(buf2, xR, *lower_right_corner_y);
                }
                else
                {
                    ClearTooltip();
                }

                D2D1_RECT_F rect_hold = rect;

                D2D1_RECT_F box;
                box.top = static_cast<FLOAT>(rect.top);
                box.left = static_cast<FLOAT>(rect.left);
                box.right = static_cast<FLOAT>(rect.left);
                box.bottom = static_cast<FLOAT>(rect.top);

                wchar_t str[512] = {0}, str2[512] = {0};
                int nFontHeight = GetFontHeight(SIMPLE_FONT);
                for (int pass = 0; pass < 2; pass++)
                {
                    rect = rect_hold;
                    for (int i = first_line; i < last_line; i++)
                    {
                        D2D1_RECT_F r2 = rect;

                        if (pass == 0)
                        {
                            // Remove the extension before displaying the filename. Also pad with spaces.
                            //wcscpy_s(str, m_pPresetAddr[i]);
                            bool bIsDir = (m_presets[i].szFilename.c_str()[0] == '*');
                            bool bIsRunning = (i == m_nCurrentPreset); //false;
                            bool bIsSelected = (i == m_nPresetListCurPos);

                            if (bIsDir)
                            {
                                // Directory.
                                if (wcscmp(m_presets[i].szFilename.c_str() + 1, L"..") == 0)
                                    swprintf_s(str2, L" [ %s ] (%s) ", m_presets[i].szFilename.c_str() + 1, WASABI_API_LNGSTRINGW(IDS_PARENT_DIRECTORY));
                                else
                                    swprintf_s(str2, L" [ %s ] ", m_presets[i].szFilename.c_str() + 1);
                            }
                            else
                            {
                                // Preset file.
                                wcscpy_s(str, m_presets[i].szFilename.c_str());
                                RemoveExtension(str);
                                swprintf_s(str2, L" %s ", str);

                                //if (wcscmp(m_pState->m_szDesc, str) == 0)
                                //    bIsRunning = true;
                            }

                            if (bIsRunning && m_bPresetLockedByUser)
                                wcscat_s(str2, WASABI_API_LNGSTRINGW(IDS_LOCKED));

                            DWORD color = bIsDir ? DIR_COLOR : PLAYLIST_COLOR_NORMAL;
                            if (bIsRunning)
                                color = bIsSelected ? PLAYLIST_COLOR_BOTH : PLAYLIST_COLOR_PLAYING_TRACK;
                            else if (bIsSelected)
                                color = PLAYLIST_COLOR_HILITE_TRACK;

                            D2D1_COLOR_F fColor = D2D1::ColorF(color, GetAlpha(color));
                            m_loadPresetItem[i%MAX_PRESETS_PER_PAGE].Initialize(m_lpDX->GetD2DDeviceContext());
                            m_loadPresetItem[i%MAX_PRESETS_PER_PAGE].SetAlignment(AlignNear, AlignNear);
                            m_loadPresetItem[i%MAX_PRESETS_PER_PAGE].SetTextColor(fColor);
                            m_loadPresetItem[i%MAX_PRESETS_PER_PAGE].SetTextOpacity(fColor.a);
                            m_loadPresetItem[i%MAX_PRESETS_PER_PAGE].SetContainer(r2);
                            m_loadPresetItem[i%MAX_PRESETS_PER_PAGE].SetVisible(true);
                            m_loadPresetItem[i%MAX_PRESETS_PER_PAGE].SetText(str2);
                            m_loadPresetItem[i%MAX_PRESETS_PER_PAGE].SetTextStyle(GetFont(SIMPLE_FONT));
                            m_loadPresetItem[i%MAX_PRESETS_PER_PAGE].SetTextShadow(false);
                            int nHeight = m_text.DrawD2DText(GetFont(SIMPLE_FONT), &m_loadPresetItem[i%MAX_PRESETS_PER_PAGE], str2, &r2, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX | DT_CALCRECT, color, false);

                            // Calculate dark box rectangle.
                            box.right = std::max(box.right, box.left + r2.right - r2.left);
                            box.bottom += static_cast<FLOAT>(std::max(nFontHeight, nHeight)); //r2.bottom - r2.top;
                        }
                        else
                        {
                            m_loadPresetItem[i%MAX_PRESETS_PER_PAGE].SetContainer(r2);
                            int nHeight = m_text.DrawD2DText(GetFont(SIMPLE_FONT), &m_loadPresetItem[i%MAX_PRESETS_PER_PAGE], str2, &r2, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX, 0xFFFFFFFF, false);
                            rect.top += static_cast<FLOAT>(std::max(nFontHeight, nHeight));
                            m_text.RegisterElement(&m_loadPresetItem[i%MAX_PRESETS_PER_PAGE]);
                        }
                    }
                    for (int i = last_line; i < MAX_PRESETS_PER_PAGE * (m_nPresetListCurPos / lines_available + 1); i++)
                    {
                        if (pass == 0)
                        {
                            if (m_loadPresetItem[i%MAX_PRESETS_PER_PAGE].IsVisible())
                            {
                                m_loadPresetItem[i%MAX_PRESETS_PER_PAGE].SetVisible(false);
                                //m_text.UnregisterElement(&m_loadPresetItem[i]);
                            }
                        }
                    }

                    if (pass == 1) // calculate dark box rectangle (pass 0 in DirectX 9)
                    {
                        DWORD boxColor = 0xD0000000;
                        box.top -= PLAYLIST_INNER_MARGIN;
                        box.left -= PLAYLIST_INNER_MARGIN;
                        box.right += PLAYLIST_INNER_MARGIN;
                        box.bottom += PLAYLIST_INNER_MARGIN;
                        DrawDarkTranslucentBox(&box);
                        m_loadPresetItem[0].SetTextBox(D2D1::ColorF(boxColor, GetAlpha(boxColor)), box);
                        *upper_left_corner_y = static_cast<int>(box.bottom + PLAYLIST_INNER_MARGIN);
                    }
                }
            }
        }
    }
    // clang-format on

    // 5. Render *remaining* text to upper-right corner.
    {
        // e) Custom timed message.
        if (!m_bWarningsDisabled2)
        {
            SelectFont(SIMPLE_FONT);
            float t = GetTime();
#ifdef _FOOBAR
            // https://learn.microsoft.com/en-us/windows/win32/dataxchg/using-data-copy
            ErrorCopy msg;
            COPYDATASTRUCT cds{};
            cds.dwData = 0x09; // PRINT_CONSOLE
            cds.cbData = sizeof(msg);
            cds.lpData = &msg;
#endif
            for (ErrorMsgList::iterator it = m_errors.begin(); it != m_errors.end();)
            {
                if (t >= it->birthTime && t < it->expireTime)
                {
                    _snwprintf_s(buf, _TRUNCATE, L"%s ", it->msg.c_str());
                    float age_rel = (t - it->birthTime) / (it->expireTime - it->birthTime);
                    DWORD cr = static_cast<DWORD>(200 - 199 * powf(age_rel, 4));
                    DWORD cg = 0; //static_cast<DWORD>(136 - 135 * powf(age_rel, 1));
                    DWORD cb = 0;
                    DWORD z = 0xFF000000 | (cr << 16) | (cg << 8) | cb;
                    MilkDropTextOut_Box(buf, it->text, 0xFFFFFFFF, MTO_UPPER_RIGHT, true, it->bold ? z : 0xFF000000);
#ifdef _FOOBAR
                    if (!it->printed)
                    {
                        _snwprintf_s(msg.error, 1024, L"%s ", it->msg.c_str());
                        SendMessage(GetWinampWindow(), WM_COPYDATA, (WPARAM)(HWND)GetWinampWindow(), (LPARAM)(LPVOID)&cds);
                        it->printed = true;
                    }
#endif
                    ++it;
                }
                else
                {
                    if (it->text.IsVisible())
                    {
                        it->text.SetVisible(false);
                        m_text.UnregisterElement(&it->text);
                    }
                    it = m_errors.erase(it);
                }
            }
        }
    }
}

void CPlugin::ClearText()
{
    if (m_loadPresetInstruction.IsVisible())
    {
        m_loadPresetInstruction.SetVisible(false);
        m_text.UnregisterElement(&m_loadPresetInstruction);
    }
    if (m_loadPresetDir.IsVisible())
    {
        m_loadPresetDir.SetVisible(false);
        m_text.UnregisterElement(&m_loadPresetDir);
    }
    for (int i = 0; i < MAX_PRESETS_PER_PAGE; ++i)
    {
        if (m_waitText[i].IsVisible())
        {
            m_waitText[i].SetVisible(false);
            m_text.UnregisterElement(&m_waitText[i]);
        }
        if (i < MAX_PRESETS_PER_PAGE / 2)
        {
            if (m_menuText[i].IsVisible())
            {
                m_menuText[i].SetVisible(false);
                m_text.UnregisterElement(&m_menuText[i]);
            }
        }
        if (m_loadPresetItem[i].IsVisible())
        {
            m_loadPresetItem[i].SetVisible(false);
            m_text.UnregisterElement(&m_loadPresetItem[i]);
        }
    }
    m_pCurMenu->UndrawMenus();
}

void CPlugin::SetMenusForPresetVersion(int WarpPSVersion, int CompPSVersion)
{
    int MaxPSVersion = std::max(WarpPSVersion, CompPSVersion);

    m_menuPreset.EnableItem(WASABI_API_LNGSTRINGW(IDS_MENU_EDIT_WARP_SHADER), WarpPSVersion > 0);
    m_menuPreset.EnableItem(WASABI_API_LNGSTRINGW(IDS_MENU_EDIT_COMPOSITE_SHADER), CompPSVersion > 0);
    m_menuPost.EnableItem(WASABI_API_LNGSTRINGW(IDS_MENU_SUSTAIN_LEVEL), WarpPSVersion == 0);
    m_menuPost.EnableItem(WASABI_API_LNGSTRINGW(IDS_MENU_TEXTURE_WRAP), WarpPSVersion == 0);
    m_menuPost.EnableItem(WASABI_API_LNGSTRINGW(IDS_MENU_GAMMA_ADJUSTMENT), CompPSVersion == 0);
    m_menuPost.EnableItem(WASABI_API_LNGSTRINGW(IDS_MENU_HUE_SHADER), CompPSVersion == 0);
    m_menuPost.EnableItem(WASABI_API_LNGSTRINGW(IDS_MENU_VIDEO_ECHO_ALPHA), CompPSVersion == 0);
    m_menuPost.EnableItem(WASABI_API_LNGSTRINGW(IDS_MENU_VIDEO_ECHO_ZOOM), CompPSVersion == 0);
    m_menuPost.EnableItem(WASABI_API_LNGSTRINGW(IDS_MENU_VIDEO_ECHO_ORIENTATION), CompPSVersion == 0);
    m_menuPost.EnableItem(WASABI_API_LNGSTRINGW(IDS_MENU_FILTER_INVERT), CompPSVersion == 0);
    m_menuPost.EnableItem(WASABI_API_LNGSTRINGW(IDS_MENU_FILTER_BRIGHTEN), CompPSVersion == 0);
    m_menuPost.EnableItem(WASABI_API_LNGSTRINGW(IDS_MENU_FILTER_DARKEN), CompPSVersion == 0);
    m_menuPost.EnableItem(WASABI_API_LNGSTRINGW(IDS_MENU_FILTER_SOLARIZE), CompPSVersion == 0);
    m_menuPost.EnableItem(WASABI_API_LNGSTRINGW(IDS_MENU_BLUR1_EDGE_DARKEN_AMOUNT), MaxPSVersion > 0);
    m_menuPost.EnableItem(WASABI_API_LNGSTRINGW(IDS_MENU_BLUR1_MIN_COLOR_VALUE), MaxPSVersion > 0);
    m_menuPost.EnableItem(WASABI_API_LNGSTRINGW(IDS_MENU_BLUR1_MAX_COLOR_VALUE), MaxPSVersion > 0);
    m_menuPost.EnableItem(WASABI_API_LNGSTRINGW(IDS_MENU_BLUR2_MIN_COLOR_VALUE), MaxPSVersion > 0);
    m_menuPost.EnableItem(WASABI_API_LNGSTRINGW(IDS_MENU_BLUR2_MAX_COLOR_VALUE), MaxPSVersion > 0);
    m_menuPost.EnableItem(WASABI_API_LNGSTRINGW(IDS_MENU_BLUR3_MIN_COLOR_VALUE), MaxPSVersion > 0);
    m_menuPost.EnableItem(WASABI_API_LNGSTRINGW(IDS_MENU_BLUR3_MAX_COLOR_VALUE), MaxPSVersion > 0);
}

void CPlugin::BuildMenus()
{
    wchar_t buf[1024] = {0};

    m_pCurMenu = &m_menuPreset; //&m_menuMain;

    m_menuPreset.Init(WASABI_API_LNGSTRINGW(IDS_EDIT_CURRENT_PRESET));
    m_menuMotion.Init(WASABI_API_LNGSTRINGW(IDS_MOTION));
    m_menuCustomShape.Init(WASABI_API_LNGSTRINGW(IDS_DRAWING_CUSTOM_SHAPES));
    m_menuCustomWave.Init(WASABI_API_LNGSTRINGW(IDS_DRAWING_CUSTOM_WAVES));
    m_menuWave.Init(WASABI_API_LNGSTRINGW(IDS_DRAWING_SIMPLE_WAVEFORM));
    m_menuAugment.Init(WASABI_API_LNGSTRINGW(IDS_DRAWING_BORDERS_MOTION_VECTORS));
    m_menuPost.Init(WASABI_API_LNGSTRINGW(IDS_POST_PROCESSING_MISC));
    for (int i = 0; i < MAX_CUSTOM_WAVES; i++)
    {
        swprintf_s(buf, WASABI_API_LNGSTRINGW(IDS_CUSTOM_WAVE_X), i + 1);
        m_menuWavecode[i].Init(buf);
    }
    for (int i = 0; i < MAX_CUSTOM_SHAPES; i++)
    {
        swprintf_s(buf, WASABI_API_LNGSTRINGW(IDS_CUSTOM_SHAPE_X), i + 1);
        m_menuShapecode[i].Init(buf);
    }

    // MAIN MENU / menu hierarchy
    m_menuPreset.AddChildMenu(&m_menuMotion);
    m_menuPreset.AddChildMenu(&m_menuCustomShape);
    m_menuPreset.AddChildMenu(&m_menuCustomWave);
    m_menuPreset.AddChildMenu(&m_menuWave);
    m_menuPreset.AddChildMenu(&m_menuAugment);
    m_menuPreset.AddChildMenu(&m_menuPost);

    for (int i = 0; i < MAX_CUSTOM_SHAPES; i++)
        m_menuCustomShape.AddChildMenu(&m_menuShapecode[i]);
    for (int i = 0; i < MAX_CUSTOM_WAVES; i++)
        m_menuCustomWave.AddChildMenu(&m_menuWavecode[i]);

    // Note: all of the eval menu items use a CALLBACK function to register the user's changes (see last param).
    m_menuPreset.AddItem(WASABI_API_LNGSTRINGW(IDS_MENU_EDIT_PRESET_INIT_CODE), &m_pState->m_szPerFrameInit, MENUITEMTYPE_STRING, WASABI_API_LNGSTRINGW_BUF(IDS_MENU_EDIT_PRESET_INIT_CODE_TT, buf, 1024), 256, 0, &OnUserEditedPresetInit, sizeof(m_pState->m_szPerFrameInit), 0);
    m_menuPreset.AddItem(WASABI_API_LNGSTRINGW(IDS_MENU_EDIT_PER_FRAME_EQUATIONS), &m_pState->m_szPerFrameExpr, MENUITEMTYPE_STRING, WASABI_API_LNGSTRINGW_BUF(IDS_MENU_EDIT_PER_FRAME_EQUATIONS_TT, buf, 1024), 256, 0, &OnUserEditedPerFrame, sizeof(m_pState->m_szPerFrameExpr), 0);
    m_menuPreset.AddItem(WASABI_API_LNGSTRINGW(IDS_MENU_EDIT_PER_VERTEX_EQUATIONS), &m_pState->m_szPerPixelExpr, MENUITEMTYPE_STRING, WASABI_API_LNGSTRINGW_BUF(IDS_MENU_EDIT_PER_VERTEX_EQUATIONS_TT, buf, 1024), 256, 0, &OnUserEditedPerPixel, sizeof(m_pState->m_szPerPixelExpr), 0);
    m_menuPreset.AddItem(WASABI_API_LNGSTRINGW(IDS_MENU_EDIT_WARP_SHADER), &m_pState->m_szWarpShadersText, MENUITEMTYPE_STRING, WASABI_API_LNGSTRINGW_BUF(IDS_MENU_EDIT_WARP_SHADER_TT, buf, 1024), 256, 0, &OnUserEditedWarpShaders, sizeof(m_pState->m_szWarpShadersText), 0);
    m_menuPreset.AddItem(WASABI_API_LNGSTRINGW(IDS_MENU_EDIT_COMPOSITE_SHADER), &m_pState->m_szCompShadersText, MENUITEMTYPE_STRING, WASABI_API_LNGSTRINGW_BUF(IDS_MENU_EDIT_COMPOSITE_SHADER_TT, buf, 1024), 256, 0, &OnUserEditedCompShaders, sizeof(m_pState->m_szCompShadersText), 0);
    m_menuPreset.AddItem(WASABI_API_LNGSTRINGW(IDS_MENU_EDIT_UPGRADE_PRESET_PS_VERSION), (void*)UI_UPGRADE_PIXEL_SHADER, MENUITEMTYPE_UIMODE, WASABI_API_LNGSTRINGW_BUF(IDS_MENU_EDIT_UPGRADE_PRESET_PS_VERSION_TT, buf, 1024), 0, 0, NULL, UI_UPGRADE_PIXEL_SHADER, 0);
    m_menuPreset.AddItem(WASABI_API_LNGSTRINGW(IDS_MENU_EDIT_DO_A_PRESET_MASH_UP), (void*)UI_MASHUP, MENUITEMTYPE_UIMODE, WASABI_API_LNGSTRINGW_BUF(IDS_MENU_EDIT_DO_A_PRESET_MASH_UP_TT, buf, 1024), 0, 0, NULL, UI_MASHUP, 0);

    // Menu items.
#define MEN_T(id) WASABI_API_LNGSTRINGW(id)
#define MEN_TT(id) WASABI_API_LNGSTRINGW_BUF(id, buf, 1024)

    m_menuWave.AddItem(MEN_T(IDS_MENU_WAVE_TYPE), &m_pState->m_nWaveMode, MENUITEMTYPE_INT, MEN_TT(IDS_MENU_WAVE_TYPE_TT), 0, NUM_WAVES - 1);
    m_menuWave.AddItem(MEN_T(IDS_MENU_SIZE), &m_pState->m_fWaveScale, MENUITEMTYPE_LOGBLENDABLE, MEN_TT(IDS_MENU_SIZE_TT));
    m_menuWave.AddItem(MEN_T(IDS_MENU_SMOOTH), &m_pState->m_fWaveSmoothing, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_SMOOTH_TT), 0.0f, 0.9f);
    m_menuWave.AddItem(MEN_T(IDS_MENU_MYSTERY_PARAMETER), &m_pState->m_fWaveParam, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_MYSTERY_PARAMETER_TT), -1.0f, 1.0f);
    m_menuWave.AddItem(MEN_T(IDS_MENU_POSITION_X), &m_pState->m_fWaveX, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_POSITION_X_TT), 0, 1);
    m_menuWave.AddItem(MEN_T(IDS_MENU_POSITION_Y), &m_pState->m_fWaveY, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_POSITION_Y_TT), 0, 1);
    m_menuWave.AddItem(MEN_T(IDS_MENU_COLOR_RED), &m_pState->m_fWaveR, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_COLOR_RED_TT), 0, 1);
    m_menuWave.AddItem(MEN_T(IDS_MENU_COLOR_GREEN), &m_pState->m_fWaveG, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_COLOR_GREEN_TT), 0, 1);
    m_menuWave.AddItem(MEN_T(IDS_MENU_COLOR_BLUE), &m_pState->m_fWaveB, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_COLOR_BLUE_TT), 0, 1);
    m_menuWave.AddItem(MEN_T(IDS_MENU_OPACITY), &m_pState->m_fWaveAlpha, MENUITEMTYPE_LOGBLENDABLE, MEN_TT(IDS_MENU_OPACITY_TT), 0.001f, 100.0f);
    m_menuWave.AddItem(MEN_T(IDS_MENU_USE_DOTS), &m_pState->m_bWaveDots, MENUITEMTYPE_BOOL, MEN_TT(IDS_MENU_USE_DOTS_TT));
    m_menuWave.AddItem(MEN_T(IDS_MENU_DRAW_THICK), &m_pState->m_bWaveThick, MENUITEMTYPE_BOOL, MEN_TT(IDS_MENU_DRAW_THICK_TT));
    m_menuWave.AddItem(MEN_T(IDS_MENU_MODULATE_OPACITY_BY_VOLUME), &m_pState->m_bModWaveAlphaByVolume, MENUITEMTYPE_BOOL, MEN_TT(IDS_MENU_MODULATE_OPACITY_BY_VOLUME_TT));
    m_menuWave.AddItem(MEN_T(IDS_MENU_MODULATION_TRANSPARENT_VOLUME), &m_pState->m_fModWaveAlphaStart, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_MODULATION_TRANSPARENT_VOLUME_TT), 0.0f, 2.0f);
    m_menuWave.AddItem(MEN_T(IDS_MENU_MODULATION_OPAQUE_VOLUME), &m_pState->m_fModWaveAlphaEnd, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_MODULATION_OPAQUE_VOLUME_TT), 0.0f, 2.0f);
    m_menuWave.AddItem(MEN_T(IDS_MENU_ADDITIVE_DRAWING), &m_pState->m_bAdditiveWaves, MENUITEMTYPE_BOOL, MEN_TT(IDS_MENU_ADDITIVE_DRAWING_TT));
    m_menuWave.AddItem(MEN_T(IDS_MENU_COLOR_BRIGHTENING), &m_pState->m_bMaximizeWaveColor, MENUITEMTYPE_BOOL, MEN_TT(IDS_MENU_COLOR_BRIGHTENING_TT));

    m_menuAugment.AddItem(MEN_T(IDS_MENU_OUTER_BORDER_THICKNESS), &m_pState->m_fOuterBorderSize, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_OUTER_BORDER_THICKNESS_TT), 0, 0.5f);
    m_menuAugment.AddItem(MEN_T(IDS_MENU_COLOR_RED_OUTER), &m_pState->m_fOuterBorderR, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_COLOR_RED_OUTER_TT), 0, 1);
    m_menuAugment.AddItem(MEN_T(IDS_MENU_COLOR_GREEN_OUTER), &m_pState->m_fOuterBorderG, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_COLOR_GREEN_OUTER_TT), 0, 1);
    m_menuAugment.AddItem(MEN_T(IDS_MENU_COLOR_BLUE_OUTER), &m_pState->m_fOuterBorderB, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_COLOR_BLUE_OUTER_TT), 0, 1);
    m_menuAugment.AddItem(MEN_T(IDS_MENU_OPACITY_OUTER), &m_pState->m_fOuterBorderA, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_OPACITY_OUTER_TT), 0, 1);
    m_menuAugment.AddItem(MEN_T(IDS_MENU_INNER_BORDER_THICKNESS), &m_pState->m_fInnerBorderSize, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_INNER_BORDER_THICKNESS_TT), 0, 0.5f);
    m_menuAugment.AddItem(MEN_T(IDS_MENU_COLOR_RED_OUTER), &m_pState->m_fInnerBorderR, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_COLOR_RED_INNER_TT), 0, 1);
    m_menuAugment.AddItem(MEN_T(IDS_MENU_COLOR_GREEN_OUTER), &m_pState->m_fInnerBorderG, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_COLOR_GREEN_INNER_TT), 0, 1);
    m_menuAugment.AddItem(MEN_T(IDS_MENU_COLOR_BLUE_OUTER), &m_pState->m_fInnerBorderB, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_COLOR_BLUE_INNER_TT), 0, 1);
    m_menuAugment.AddItem(MEN_T(IDS_MENU_OPACITY_OUTER), &m_pState->m_fInnerBorderA, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_OPACITY_INNER_TT), 0, 1);
    m_menuAugment.AddItem(MEN_T(IDS_MENU_MOTION_VECTOR_OPACITY), &m_pState->m_fMvA, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_MOTION_VECTOR_OPACITY_TT), 0, 1);
    m_menuAugment.AddItem(MEN_T(IDS_MENU_NUM_MOT_VECTORS_X), &m_pState->m_fMvX, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_NUM_MOT_VECTORS_X_TT), 0, 64);
    m_menuAugment.AddItem(MEN_T(IDS_MENU_NUM_MOT_VECTORS_Y), &m_pState->m_fMvY, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_NUM_MOT_VECTORS_Y_TT), 0, 48);
    m_menuAugment.AddItem(MEN_T(IDS_MENU_OFFSET_X), &m_pState->m_fMvDX, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_OFFSET_X_TT), -1, 1);
    m_menuAugment.AddItem(MEN_T(IDS_MENU_OFFSET_Y), &m_pState->m_fMvDY, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_OFFSET_Y_TT), -1, 1);
    m_menuAugment.AddItem(MEN_T(IDS_MENU_TRAIL_LENGTH), &m_pState->m_fMvL, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_TRAIL_LENGTH_TT), 0, 5);
    m_menuAugment.AddItem(MEN_T(IDS_MENU_COLOR_RED_OUTER), &m_pState->m_fMvR, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_COLOR_RED_MOTION_VECTOR_TT), 0, 1);
    m_menuAugment.AddItem(MEN_T(IDS_MENU_COLOR_GREEN_OUTER), &m_pState->m_fMvG, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_COLOR_GREEN_MOTION_VECTOR_TT), 0, 1);
    m_menuAugment.AddItem(MEN_T(IDS_MENU_COLOR_BLUE_OUTER), &m_pState->m_fMvB, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_COLOR_BLUE_MOTION_VECTOR_TT), 0, 1);

    m_menuMotion.AddItem(MEN_T(IDS_MENU_ZOOM_AMOUNT), &m_pState->m_fZoom, MENUITEMTYPE_LOGBLENDABLE, MEN_TT(IDS_MENU_ZOOM_AMOUNT_TT));
    m_menuMotion.AddItem(MEN_T(IDS_MENU_ZOOM_EXPONENT), &m_pState->m_fZoomExponent, MENUITEMTYPE_LOGBLENDABLE, MEN_TT(IDS_MENU_ZOOM_EXPONENT_TT));
    m_menuMotion.AddItem(MEN_T(IDS_MENU_WARP_AMOUNT), &m_pState->m_fWarpAmount, MENUITEMTYPE_LOGBLENDABLE, MEN_TT(IDS_MENU_WARP_AMOUNT_TT));
    m_menuMotion.AddItem(MEN_T(IDS_MENU_WARP_SCALE), &m_pState->m_fWarpScale, MENUITEMTYPE_LOGBLENDABLE, MEN_TT(IDS_MENU_WARP_SCALE_TT));
    m_menuMotion.AddItem(MEN_T(IDS_MENU_WARP_SPEED), &m_pState->m_fWarpAnimSpeed, MENUITEMTYPE_LOGFLOAT, MEN_TT(IDS_MENU_WARP_SPEED_TT));
    m_menuMotion.AddItem(MEN_T(IDS_MENU_ROTATION_AMOUNT), &m_pState->m_fRot, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_ROTATION_AMOUNT_TT), -1.00f, 1.00f);
    m_menuMotion.AddItem(MEN_T(IDS_MENU_ROTATION_CENTER_OF_X), &m_pState->m_fRotCX, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_ROTATION_CENTER_OF_X_TT), -1.0f, 2.0f);
    m_menuMotion.AddItem(MEN_T(IDS_MENU_ROTATION_CENTER_OF_Y), &m_pState->m_fRotCY, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_ROTATION_CENTER_OF_Y_TT), -1.0f, 2.0f);
    m_menuMotion.AddItem(MEN_T(IDS_MENU_TRANSLATION_X), &m_pState->m_fXPush, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_TRANSLATION_X_TT), -1.0f, 1.0f);
    m_menuMotion.AddItem(MEN_T(IDS_MENU_TRANSLATION_Y), &m_pState->m_fYPush, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_TRANSLATION_Y_TT), -1.0f, 1.0f);
    m_menuMotion.AddItem(MEN_T(IDS_MENU_SCALING_X), &m_pState->m_fStretchX, MENUITEMTYPE_LOGBLENDABLE, MEN_TT(IDS_MENU_SCALING_X_TT));
    m_menuMotion.AddItem(MEN_T(IDS_MENU_SCALING_Y), &m_pState->m_fStretchY, MENUITEMTYPE_LOGBLENDABLE, MEN_TT(IDS_MENU_SCALING_Y_TT));

    m_menuPost.AddItem(MEN_T(IDS_MENU_SUSTAIN_LEVEL), &m_pState->m_fDecay, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_SUSTAIN_LEVEL_TT), 0.50f, 1.0f);
    m_menuPost.AddItem(MEN_T(IDS_MENU_DARKEN_CENTER), &m_pState->m_bDarkenCenter, MENUITEMTYPE_BOOL, MEN_TT(IDS_MENU_DARKEN_CENTER_TT));
    m_menuPost.AddItem(MEN_T(IDS_MENU_GAMMA_ADJUSTMENT), &m_pState->m_fGammaAdj, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_GAMMA_ADJUSTMENT_TT), 1.0f, 8.0f);
    m_menuPost.AddItem(MEN_T(IDS_MENU_HUE_SHADER), &m_pState->m_fShader, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_HUE_SHADER_TT), 0.0f, 1.0f);
    m_menuPost.AddItem(MEN_T(IDS_MENU_VIDEO_ECHO_ALPHA), &m_pState->m_fVideoEchoAlpha, MENUITEMTYPE_BLENDABLE, MEN_TT(IDS_MENU_VIDEO_ECHO_ALPHA_TT), 0.0f, 1.0f);
    m_menuPost.AddItem(MEN_T(IDS_MENU_VIDEO_ECHO_ZOOM), &m_pState->m_fVideoEchoZoom, MENUITEMTYPE_LOGBLENDABLE, MEN_TT(IDS_MENU_VIDEO_ECHO_ZOOM_TT));
    m_menuPost.AddItem(MEN_T(IDS_MENU_VIDEO_ECHO_ORIENTATION), &m_pState->m_nVideoEchoOrientation, MENUITEMTYPE_INT, MEN_TT(IDS_MENU_VIDEO_ECHO_ORIENTATION_TT), 0.0f, 3.0f);
    m_menuPost.AddItem(MEN_T(IDS_MENU_TEXTURE_WRAP), &m_pState->m_bTexWrap, MENUITEMTYPE_BOOL, MEN_TT(IDS_MENU_TEXTURE_WRAP_TT));
    //m_menuPost.AddItem("stereo 3D", &m_pState->m_bRedBlueStereo, MENUITEMTYPE_BOOL, "displays the image in stereo 3D; you need 3D glasses (with red and blue lenses) for this.");
    m_menuPost.AddItem(MEN_T(IDS_MENU_FILTER_INVERT), &m_pState->m_bInvert, MENUITEMTYPE_BOOL, MEN_TT(IDS_MENU_FILTER_INVERT_TT));
    m_menuPost.AddItem(MEN_T(IDS_MENU_FILTER_BRIGHTEN), &m_pState->m_bBrighten, MENUITEMTYPE_BOOL, MEN_TT(IDS_MENU_FILTER_BRIGHTEN_TT));
    m_menuPost.AddItem(MEN_T(IDS_MENU_FILTER_DARKEN), &m_pState->m_bDarken, MENUITEMTYPE_BOOL, MEN_TT(IDS_MENU_FILTER_DARKEN_TT));
    m_menuPost.AddItem(MEN_T(IDS_MENU_FILTER_SOLARIZE), &m_pState->m_bSolarize, MENUITEMTYPE_BOOL, MEN_TT(IDS_MENU_FILTER_SOLARIZE_TT));
    m_menuPost.AddItem(MEN_T(IDS_MENU_BLUR1_EDGE_DARKEN_AMOUNT), &m_pState->m_fBlur1EdgeDarken, MENUITEMTYPE_FLOAT, MEN_TT(IDS_MENU_BLUR1_EDGE_DARKEN_AMOUNT_TT), 0.0f, 1.0f);
    m_menuPost.AddItem(MEN_T(IDS_MENU_BLUR1_MIN_COLOR_VALUE), &m_pState->m_fBlur1Min, MENUITEMTYPE_FLOAT, MEN_TT(IDS_MENU_BLUR1_MIN_COLOR_VALUE_TT), 0.0f, 1.0f);
    m_menuPost.AddItem(MEN_T(IDS_MENU_BLUR1_MAX_COLOR_VALUE), &m_pState->m_fBlur1Max, MENUITEMTYPE_FLOAT, MEN_TT(IDS_MENU_BLUR1_MAX_COLOR_VALUE_TT), 0.0f, 1.0f);
    m_menuPost.AddItem(MEN_T(IDS_MENU_BLUR2_MIN_COLOR_VALUE), &m_pState->m_fBlur2Min, MENUITEMTYPE_FLOAT, MEN_TT(IDS_MENU_BLUR2_MIN_COLOR_VALUE_TT), 0.0f, 1.0f);
    m_menuPost.AddItem(MEN_T(IDS_MENU_BLUR2_MAX_COLOR_VALUE), &m_pState->m_fBlur2Max, MENUITEMTYPE_FLOAT, MEN_TT(IDS_MENU_BLUR2_MAX_COLOR_VALUE_TT), 0.0f, 1.0f);
    m_menuPost.AddItem(MEN_T(IDS_MENU_BLUR3_MIN_COLOR_VALUE), &m_pState->m_fBlur3Min, MENUITEMTYPE_FLOAT, MEN_TT(IDS_MENU_BLUR3_MIN_COLOR_VALUE_TT), 0.0f, 1.0f);
    m_menuPost.AddItem(MEN_T(IDS_MENU_BLUR3_MAX_COLOR_VALUE), &m_pState->m_fBlur3Max, MENUITEMTYPE_FLOAT, MEN_TT(IDS_MENU_BLUR3_MAX_COLOR_VALUE_TT), 0.0f, 1.0f);

    for (int i = 0; i < MAX_CUSTOM_WAVES; i++)
    {
        // Blending: do both; fade opacities in/out (with exaggerated weighting).
        m_menuWavecode[i].AddItem(MEN_T(IDS_MENU_ENABLED), &m_pState->m_wave[i].enabled, MENUITEMTYPE_BOOL, MEN_TT(IDS_MENU_ENABLED_TT));
        m_menuWavecode[i].AddItem(MEN_T(IDS_MENU_NUMBER_OF_SAMPLES), &m_pState->m_wave[i].samples, MENUITEMTYPE_INT, MEN_TT(IDS_MENU_NUMBER_OF_SAMPLES_TT), 2, 512); //0-512
        m_menuWavecode[i].AddItem(MEN_T(IDS_MENU_L_R_SEPARATION), &m_pState->m_wave[i].sep, MENUITEMTYPE_INT, MEN_TT(IDS_MENU_L_R_SEPARATION_TT), 0, 256); //0-512
        m_menuWavecode[i].AddItem(MEN_T(IDS_MENU_SCALING), &m_pState->m_wave[i].scaling, MENUITEMTYPE_LOGFLOAT, MEN_TT(IDS_MENU_SCALING_TT));
        m_menuWavecode[i].AddItem(MEN_T(IDS_MENU_SMOOTH), &m_pState->m_wave[i].smoothing, MENUITEMTYPE_FLOAT, MEN_TT(IDS_MENU_SMOOTHING_TT), 0, 1);
        m_menuWavecode[i].AddItem(MEN_T(IDS_MENU_COLOR_RED), &m_pState->m_wave[i].r, MENUITEMTYPE_FLOAT, MEN_TT(IDS_MENU_COLOR_RED_TT), 0, 1);
        m_menuWavecode[i].AddItem(MEN_T(IDS_MENU_COLOR_GREEN), &m_pState->m_wave[i].g, MENUITEMTYPE_FLOAT, MEN_TT(IDS_MENU_COLOR_GREEN_TT), 0, 1);
        m_menuWavecode[i].AddItem(MEN_T(IDS_MENU_COLOR_BLUE), &m_pState->m_wave[i].b, MENUITEMTYPE_FLOAT, MEN_TT(IDS_MENU_COLOR_BLUE_TT), 0, 1);
        m_menuWavecode[i].AddItem(MEN_T(IDS_MENU_OPACITY), &m_pState->m_wave[i].a, MENUITEMTYPE_FLOAT, MEN_TT(IDS_MENU_OPACITY_WAVE_TT), 0, 1);
        m_menuWavecode[i].AddItem(MEN_T(IDS_MENU_USE_SPECTRUM), &m_pState->m_wave[i].bSpectrum, MENUITEMTYPE_BOOL, MEN_TT(IDS_MENU_USE_SPECTRUM_TT)); //0-5 [0=wave left, 1=wave center, 2=wave right; 3=spectrum left, 4=spec center, 5=spec right]
        m_menuWavecode[i].AddItem(MEN_T(IDS_MENU_USE_DOTS), &m_pState->m_wave[i].bUseDots, MENUITEMTYPE_BOOL, MEN_TT(IDS_MENU_USE_DOTS_WAVE_TT));
        m_menuWavecode[i].AddItem(MEN_T(IDS_MENU_DRAW_THICK), &m_pState->m_wave[i].bDrawThick, MENUITEMTYPE_BOOL, MEN_TT(IDS_MENU_DRAW_THICK_WAVE_TT));
        m_menuWavecode[i].AddItem(MEN_T(IDS_MENU_ADDITIVE_DRAWING), &m_pState->m_wave[i].bAdditive, MENUITEMTYPE_BOOL, MEN_TT(IDS_MENU_ADDITIVE_DRAWING_WAVE_TT));
        m_menuWavecode[i].AddItem(MEN_T(IDS_MENU_EXPORT_TO_FILE), (void*)UI_EXPORT_WAVE, MENUITEMTYPE_UIMODE, MEN_TT(IDS_MENU_EXPORT_TO_FILE_TT), 0, 0, NULL, UI_EXPORT_WAVE, i);
        m_menuWavecode[i].AddItem(MEN_T(IDS_MENU_IMPORT_FROM_FILE), (void*)UI_IMPORT_WAVE, MENUITEMTYPE_UIMODE, MEN_TT(IDS_MENU_IMPORT_FROM_FILE_TT), 0, 0, NULL, UI_IMPORT_WAVE, i);
        m_menuWavecode[i].AddItem(MEN_T(IDS_MENU_EDIT_INIT_CODE), &m_pState->m_wave[i].m_szInit, MENUITEMTYPE_STRING, MEN_TT(IDS_MENU_EDIT_INIT_CODE_TT), 256, 0, &OnUserEditedWavecodeInit, sizeof(m_pState->m_wave[i].m_szInit), 0);
        m_menuWavecode[i].AddItem(MEN_T(IDS_MENU_EDIT_PER_FRAME_CODE), &m_pState->m_wave[i].m_szPerFrame, MENUITEMTYPE_STRING, MEN_TT(IDS_MENU_EDIT_PER_FRAME_CODE_TT), 256, 0, &OnUserEditedWavecode, sizeof(m_pState->m_wave[i].m_szPerFrame), 0);
        m_menuWavecode[i].AddItem(MEN_T(IDS_MENU_EDIT_PER_POINT_CODE), &m_pState->m_wave[i].m_szPerPoint, MENUITEMTYPE_STRING, MEN_TT(IDS_MENU_EDIT_PER_POINT_CODE_TT), 256, 0, &OnUserEditedWavecode, sizeof(m_pState->m_wave[i].m_szPerPoint), 0);
    }

    for (int i = 0; i < MAX_CUSTOM_SHAPES; i++)
    {
        // Blending: do both; fade opacities in/out (with exaggerated weighting).
        m_menuShapecode[i].AddItem(MEN_T(IDS_MENU_ENABLED), &m_pState->m_shape[i].enabled, MENUITEMTYPE_BOOL, MEN_TT(IDS_MENU_ENABLED_SHAPE_TT));
        m_menuShapecode[i].AddItem(MEN_T(IDS_MENU_NUMBER_OF_INSTANCES), &m_pState->m_shape[i].instances, MENUITEMTYPE_INT, MEN_TT(IDS_MENU_NUMBER_OF_INSTANCES_TT), 1, 1024);
        m_menuShapecode[i].AddItem(MEN_T(IDS_MENU_NUMBER_OF_SIDES), &m_pState->m_shape[i].sides, MENUITEMTYPE_INT, MEN_TT(IDS_MENU_NUMBER_OF_SIDES_TT), 3, 100);
        m_menuShapecode[i].AddItem(MEN_T(IDS_MENU_DRAW_THICK), &m_pState->m_shape[i].thickOutline, MENUITEMTYPE_BOOL, MEN_TT(IDS_MENU_DRAW_THICK_SHAPE_TT));
        m_menuShapecode[i].AddItem(MEN_T(IDS_MENU_ADDITIVE_DRAWING), &m_pState->m_shape[i].additive, MENUITEMTYPE_BOOL, MEN_TT(IDS_MENU_ADDITIVE_DRAWING_SHAPE_TT));
        m_menuShapecode[i].AddItem(MEN_T(IDS_MENU_X_POSITION), &m_pState->m_shape[i].x, MENUITEMTYPE_FLOAT, MEN_TT(IDS_MENU_X_POSITION_TT), 0, 1);
        m_menuShapecode[i].AddItem(MEN_T(IDS_MENU_Y_POSITION), &m_pState->m_shape[i].y, MENUITEMTYPE_FLOAT, MEN_TT(IDS_MENU_Y_POSITION_TT), 0, 1);
        m_menuShapecode[i].AddItem(MEN_T(IDS_MENU_RADIUS), &m_pState->m_shape[i].rad, MENUITEMTYPE_LOGFLOAT, MEN_TT(IDS_MENU_RADIUS_TT));
        m_menuShapecode[i].AddItem(MEN_T(IDS_MENU_ANGLE), &m_pState->m_shape[i].ang, MENUITEMTYPE_FLOAT, MEN_TT(IDS_MENU_ANGLE_TT), 0, 3.1415927f * 2.0f);
        m_menuShapecode[i].AddItem(MEN_T(IDS_MENU_TEXTURED), &m_pState->m_shape[i].textured, MENUITEMTYPE_BOOL, MEN_TT(IDS_MENU_TEXTURED_TT));
        m_menuShapecode[i].AddItem(MEN_T(IDS_MENU_TEXTURE_ZOOM), &m_pState->m_shape[i].tex_zoom, MENUITEMTYPE_LOGFLOAT, MEN_TT(IDS_MENU_TEXTURE_ZOOM_TT));
        m_menuShapecode[i].AddItem(MEN_T(IDS_MENU_TEXTURE_ANGLE), &m_pState->m_shape[i].tex_ang, MENUITEMTYPE_FLOAT, MEN_TT(IDS_MENU_TEXTURE_ANGLE_TT), 0, 3.1415927f * 2.0f);
        m_menuShapecode[i].AddItem(MEN_T(IDS_MENU_INNER_COLOR_RED), &m_pState->m_shape[i].r, MENUITEMTYPE_FLOAT, MEN_TT(IDS_MENU_INNER_COLOR_RED_TT), 0, 1);
        m_menuShapecode[i].AddItem(MEN_T(IDS_MENU_INNER_COLOR_GREEN), &m_pState->m_shape[i].g, MENUITEMTYPE_FLOAT, MEN_TT(IDS_MENU_INNER_COLOR_GREEN_TT), 0, 1);
        m_menuShapecode[i].AddItem(MEN_T(IDS_MENU_INNER_COLOR_BLUE), &m_pState->m_shape[i].b, MENUITEMTYPE_FLOAT, MEN_TT(IDS_MENU_INNER_COLOR_BLUE_TT), 0, 1);
        m_menuShapecode[i].AddItem(MEN_T(IDS_MENU_INNER_OPACITY), &m_pState->m_shape[i].a, MENUITEMTYPE_FLOAT, MEN_TT(IDS_MENU_INNER_OPACITY_TT), 0, 1);
        m_menuShapecode[i].AddItem(MEN_T(IDS_MENU_OUTER_COLOR_RED), &m_pState->m_shape[i].r2, MENUITEMTYPE_FLOAT, MEN_TT(IDS_MENU_OUTER_COLOR_RED_TT), 0, 1);
        m_menuShapecode[i].AddItem(MEN_T(IDS_MENU_OUTER_COLOR_GREEN), &m_pState->m_shape[i].g2, MENUITEMTYPE_FLOAT, MEN_TT(IDS_MENU_OUTER_COLOR_GREEN_TT), 0, 1);
        m_menuShapecode[i].AddItem(MEN_T(IDS_MENU_OUTER_COLOR_BLUE), &m_pState->m_shape[i].b2, MENUITEMTYPE_FLOAT, MEN_TT(IDS_MENU_OUTER_COLOR_BLUE_TT), 0, 1);
        m_menuShapecode[i].AddItem(MEN_T(IDS_MENU_OUTER_OPACITY), &m_pState->m_shape[i].a2, MENUITEMTYPE_FLOAT, MEN_TT(IDS_MENU_OUTER_OPACITY_TT), 0, 1);
        m_menuShapecode[i].AddItem(MEN_T(IDS_MENU_BORDER_COLOR_RED), &m_pState->m_shape[i].border_r, MENUITEMTYPE_FLOAT, MEN_TT(IDS_MENU_BORDER_COLOR_RED_TT), 0, 1);
        m_menuShapecode[i].AddItem(MEN_T(IDS_MENU_BORDER_COLOR_GREEN), &m_pState->m_shape[i].border_g, MENUITEMTYPE_FLOAT, MEN_TT(IDS_MENU_BORDER_COLOR_GREEN_TT), 0, 1);
        m_menuShapecode[i].AddItem(MEN_T(IDS_MENU_BORDER_COLOR_BLUE), &m_pState->m_shape[i].border_b, MENUITEMTYPE_FLOAT, MEN_TT(IDS_MENU_BORDER_COLOR_BLUE_TT), 0, 1);
        m_menuShapecode[i].AddItem(MEN_T(IDS_MENU_BORDER_OPACITY), &m_pState->m_shape[i].border_a, MENUITEMTYPE_FLOAT, MEN_TT(IDS_MENU_BORDER_OPACITY_TT), 0, 1);
        m_menuShapecode[i].AddItem(MEN_T(IDS_MENU_EXPORT_TO_FILE), NULL, MENUITEMTYPE_UIMODE, MEN_TT(IDS_MENU_EXPORT_TO_FILE_SHAPE_TT), 0, 0, NULL, UI_EXPORT_SHAPE, i);
        m_menuShapecode[i].AddItem(MEN_T(IDS_MENU_IMPORT_FROM_FILE), NULL, MENUITEMTYPE_UIMODE, MEN_TT(IDS_MENU_IMPORT_FROM_FILE_SHAPE_TT), 0, 0, NULL, UI_IMPORT_SHAPE, i);
        m_menuShapecode[i].AddItem(MEN_T(IDS_MENU_EDIT_INIT_CODE), &m_pState->m_shape[i].m_szInit, MENUITEMTYPE_STRING, MEN_TT(IDS_MENU_EDIT_INIT_CODE_SHAPE_TT), 256, 0, &OnUserEditedShapecodeInit, sizeof(m_pState->m_shape[i].m_szInit), 0);
        m_menuShapecode[i].AddItem(MEN_T(IDS_MENU_EDIT_PER_FRAME_INSTANCE_CODE), &m_pState->m_shape[i].m_szPerFrame, MENUITEMTYPE_STRING, MEN_TT(IDS_MENU_EDIT_PER_FRAME_INSTANCE_CODE_TT), 256, 0, &OnUserEditedShapecode, sizeof(m_pState->m_shape[i].m_szPerFrame), 0);
        //m_menuShapecode[i].AddItem("[ edit per-point code ]",&m_pState->m_shape[i].m_szPerPoint,  MENUITEMTYPE_STRING, "in: sample [0..1], value1 [left ch], value2 [right ch], plus all vars for per-frame code / out: x, y, r, g, b, a, t1-t8", 256, 0, &OnUserEditedWavecode);
    }
}

#ifndef _FOOBAR
void CPlugin::WriteRealtimeConfig()
{
    WritePrivateProfileInt(m_bShowFPS, L"bShowFPS", GetConfigIniFile(), L"settings");
    WritePrivateProfileInt(m_bShowRating, L"bShowRating", GetConfigIniFile(), L"settings");
    WritePrivateProfileInt(m_bShowPresetInfo, L"bShowPresetInfo", GetConfigIniFile(), L"settings");
    WritePrivateProfileInt(m_bShowSongTitle, L"bShowSongTitle", GetConfigIniFile(), L"settings");
    WritePrivateProfileInt(m_bShowSongTime, L"bShowSongTime", GetConfigIniFile(), L"settings");
    WritePrivateProfileInt(m_bShowSongLen, L"bShowSongLen", GetConfigIniFile(), L"settings");
}
#endif

//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------

void CPlugin::DumpDebugMessage(const wchar_t* s)
{
#ifdef _DEBUG
    OutputDebugString(s);
    if (s[0])
    {
        size_t len = wcslen(s);
        if (s[len - 1] != L'\n')
            OutputDebugString(L"\n");
    }
#else
    UNREFERENCED_PARAMETER(s);
#endif
}

void CPlugin::PopupMessage(int message_id, int title_id, bool dump)
{
#ifdef _DEBUG
    wchar_t buf[2048] = {0}, title[64] = {0};
    LoadString(GetInstance(), message_id, buf, 2048);
    LoadString(GetInstance(), title_id, title, 64);
    if (dump)
    {
        DumpDebugMessage(buf);
    }
    MessageBox(GetPluginWindow(), buf, title, MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
#else
    UNREFERENCED_PARAMETER(message_id);
    UNREFERENCED_PARAMETER(title_id);
    UNREFERENCED_PARAMETER(dump);
#endif
}

void CPlugin::ConsoleMessage(const wchar_t* function_name, int message_id, int title_id)
{
#ifdef _FOOBAR
    if (!SendMessage(GetWinampWindow(), WM_USER, MAKEWORD(0x21, 0x09), MAKELONG(message_id, title_id)))
    {
        // Retrieve the system error message for the last error code.
        LPVOID lpMsgBuf = NULL;
        LPVOID lpDisplayBuf = NULL;
        DWORD dw = GetLastError();

        if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                           NULL,
                           dw,
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                           (LPTSTR)&lpMsgBuf,
                           0,
                           NULL))
        {
            wprintf_s(L"Format message failed with 0x%x\n", GetLastError());
            return;
        }

        // Display the error message.
        // Buffer size: lpMsgBuf (includes CR+LF) + function_name + format (41) + dw (4) + '\0' (1)
        if ((lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)function_name) + 41 + 4 + 1) * sizeof(TCHAR))) == NULL)
            return;
        swprintf_s((LPTSTR)lpDisplayBuf, LocalSize(lpDisplayBuf) / sizeof(TCHAR), TEXT("foo_vis_milk2.dll: %s failed with error %d - %s"), function_name, dw, (LPCTSTR)lpMsgBuf);
        OutputDebugString((LPCTSTR)lpDisplayBuf); //MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

        LocalFree(lpMsgBuf);
        LocalFree(lpDisplayBuf);

        // Exit the process
        //ExitProcess(dw);
    }
#else
    UNREFERENCED_PARAMETER(message_id);
    UNREFERENCED_PARAMETER(title_id);
#endif
}

void ErrorOutput(LPCTSTR lpszFunction)
{
    // Retrieve the system error message for the last-error code.
    LPVOID lpMsgBuf = NULL;
    LPVOID lpDisplayBuf = NULL;
    DWORD dw = GetLastError();

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  dw,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR)&lpMsgBuf,
                  0,
                  NULL);

    // Display the error message.
    // Buffer size: lpMsgBuf (includes CR+LF) + lpszFunction + format (40) + dw (4) + '\0' (1)
    if ((lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40 + 4 + 1) * sizeof(TCHAR))) == NULL)
        return;
    swprintf_s((LPTSTR)lpDisplayBuf, LocalSize(lpDisplayBuf) / sizeof(TCHAR), TEXT("%s failed with error %d: %s"), lpszFunction, dw, (LPCTSTR)lpMsgBuf);
    OutputDebugString((LPCTSTR)lpDisplayBuf); //MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);

    // Exit the process
    //ExitProcess(dw);
}

void CPlugin::PrevPreset(float fBlendTime)
{
    if (m_bSequentialPresetOrder)
    {
        m_nCurrentPreset--;
        if (m_nCurrentPreset < m_nDirs)
            m_nCurrentPreset = m_nPresets - 1;
        if (m_nCurrentPreset >= m_nPresets) // just in case
            m_nCurrentPreset = m_nDirs;

        wchar_t szFile[MAX_PATH] = {0};
        wcscpy_s(szFile, m_szPresetDir); // note: m_szPresetDir always ends with '\'
        wcscat_s(szFile, m_presets[m_nCurrentPreset].szFilename.c_str());

        LoadPreset(szFile, fBlendTime);
    }
    else
    {
        int prev = (m_presetHistoryPos - 1 + PRESET_HIST_LEN) % PRESET_HIST_LEN;
        if (m_presetHistoryPos != m_presetHistoryBackFence)
        {
            m_presetHistoryPos = prev;
            LoadPreset(m_presetHistory[m_presetHistoryPos].c_str(), fBlendTime);
            SetPresetListPosition(m_presetHistory[m_presetHistoryPos]);
        }
    }
}

// If not retracing former steps, it will choose a random one.
void CPlugin::NextPreset(float fBlendTime)
{
    LoadRandomPreset(fBlendTime);
}

void CPlugin::LoadRandomPreset(float fBlendTime)
{
    // Ensure file list is OK.
    if (m_nPresets - m_nDirs == 0)
    {
        // Note: this error message is repeated in `milkdropfs.cpp` in `DrawText()`.
        wchar_t buf[1024] = {0};
        swprintf_s(buf, WASABI_API_LNGSTRINGW(IDS_ERROR_NO_PRESET_FILE_FOUND_IN_X_MILK), m_szPresetDir);
        AddError(buf, 6.0f, ERR_MISC, true);

        // Also bring up the directory navigation menu...
        if (m_UI_mode == UI_REGULAR || m_UI_mode == UI_MENU)
        {
            m_UI_mode = UI_LOAD;
            m_bUserPagedUp = false;
            m_bUserPagedDown = false;
        }
        return;
    }

    bool bHistoryEmpty = (m_presetHistoryFwdFence == m_presetHistoryBackFence);

    // If we have history to march back forward through, do that first.
    if (!m_bSequentialPresetOrder)
    {
        int next = (m_presetHistoryPos + 1) % PRESET_HIST_LEN;
        if (next != m_presetHistoryFwdFence && !bHistoryEmpty)
        {
            m_presetHistoryPos = next;
            LoadPreset(m_presetHistory[m_presetHistoryPos].c_str(), fBlendTime);
            SetPresetListPosition(m_presetHistory[m_presetHistoryPos]);
            return;
        }
    }

    // --TEMPORARY--
    // This comes in handy to mass-modify a batch of presets;
    // just automatically tweak values in Import, then they immediately get exported to a .MILK in a new dir.
    /*
    for (int i = 0; i < m_nPresets; i++)
    {
        wchar_t szPresetFile[512] = {0};
        wcscpy_s(szPresetFile, m_szPresetDir); // note: m_szPresetDir always ends with '\'
        wcscat_s(szPresetFile, m_pPresetAddr[i]);
        //CState newstate;
        m_state2.Import(szPresetFile, GetTime());

        wcscpy_s(szPresetFile, L"c:\\t7\\");
        wcscat_s(szPresetFile, m_pPresetAddr[i]);
        m_state2.Export(szPresetFile);
    }
    */
    // --[END]TEMPORARY--

    if (m_bSequentialPresetOrder)
    {
        m_nCurrentPreset++;
        if (m_nCurrentPreset < m_nDirs || m_nCurrentPreset >= m_nPresets)
            m_nCurrentPreset = m_nDirs;
    }
    else
    {
        // Pick a random file.
        if (!m_bEnableRating || (m_presets[static_cast<size_t>(m_nPresets) - 1].fRatingCum < 0.1f)) //|| (m_nRatingReadProgress < m_nPresets))
        {
            m_nCurrentPreset = m_nDirs + (warand() % (m_nPresets - m_nDirs));
        }
        else
        {
            float cdf_pos = (warand() % 14345) / 14345.0f * m_presets[static_cast<size_t>(m_nPresets) - 1].fRatingCum;

            /*
            char buf[512] = {0};
            sprintf_s(buf, "max = %f, rand = %f, \tvalues: ", m_presets[static_cast<size_t>(m_nPresets) - 1].fRatingCum, cdf_pos);
            for (int i=m_nDirs; i<m_nPresets; i++)
            {
                char buf2[32] = {0};
                sprintf_s(buf2, "%3.1f ", m_presets[i].fRatingCum);
                wcscat_s(buf, buf2);
            }
            DumpDebugMessage(buf);
            */

            if (cdf_pos < m_presets[m_nDirs].fRatingCum)
            {
                m_nCurrentPreset = m_nDirs;
            }
            else
            {
                int lo = m_nDirs;
                int hi = m_nPresets;
                while (lo + 1 < hi)
                {
                    int mid = (lo + hi) / 2;
                    if (m_presets[mid].fRatingCum > cdf_pos)
                        hi = mid;
                    else
                        lo = mid;
                }
                m_nCurrentPreset = hi;
            }
        }
    }

    // `m_pPresetAddr[m_nCurrentPreset]` points to the preset file to load (without the path);
    // first prepend the path, then load section [preset00] within that file.
    wchar_t szFile[MAX_PATH] = {0};
    wcscpy_s(szFile, m_szPresetDir); // note: m_szPresetDir always ends with '\'
    wcscat_s(szFile, m_presets[m_nCurrentPreset].szFilename.c_str());

    if (!bHistoryEmpty)
        m_presetHistoryPos = (m_presetHistoryPos + 1) % PRESET_HIST_LEN;

    LoadPreset(szFile, fBlendTime);
}

void CPlugin::RandomizeBlendPattern()
{
    if (!m_vertinfo)
        return;

    // Note: now avoid constant uniform blend because it is half-speed for shader blending.
    //       (both old and new shaders would have to run on every pixel...)
    int mixtype = 1 + (warand() % 3); //warand()%4;

    if (mixtype == 0)
    {
        // Constant, uniform blend.
        int nVert = 0;
        for (int y = 0; y <= m_nGridY; y++)
        {
            for (int x = 0; x <= m_nGridX; x++)
            {
                m_vertinfo[nVert].a = 1;
                m_vertinfo[nVert].c = 0;
                nVert++;
            }
        }
    }
    else if (mixtype == 1)
    {
        // Directional wipe.
        float ang = FRAND * 6.28f;
        float vx = cosf(ang);
        float vy = sinf(ang);
        float band = 0.1f + 0.2f * FRAND; // 0.2 is good
        float inv_band = 1.0f / band;

        int nVert = 0;
        for (int y = 0; y <= m_nGridY; y++)
        {
            float fy = (y / (float)m_nGridY) * m_fAspectY;
            for (int x = 0; x <= m_nGridX; x++)
            {
                float fx = (x / (float)m_nGridX) * m_fAspectX;

                // at t==0, mix rangse from -10..0
                // at t==1, mix ranges from   1..11

                float t = (fx - 0.5f) * vx + (fy - 0.5f) * vy + 0.5f;
                t = (t - 0.5f) / sqrtf(2.0f) + 0.5f;

                m_vertinfo[nVert].a = inv_band * (1 + band);
                m_vertinfo[nVert].c = -inv_band + inv_band * t; //(x/(float)m_nGridX - 0.5f)/band;
                nVert++;
            }
        }
    }
    else if (mixtype == 2)
    {
        // Plasma transition.
        float band = 0.12f + 0.13f * FRAND; //0.02f + 0.18f*FRAND;
        float inv_band = 1.0f / band;

        // First generate plasma array of height values.
        m_vertinfo[0].c = FRAND;
        m_vertinfo[m_nGridX].c = FRAND;
        m_vertinfo[m_nGridY * (m_nGridX + 1)].c = FRAND;
        m_vertinfo[m_nGridY * (m_nGridX + 1) + m_nGridX].c = FRAND;
        GenPlasma(0, m_nGridX, 0, m_nGridY, 0.25f);

        // then find min,max so we can normalize to [0..1] range and then to the proper 'constant offset' range.
        float minc = m_vertinfo[0].c;
        float maxc = m_vertinfo[0].c;
        int x, y, nVert;

        nVert = 0;
        for (y = 0; y <= m_nGridY; y++)
        {
            for (x = 0; x <= m_nGridX; x++)
            {
                if (minc > m_vertinfo[nVert].c)
                    minc = m_vertinfo[nVert].c;
                if (maxc < m_vertinfo[nVert].c)
                    maxc = m_vertinfo[nVert].c;
                nVert++;
            }
        }

        float mult = 1.0f / (maxc - minc);
        nVert = 0;
        for (y = 0; y <= m_nGridY; y++)
        {
            for (x = 0; x <= m_nGridX; x++)
            {
                float t = (m_vertinfo[nVert].c - minc) * mult;
                m_vertinfo[nVert].a = inv_band * (1 + band);
                m_vertinfo[nVert].c = -inv_band + inv_band * t;
                nVert++;
            }
        }
    }
    else if (mixtype == 3)
    {
        // Radial blend.
        float band = 0.02f + 0.14f * FRAND + 0.34f * FRAND;
        float inv_band = 1.0f / band;
        float dir = (float)((warand() % 2) * 2 - 1); // 1=outside-in, -1=inside-out

        int nVert = 0;
        for (int y = 0; y <= m_nGridY; y++)
        {
            float dy = (y / (float)m_nGridY - 0.5f) * m_fAspectY;
            for (int x = 0; x <= m_nGridX; x++)
            {
                float dx = (x / (float)m_nGridX - 0.5f) * m_fAspectX;
                float t = sqrtf(dx * dx + dy * dy) * 1.41421f;
                if (dir == -1)
                    t = 1 - t;

                m_vertinfo[nVert].a = inv_band * (1 + band);
                m_vertinfo[nVert].c = -inv_band + inv_band * t;
                nVert++;
            }
        }
    }
}

void CPlugin::GenPlasma(int x0, int x1, int y0, int y1, float dt)
{
    int midx = (x0 + x1) / 2;
    int midy = (y0 + y1) / 2;
    float t00 = m_vertinfo[y0 * (m_nGridX + 1) + x0].c;
    float t01 = m_vertinfo[y0 * (m_nGridX + 1) + x1].c;
    float t10 = m_vertinfo[y1 * (m_nGridX + 1) + x0].c;
    float t11 = m_vertinfo[y1 * (m_nGridX + 1) + x1].c;

    if (y1 - y0 >= 2)
    {
        if (x0 == 0)
            m_vertinfo[midy * (m_nGridX + 1) + x0].c = 0.5f * (t00 + t10) + (FRAND * 2 - 1) * dt * m_fAspectY;
        m_vertinfo[midy * (m_nGridX + 1) + x1].c = 0.5f * (t01 + t11) + (FRAND * 2 - 1) * dt * m_fAspectY;
    }
    if (x1 - x0 >= 2)
    {
        if (y0 == 0)
            m_vertinfo[y0 * (m_nGridX + 1) + midx].c = 0.5f * (t00 + t01) + (FRAND * 2 - 1) * dt * m_fAspectX;
        m_vertinfo[y1 * (m_nGridX + 1) + midx].c = 0.5f * (t10 + t11) + (FRAND * 2 - 1) * dt * m_fAspectX;
    }

    if (y1 - y0 >= 2 && x1 - x0 >= 2)
    {
        // Do midpoint and recurse.
        t00 = m_vertinfo[midy * (m_nGridX + 1) + x0].c;
        t01 = m_vertinfo[midy * (m_nGridX + 1) + x1].c;
        t10 = m_vertinfo[y0 * (m_nGridX + 1) + midx].c;
        t11 = m_vertinfo[y1 * (m_nGridX + 1) + midx].c;
        m_vertinfo[midy * (m_nGridX + 1) + midx].c = 0.25f * (t10 + t11 + t00 + t01) + (FRAND * 2 - 1) * dt;

        GenPlasma(x0, midx, y0, midy, dt * 0.5f);
        GenPlasma(midx, x1, y0, midy, dt * 0.5f);
        GenPlasma(x0, midx, midy, y1, dt * 0.5f);
        GenPlasma(midx, x1, midy, y1, dt * 0.5f);
    }
}

void CPlugin::LoadPreset(const wchar_t* szPresetFilename, float fBlendTime)
{
    OutputDebugString(szPresetFilename);
    //OutputDebugString(L"\n");
    // Clear old error message.
    //if (m_nFramesSinceResize > 4)
    //    ClearErrors(ERR_PRESET);

    // Make sure preset still exists. (might not if they are using the "back"/fwd buttons
    //  in RANDOM preset order and a file was renamed or deleted!)
    if (GetFileAttributes(szPresetFilename) == INVALID_FILE_ATTRIBUTES)
    {
        /*
        const wchar_t* p = wcsrchr(szPresetFilename, L'\\');
        p = (p) ? p + 1 : szPresetFilename;
        wchar_t buf[1024] = {0};
        swprintf_s(buf, WASABI_API_LNGSTRINGW(IDS_ERROR_PRESET_NOT_FOUND_X), p);
        AddError(buf, 6.0f, ERR_PRESET, true);
        */
        return;
    }

    if (!m_bSequentialPresetOrder)
    {
        // save preset in the history.  keep in mind - maybe we are searching back through it already!
        if (m_presetHistoryFwdFence == m_presetHistoryPos)
        {
            // We're at the forward frontier; add to history.
            m_presetHistory[m_presetHistoryPos] = szPresetFilename;
            m_presetHistoryFwdFence = (m_presetHistoryFwdFence + 1) % PRESET_HIST_LEN;

            // Don't let the two fences touch.
            if (m_presetHistoryBackFence == m_presetHistoryFwdFence)
                m_presetHistoryBackFence = (m_presetHistoryBackFence + 1) % PRESET_HIST_LEN;
        }
        else
        {
            // we're retracing our steps, either forward or backward...
        }
    }

    // if no preset was valid before, make sure there is no blend, because there is nothing valid to blend from.
    if (!wcscmp(m_pState->m_szDesc, INVALID_PRESET_DESC))
        fBlendTime = 0;

    if (fBlendTime == 0)
    {
        // Do it all NOW!
        if (szPresetFilename != m_szCurrentPresetFile) // [sic]
            wcscpy_s(m_szCurrentPresetFile, szPresetFilename);

        CState* temp = m_pState;
        m_pState = m_pOldState;
        m_pOldState = temp;

        DWORD ApplyFlags = STATE_ALL;
        ApplyFlags ^= (m_bWarpShaderLock ? STATE_WARP : 0);
        ApplyFlags ^= (m_bCompShaderLock ? STATE_COMP : 0);

        m_pState->Import(m_szCurrentPresetFile, GetTime(), m_pOldState, ApplyFlags);

        if (fBlendTime >= 0.001f)
        {
            RandomizeBlendPattern();
            m_pState->StartBlendFrom(m_pOldState, GetTime(), fBlendTime);
        }

        m_fPresetStartTime = GetTime();
        m_fNextPresetTime = -1.0f; // flags UpdateTime() to recompute this

        // Release stuff from `m_OldShaders`, then move m_shaders to `m_OldShaders`, then load the new shaders.
        SafeRelease(m_OldShaders.comp.ptr);
        SafeRelease(m_OldShaders.warp.ptr);
        SafeRelease(m_OldShaders.comp.CT);
        SafeRelease(m_OldShaders.warp.CT);
        m_OldShaders = m_shaders;
        ZeroMemory(&m_shaders, sizeof(PShaderSet));

        LoadShaders(&m_shaders, m_pState, false);

        OnFinishedLoadingPreset();
    }
    else
    {
        // Set up to load the preset (and especially compile shaders) a little bit at a time.
        SafeRelease(m_NewShaders.comp.ptr);
        SafeRelease(m_NewShaders.warp.ptr);
        ZeroMemory(&m_NewShaders, sizeof(PShaderSet));

        DWORD ApplyFlags = STATE_ALL;
        ApplyFlags ^= (m_bWarpShaderLock ? STATE_WARP : 0);
        ApplyFlags ^= (m_bCompShaderLock ? STATE_COMP : 0);

        m_pNewState->Import(szPresetFilename, GetTime(), m_pOldState, ApplyFlags);

        m_nLoadingPreset = 1; // this will cause `LoadPresetTick()` to get called over the next few frames...

        m_fLoadingPresetBlendTime = fBlendTime;
        wcscpy_s(m_szLoadingPreset, szPresetFilename);
    }
}

// Note: Only used this if the preset loaded *intact* (or mostly intact).
void CPlugin::OnFinishedLoadingPreset()
{
    SetMenusForPresetVersion(m_pState->m_nWarpPSVersion, m_pState->m_nCompPSVersion);
    m_nPresetsLoadedTotal++; // only increment this on COMPLETION of the load

    for (int mash = 0; mash < MASH_SLOTS; mash++)
        m_nMashPreset[mash] = m_nCurrentPreset;
}

void CPlugin::LoadPresetTick()
{
    if (m_nLoadingPreset == 2 || m_nLoadingPreset == 5)
    {
        // Just loads one shader (warp or comp) then returns.
        LoadShaders(&m_NewShaders, m_pNewState, true);
    }
    else if (m_nLoadingPreset == 8)
    {
        // Finished loading the shaders - apply the preset!
        wcscpy_s(m_szCurrentPresetFile, m_szLoadingPreset);
        m_szLoadingPreset[0] = 0;

        CState* temp = m_pState;
        m_pState = m_pOldState;
        m_pOldState = temp;

        temp = m_pState;
        m_pState = m_pNewState;
        m_pNewState = temp;

        RandomizeBlendPattern();

        //if (fBlendTime >= 0.001f)
        m_pState->StartBlendFrom(m_pOldState, GetTime(), m_fLoadingPresetBlendTime);

        m_fPresetStartTime = GetTime();
        m_fNextPresetTime = -1.0f; // flags `UpdateTime()` to recompute this

        // Release stuff from `m_OldShaders`, then move `m_shaders` to `m_OldShaders`, then load the new shaders.
        SafeRelease(m_OldShaders.comp.ptr);
        SafeRelease(m_OldShaders.warp.ptr);
        m_OldShaders = m_shaders;
        m_shaders = m_NewShaders;
        ZeroMemory(&m_NewShaders, sizeof(PShaderSet));

        // End slow-preset-load mode.
        m_nLoadingPreset = 0;

        OnFinishedLoadingPreset();
    }

    if (m_nLoadingPreset > 0)
        m_nLoadingPreset++;
}

void CPlugin::SetPresetListPosition(std::wstring search)
{
    size_t basename = search.find_last_of(L"\\");
    if (basename != std::wstring::npos)
        search = search.substr(basename + 1, search.length() - basename - 1);
    auto it = std::find_if(m_presets.begin(), m_presets.end(), [&s = search](const PresetInfo& m) -> bool { return m.szFilename == s; });
    if (it != m_presets.end())
        m_nCurrentPreset = static_cast<int>(it - m_presets.begin());
}

void CPlugin::SeekToPreset(wchar_t cStartChar)
{
    if (cStartChar >= L'a' && cStartChar <= L'z')
        cStartChar -= L'a' - L'A';

    for (int i = m_nDirs; i < m_nPresets; i++)
    {
        wchar_t ch = m_presets[i].szFilename.c_str()[0];
        if (ch >= L'a' && ch <= L'z')
            ch -= L'a' - L'A';
        if (ch == cStartChar)
        {
            m_nPresetListCurPos = i;
            return;
        }
    }
}

void CPlugin::FindValidPresetDir()
{
    swprintf_s(m_szPresetDir, L"%spresets\\", m_szMilkdrop2Path);
    if (GetFileAttributes(m_szPresetDir) != INVALID_FILE_ATTRIBUTES)
        return;
    wcscpy_s(m_szPresetDir, m_szMilkdrop2Path);
    if (GetFileAttributes(m_szPresetDir) != INVALID_FILE_ATTRIBUTES)
        return;
    wcscpy_s(m_szPresetDir, GetPluginsDirPath());
    if (GetFileAttributes(m_szPresetDir) != INVALID_FILE_ATTRIBUTES)
        return;
    wcscpy_s(m_szPresetDir, L"c:\\program files\\winamp\\"); // getting desperate here
    if (GetFileAttributes(m_szPresetDir) != INVALID_FILE_ATTRIBUTES)
        return;
    wcscpy_s(m_szPresetDir, L"c:\\program files\\"); // more desperate here
    if (GetFileAttributes(m_szPresetDir) != INVALID_FILE_ATTRIBUTES)
        return;
    wcscpy_s(m_szPresetDir, L"c:\\");
}

char* NextLine(char* p)
{
    // `p` points to the beginning of a line.
    // Return a pointer to the first character of the next line
    // if we hit a NULL char before that, we'll return NULL.
    if (!p)
        return NULL;

    char* s = p;
    while (*s != '\r' && *s != '\n' && *s != 0)
        s++;

    while (*s == '\r' || *s == '\n')
        s++;

    if (*s == 0)
        return NULL;

    return s;
}

// NOTE - this is run in a separate thread!!!
static unsigned int WINAPI __UpdatePresetList(void* lpVoid)
{
    ULONG_PTR flags = reinterpret_cast<ULONG_PTR>(lpVoid);
    bool bForce = (flags & 1) ? true : false;
    bool bTryReselectCurrentPreset = (flags & 2) ? true : false;

    WIN32_FIND_DATAW fd;
    ZeroMemory(&fd, sizeof(fd));
    HANDLE h = INVALID_HANDLE_VALUE;

    //int nTry = 0;
    bool bRetrying = false;

    EnterCriticalSection(&g_cs);

retry:
    // Make sure the path exists; if not, go to Winamp plugins directory.
    if (GetFileAttributes(g_plugin.m_szPresetDir) == INVALID_FILE_ATTRIBUTES)
    {
        g_plugin.FindValidPresetDir();
    }

    // If Mask (dir) changed, do a full re-scan.
    // If not, just finish the old scan.
    wchar_t szMask[MAX_PATH] = {0};
    swprintf_s(szMask, L"%s*.*", g_plugin.m_szPresetDir); // because directory names could have extensions, etc.
    if (bForce || !g_plugin.m_szUpdatePresetMask[0] || wcscmp(szMask, g_plugin.m_szUpdatePresetMask))
    {
        // If old directory was "" or the directory changed, reset the search.
        if (h != INVALID_HANDLE_VALUE)
            FindClose(h);
        h = INVALID_HANDLE_VALUE;
        g_plugin.m_bPresetListReady = false;
        wcscpy_s(g_plugin.m_szUpdatePresetMask, szMask);
        ZeroMemory(&fd, sizeof(fd));

        g_plugin.m_nPresets = 0;
        g_plugin.m_nDirs = 0;
        g_plugin.m_presets.clear();

        // Find first .MILK file
        if ((h = FindFirstFile(g_plugin.m_szUpdatePresetMask, &fd)) == INVALID_HANDLE_VALUE) // note: returns filename -without- path
        {
            // Revert back to plugins directory.
            /*
            wchar_t buf[1024];
            swprintf_s(buf, WASABI_API_LNGSTRINGW(IDS_ERROR_NO_PRESET_FILES_OR_DIRS_FOUND_IN_X), g_plugin.m_szPresetDir);
            g_plugin.AddError(buf, 4.0f, ERR_MISC, true);
            */

            if (bRetrying)
            {
                LeaveCriticalSection(&g_cs);
                g_bThreadAlive = false;
                _endthreadex(0);
                return 0;
            }

            g_plugin.FindValidPresetDir();

            bRetrying = true;
            goto retry;
        }

        g_plugin.AddError(GetStringW(WASABI_API_LNG_HINST, g_plugin.GetInstance(), IDS_SCANNING_PRESETS), 4.0f, ERR_SCANNING_PRESETS, false);
    }

    if (g_plugin.m_bPresetListReady)
    {
        LeaveCriticalSection(&g_cs);
        g_bThreadAlive = false;
        _endthreadex(0);
        return 0;
    }

    int nMaxPSVersion = g_plugin.m_nMaxPSVersion;
    wchar_t szPresetDir[MAX_PATH];
    wcscpy_s(szPresetDir, g_plugin.m_szPresetDir);

    LeaveCriticalSection(&g_cs);

    PresetList temp_presets;
    int temp_nDirs = 0;
    int temp_nPresets = 0;

    // scan for the desired # of presets, this call...
    while (!g_bThreadShouldQuit && h != INVALID_HANDLE_VALUE)
    {
        bool bSkip = false;
        bool bIsDir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        float fRating = 0;

        wchar_t szFilename[512] = {0};
        wcscpy_s(szFilename, fd.cFileName);

        if (bIsDir)
        {
            // Skip "." directory.
            if (wcscmp(fd.cFileName, L".") == 0) //|| wcslen(ffd.cFileName) < 1)
                bSkip = true;
            else
                swprintf_s(szFilename, L"*%s", fd.cFileName);
        }
        else
        {
            // Skip normal files not ending in ".milk".
            size_t len = wcslen(fd.cFileName);
            if (len < 5 || wcscmp(fd.cFileName + len - 5, L".milk") != 0)
                bSkip = true;

            // If it is ".milk," make sure to know how to run its pixel shaders -
            // otherwise do not show it in the preset list!
            if (!bSkip)
            {
                // If the first line of the file is not "MILKDROP_PRESET_VERSION XXX",
                // then it's a MilkDrop 1-era preset, so it is definitely runnable (no shaders).
                // Otherwise, check for the value "PSVERSION". It will be 0, 2, or 3.
                // If missing, assume it is 2.
                wchar_t szFullPath[MAX_PATH];
                swprintf_s(szFullPath, L"%s%s", szPresetDir, fd.cFileName);
                FILE* f;
                errno_t err = _wfopen_s(&f, szFullPath, L"r");
                if (err)
                    bSkip = true;
                else
                {
                    constexpr size_t PRESET_HEADER_SCAN_BYTES = 160U;
                    char szLine[PRESET_HEADER_SCAN_BYTES] = {0};
                    char* p = szLine;

                    int bytes_to_read = sizeof(szLine) - 1;
                    size_t count = fread(szLine, bytes_to_read, 1, f);
                    if (count < 1)
                    {
                        fseek(f, SEEK_SET, 0);
                        count = fread(szLine, 1, bytes_to_read, f);
                        szLine[count] = 0;
                    }
                    else
                        szLine[bytes_to_read - 1] = 0;

                    bool bScanForPreset00AndRating = false;
                    bool bRatingKnown = false;

                    // Try to read the PSVERSION and the fRating= value.
                    // Most presets (unless hand-edited) will have these right at the top.
                    // If not, [at least for fRating] use GetPrivateProfileFloat to search whole file.
                    // Read line 1.
                    //p = NextLine(p);//fgets(p, sizeof(p)-1, f);
                    if (!strncmp(p, "MILKDROP_PRESET_VERSION", 23))
                    {
                        p = NextLine(p); //fgets(p, sizeof(p)-1, f);
                        int ps_version = 2;
                        if (p && !strncmp(p, "PSVERSION", 9))
                        {
                            sscanf_s(&p[10], "%d", &ps_version);
                            if (ps_version > nMaxPSVersion)
                                bSkip = true;
                            else
                            {
                                p = NextLine(p); //fgets(p, sizeof(p)-1, f);
                                bScanForPreset00AndRating = true;
                            }
                        }
                    }
                    else
                    {
                        // otherwise it's a MilkDrop 1 preset - we can run it.
                        bScanForPreset00AndRating = true;
                    }

                    // scan up to 10 more lines in the file, looking for [preset00] and fRating=...
                    // (this is WAY faster than GetPrivateProfileFloat, when it works!)
                    int reps = (bScanForPreset00AndRating) ? 10 : 0;
                    for (int z = 0; z < reps; z++)
                    {
                        if (p && !strncmp(p, "[preset00]", 10))
                        {
                            p = NextLine(p);
                            if (p && !strncmp(p, "fRating=", 8))
                            {
                                _sscanf_s_l(&p[8], "%f", g_use_C_locale, &fRating);
                                bRatingKnown = true;
                                break;
                            }
                        }
                        p = NextLine(p);
                    }

                    fclose(f);

                    if (!bRatingKnown)
                        fRating = GetPrivateProfileFloat(L"preset00", L"fRating", 3.0f, szFullPath);
                    fRating = std::max(0.0f, std::min(5.0f, fRating));
                }
            }
        }

        if (!bSkip)
        {
            float fPrevPresetRatingCum = 0;
            if (temp_nPresets > 0)
                fPrevPresetRatingCum += temp_presets[static_cast<size_t>(temp_nPresets) - 1].fRatingCum;

            PresetInfo x;
            x.szFilename = szFilename;
            x.fRatingThis = fRating;
            x.fRatingCum = fPrevPresetRatingCum + fRating;
            temp_presets.push_back(x);

            temp_nPresets++;
            if (bIsDir)
                temp_nDirs++;
        }

        if (!FindNextFile(h, &fd))
        {
            FindClose(h);
            h = INVALID_HANDLE_VALUE;

            break;
        }

#define PRESET_UPDATE_INTERVAL 64
        // Every so often, add some presets...
        if (temp_nPresets == 30 || ((temp_nPresets % PRESET_UPDATE_INTERVAL) == 0))
        {
            EnterCriticalSection(&g_cs);

            //g_plugin.m_presets = temp_presets;
            for (int i = g_plugin.m_nPresets; i < temp_nPresets; i++)
                g_plugin.m_presets.push_back(temp_presets[i]);
            g_plugin.m_nPresets = temp_nPresets;
            g_plugin.m_nDirs = temp_nDirs;

            LeaveCriticalSection(&g_cs);
        }
    }

    if (g_bThreadShouldQuit)
    {
        // just abort... we are exiting the program or restarting the scan.
        g_bThreadAlive = false;
        _endthreadex(0);
        return 0;
    }

    EnterCriticalSection(&g_cs);

    //g_plugin.m_presets = temp_presets;
    for (int i = g_plugin.m_nPresets; i < temp_nPresets; i++)
        g_plugin.m_presets.push_back(temp_presets[i]);
    g_plugin.m_nPresets = temp_nPresets;
    g_plugin.m_nDirs = temp_nDirs;
    //g_plugin.m_bPresetListReady = true;

    if (g_plugin.m_nPresets == 0) //if (g_plugin.m_bPresetListReady && g_plugin.m_nPresets == 0)
    {
        // no presets OR directories found - weird - but it happens.
        // --> revert back to plugins dir
        /*
        wchar_t buf[1024];
        swprintf_s(buf, WASABI_API_LNGSTRINGW(IDS_ERROR_NO_PRESET_FILES_OR_DIRS_FOUND_IN_X), g_plugin.m_szPresetDir);
        g_plugin.AddError(buf, 4.0f, ERR_MISC, true);
        */

        if (bRetrying)
        {
            LeaveCriticalSection(&g_cs);
            g_bThreadAlive = false;
            _endthreadex(0);
            return 0;
        }

        g_plugin.FindValidPresetDir();

        bRetrying = true;
        goto retry;
    }

    //if (g_plugin.m_bPresetListReady)
    {
        g_plugin.MergeSortPresets(0, g_plugin.m_nPresets - 1);

        // Update cumulative ratings, since order changed...
        g_plugin.m_presets[0].fRatingCum = g_plugin.m_presets[0].fRatingThis;
        for (int i = 1; i < g_plugin.m_nPresets; i++)
            g_plugin.m_presets[i].fRatingCum = g_plugin.m_presets[static_cast<size_t>(i) - 1].fRatingCum + g_plugin.m_presets[i].fRatingThis;

        // Clear the "Scanning presets..." message.
        //g_plugin.ClearErrors(ERR_SCANNING_PRESETS);

        // Finally, try to re-select the most recently-used preset in the list.
        g_plugin.m_nPresetListCurPos = 0;
        if (bTryReselectCurrentPreset)
        {
            if (g_plugin.m_szCurrentPresetFile[0])
            {
                // Try to automatically seek to the last preset loaded.
                wchar_t* p = wcsrchr(g_plugin.m_szCurrentPresetFile, L'\\');
                p = (p) ? (p + 1) : g_plugin.m_szCurrentPresetFile;
                for (int i = g_plugin.m_nDirs; i < g_plugin.m_nPresets; i++)
                {
                    if (wcscmp(p, g_plugin.m_presets[i].szFilename.c_str()) == 0)
                    {
                        g_plugin.m_nPresetListCurPos = i;
                        break;
                    }
                }
            }
        }
    }

    LeaveCriticalSection(&g_cs);
    g_plugin.m_bPresetListReady = true;

    g_bThreadAlive = false;
    //_endthreadex(0); // calling this here stops destructors from being called for local objects!
    return 0;
}

// Note: If directory changed, make sure `bForce` is true!
void CPlugin::UpdatePresetList(bool bBackground, bool bForce, bool bTryReselectCurrentPreset)
{
    if (bForce)
    {
        if (g_bThreadAlive)
            CancelThread(3000); // flags it to exit; the param is the # of ms to wait before forcefully killing it
    }
    else
    {
        if (bBackground && (g_bThreadAlive || m_bPresetListReady))
            return;
        if (!bBackground && m_bPresetListReady)
            return;
    }

    assert(!g_bThreadAlive);

    // Spawn new thread.
    ULONG_PTR flags = (bForce ? 1 : 0) | (bTryReselectCurrentPreset ? 2 : 0);
    g_bThreadShouldQuit = false;
    g_bThreadAlive = true;
    g_hThread = (HANDLE)_beginthreadex(NULL, 0, __UpdatePresetList, reinterpret_cast<void*>(flags), 0, 0);

    if (!bBackground && g_hThread)
    {
        // Crank up priority, wait for it to finish, and then return.
        SetThreadPriority(g_hThread, THREAD_PRIORITY_HIGHEST);

        // Wait for it to finish.
        while (g_bThreadAlive)
            Sleep(30);

        assert(g_hThread != INVALID_HANDLE_VALUE);
        CloseHandle(g_hThread);
        g_hThread = INVALID_HANDLE_VALUE;
    }
    else if (g_hThread)
    {
        // It will just run in the background til it finishes.
        // however, we want to wait until at least ~32 presets are found (or failure) before returning,
        // so we know we have *something* in the preset list to start with.
        SetThreadPriority(g_hThread, THREAD_PRIORITY_HIGHEST);

        // Wait until either the thread exits, or number of presets is >32, before returning.
        // Also enter the CS whenever checking on it!
        // (thread will update preset list every so often, with the newest presets scanned in...)
        while (g_bThreadAlive)
        {
            Sleep(30);

            EnterCriticalSection(&g_cs);
            int nPresets = g_plugin.m_nPresets;
            LeaveCriticalSection(&g_cs);

            if (nPresets >= 30)
                break;
        }

        if (g_bThreadAlive)
        {
            // The load still takes a while even at THREAD_PRIORITY_ABOVE_NORMAL,
            // because it is waiting on the HDD so much...
            // But the OS is smart, and the CPU stays nice and zippy in other threads =)
            SetThreadPriority(g_hThread, THREAD_PRIORITY_ABOVE_NORMAL);
        }
    }

    return;
}

void CPlugin::MergeSortPresets(int left, int right)
{
    // note: left..right range is inclusive
    int nItems = right - left + 1;

    if (nItems > 2)
    {
        // Recurse to sort 2 halves (but don't actually recurse on a half if it only has 1 element).
        int mid = (left + right) / 2;
        /*if (mid   != left) */ MergeSortPresets(left, mid);
        /*if (mid+1 != right)*/ MergeSortPresets(mid + 1, right);

        // Then merge results.
        int a = left;
        int b = mid + 1;
        while (a <= mid && b <= right)
        {
            bool bSwap;

            // Merge the sorted arrays; give preference to strings that start with a '*' character.
            int nSpecial = 0;
            if (m_presets[a].szFilename.c_str()[0] == '*') nSpecial++;
            if (m_presets[b].szFilename.c_str()[0] == '*') nSpecial++;

            if (nSpecial == 1)
            {
                bSwap = (m_presets[b].szFilename.c_str()[0] == '*');
            }
            else
            {
                bSwap = (_wcsicmp(m_presets[a].szFilename.c_str(), m_presets[b].szFilename.c_str()) > 0);
            }

            if (bSwap)
            {
                PresetInfo temp = m_presets[b];
                for (int k = b; k > a; k--)
                    m_presets[k] = m_presets[k - 1];
                m_presets[a] = temp;
                mid++;
                b++;
            }
            a++;
        }
    }
    else if (nItems == 2)
    {
        // Sort 2 items; give preference to 'special' strings that start with a '*' character.
        int nSpecial = 0;
        if (m_presets[left].szFilename.c_str()[0] == '*') nSpecial++;
        if (m_presets[right].szFilename.c_str()[0] == '*') nSpecial++;

        if (nSpecial == 1)
        {
            if (m_presets[right].szFilename.c_str()[0] == '*')
            {
                PresetInfo temp = m_presets[left];
                m_presets[left] = m_presets[right];
                m_presets[right] = temp;
            }
        }
        else if (_wcsicmp(m_presets[left].szFilename.c_str(), m_presets[right].szFilename.c_str()) > 0)
        {
            PresetInfo temp = m_presets[left];
            m_presets[left] = m_presets[right];
            m_presets[right] = temp;
        }
    }
}

void CPlugin::WaitString_NukeSelection()
{
    if (m_waitstring.bActive && m_waitstring.nSelAnchorPos != -1)
    {
        // Nuke selection. Note: Start and end are INCLUSIVE.
        size_t start = (m_waitstring.nCursorPos < static_cast<unsigned int>(m_waitstring.nSelAnchorPos)) ? m_waitstring.nCursorPos : m_waitstring.nSelAnchorPos;
        size_t end = (m_waitstring.nCursorPos > static_cast<unsigned int>(m_waitstring.nSelAnchorPos)) ? m_waitstring.nCursorPos - 1 : m_waitstring.nSelAnchorPos - 1;
        size_t len = (m_waitstring.bDisplayAsCode ? strlen(reinterpret_cast<char*>(m_waitstring.szText)) : wcslen(m_waitstring.szText));
        size_t how_far_to_shift = end - start + 1;
        size_t num_chars_to_shift = len - end; // includes NULL character

        if (m_waitstring.bDisplayAsCode)
        {
            char* ptr = reinterpret_cast<char*>(m_waitstring.szText);
            for (unsigned int i = 0; i < num_chars_to_shift; i++)
                *(ptr + start + i) = *(ptr + start + i + how_far_to_shift);
        }
        else
        {
            for (unsigned int i = 0; i < num_chars_to_shift; i++)
                m_waitstring.szText[start + i] = m_waitstring.szText[start + i + how_far_to_shift];
        }

        // Clear selection.
        m_waitstring.nCursorPos = start;
        m_waitstring.nSelAnchorPos = -1;
    }
}

void CPlugin::WaitString_Cut()
{
    if (m_waitstring.bActive && m_waitstring.nSelAnchorPos != -1)
    {
        WaitString_Copy();
        WaitString_NukeSelection();
    }
}

void CPlugin::WaitString_Copy()
{
    if (m_waitstring.bActive && m_waitstring.nSelAnchorPos != -1)
    {
        // Note: Start and end are INCLUSIVE.
        size_t start = (m_waitstring.nCursorPos < static_cast<unsigned int>(m_waitstring.nSelAnchorPos)) ? m_waitstring.nCursorPos : m_waitstring.nSelAnchorPos;
        size_t end = (m_waitstring.nCursorPos > static_cast<unsigned int>(m_waitstring.nSelAnchorPos)) ? m_waitstring.nCursorPos - 1 : m_waitstring.nSelAnchorPos - 1;
        size_t chars_to_copy = end - start + 1;

        if (m_waitstring.bDisplayAsCode)
        {
            char* ptr = reinterpret_cast<char*>(m_waitstring.szText);
            for (unsigned int i = 0; i < chars_to_copy; i++)
                m_waitstring.szClipboardA[i] = *(ptr + start + i);
            m_waitstring.szClipboardA[chars_to_copy] = 0;

            std::vector<char> tmp;
            tmp.resize(65536);
            ConvertLFCToCRsA(m_waitstring.szClipboardA, tmp.data());
            copyStringToClipboardA(tmp.data());
        }
        else
        {
            for (unsigned int i = 0; i < chars_to_copy; i++)
                m_waitstring.szClipboard[i] = m_waitstring.szText[start + i];
            m_waitstring.szClipboard[chars_to_copy] = 0;

            std::vector<wchar_t> tmp;
            tmp.resize(65536);
            ConvertLFCToCRsW(m_waitstring.szClipboard, tmp.data());
            copyStringToClipboardW(tmp.data());
        }
    }
}

void CPlugin::WaitString_Paste()
{
    // NOTE: if there is a selection, it is wiped out, and replaced with the clipboard contents.

    if (m_waitstring.bActive)
    {
        WaitString_NukeSelection();

        if (m_waitstring.bDisplayAsCode)
        {
            std::vector<char> tmp;
            tmp.resize(65536);
            strcpy_s(tmp.data(), 65536, getStringFromClipboardA());
            ConvertCRsToLFCA(tmp.data(), m_waitstring.szClipboardA);
        }
        else
        {
            std::vector<wchar_t> tmp;
            tmp.resize(65536);
            wcscpy_s(tmp.data(), 65536, getStringFromClipboardW());
            ConvertCRsToLFCW(tmp.data(), m_waitstring.szClipboard);
        }

        size_t len;
        size_t chars_to_insert;
        if (m_waitstring.bDisplayAsCode)
        {
            len = strlen(reinterpret_cast<char*>(m_waitstring.szText));
            chars_to_insert = strlen(m_waitstring.szClipboardA);
        }
        else
        {
            len = wcslen(m_waitstring.szText);
            chars_to_insert = wcslen(m_waitstring.szClipboard);
        }

        if (static_cast<unsigned int>(len + chars_to_insert + 1) >= m_waitstring.nMaxLen)
        {
            chars_to_insert = m_waitstring.nMaxLen - len - 1;

            // Inform user.
            AddError(WASABI_API_LNGSTRINGW(IDS_STRING_TOO_LONG), 2.5f, ERR_MISC, true);
        }
        else
        {
            //m_fShowUserMessageUntilThisTime = GetTime(); // if there was an error message already, clear it
        }

        size_t i;
        if (m_waitstring.bDisplayAsCode)
        {
            char* ptr = reinterpret_cast<char*>(m_waitstring.szText);
            for (i = len; i >= m_waitstring.nCursorPos; i--)
                *(ptr + i + chars_to_insert) = *(ptr + i);
            for (i = 0; i < chars_to_insert; i++)
                *(ptr + i + m_waitstring.nCursorPos) = m_waitstring.szClipboardA[i];
        }
        else
        {
            for (i = len; i >= m_waitstring.nCursorPos; i--)
                m_waitstring.szText[i + chars_to_insert] = m_waitstring.szText[i];
            for (i = 0; i < chars_to_insert; i++)
                m_waitstring.szText[i + m_waitstring.nCursorPos] = m_waitstring.szClipboard[i];
        }
        m_waitstring.nCursorPos += chars_to_insert;
    }
}

// Moves to beginning of prior word.
void CPlugin::WaitString_SeekLeftWord()
{
    if (m_waitstring.bDisplayAsCode)
    {
        char* ptr = reinterpret_cast<char*>(m_waitstring.szText);
        while (m_waitstring.nCursorPos > 0 && !IsAlphanumericChar(*(ptr + m_waitstring.nCursorPos - 1)))
            m_waitstring.nCursorPos--;

        while (m_waitstring.nCursorPos > 0 && IsAlphanumericChar(*(ptr + m_waitstring.nCursorPos - 1)))
            m_waitstring.nCursorPos--;
    }
    else
    {
        while (m_waitstring.nCursorPos > 0 && !IsAlphanumericChar(m_waitstring.szText[m_waitstring.nCursorPos - 1]))
            m_waitstring.nCursorPos--;

        while (m_waitstring.nCursorPos > 0 && IsAlphanumericChar(m_waitstring.szText[m_waitstring.nCursorPos - 1]))
            m_waitstring.nCursorPos--;
    }
}

// Moves to beginning of next word
void CPlugin::WaitString_SeekRightWord()
{
    // Testing lots ofstuff.
    if (m_waitstring.bDisplayAsCode)
    {
        size_t len = strlen(reinterpret_cast<char*>(m_waitstring.szText));

        char* ptr = reinterpret_cast<char*>(m_waitstring.szText);
        while (m_waitstring.nCursorPos < len && IsAlphanumericChar(*(ptr + m_waitstring.nCursorPos)))
            m_waitstring.nCursorPos++;

        while (m_waitstring.nCursorPos < len && !IsAlphanumericChar(*(ptr + m_waitstring.nCursorPos)))
            m_waitstring.nCursorPos++;
    }
    else
    {
        size_t len = wcslen(m_waitstring.szText);

        while (m_waitstring.nCursorPos < len && IsAlphanumericChar(m_waitstring.szText[m_waitstring.nCursorPos]))
            m_waitstring.nCursorPos++;

        while (m_waitstring.nCursorPos < len && !IsAlphanumericChar(m_waitstring.szText[m_waitstring.nCursorPos]))
            m_waitstring.nCursorPos++;
    }
}

size_t CPlugin::WaitString_GetCursorColumn()
{
    if (m_waitstring.bDisplayAsCode)
    {
        int column = 0;
        char* ptr = reinterpret_cast<char*>(m_waitstring.szText);
        while (/*m_waitstring.nCursorPos - column - 1 >= 0 &&*/ *(ptr + m_waitstring.nCursorPos - column - 1) != LINEFEED_CONTROL_CHAR)
            column++;

        return column;
    }
    else
    {
        return m_waitstring.nCursorPos;
    }
}

int CPlugin::WaitString_GetLineLength()
{
    size_t line_start = m_waitstring.nCursorPos - WaitString_GetCursorColumn();
    int line_length = 0;

    if (m_waitstring.bDisplayAsCode)
    {
        char* ptr = reinterpret_cast<char*>(m_waitstring.szText);
        while (*(ptr + line_start + line_length) != 0 && *(ptr + line_start + line_length) != LINEFEED_CONTROL_CHAR)
            line_length++;
    }
    else
    {
        while (m_waitstring.szText[line_start + line_length] != 0 && m_waitstring.szText[line_start + line_length] != LINEFEED_CONTROL_CHAR)
            line_length++;
    }

    return line_length;
}

void CPlugin::WaitString_SeekUpOneLine()
{
    size_t column = g_plugin.WaitString_GetCursorColumn();

    if (column != m_waitstring.nCursorPos)
    {
        // Seek to very end of previous line (cursor will be at the semicolon).
        m_waitstring.nCursorPos -= column + 1;

        size_t new_column = g_plugin.WaitString_GetCursorColumn();

        if (new_column > column)
            m_waitstring.nCursorPos -= (new_column - column);
    }
}

void CPlugin::WaitString_SeekDownOneLine()
{
    size_t column = g_plugin.WaitString_GetCursorColumn();
    size_t newpos = m_waitstring.nCursorPos;

    char* ptr = reinterpret_cast<char*>(m_waitstring.szText);
    while (*(ptr + newpos) != 0 && *(ptr + newpos) != LINEFEED_CONTROL_CHAR)
        newpos++;

    if (*(ptr + newpos) != 0)
    {
        m_waitstring.nCursorPos = newpos + 1;

        while (column > 0 && *(ptr + m_waitstring.nCursorPos) != LINEFEED_CONTROL_CHAR && *(ptr + m_waitstring.nCursorPos) != 0)
        {
            m_waitstring.nCursorPos++;
            column--;
        }
    }
}

// Overwrites the file if it was already there,
// so check if the file exists first and prompt
// user to overwrite, before calling this function.
void CPlugin::SavePresetAs(wchar_t* szNewFile)
{
    if (!m_pState->Export(szNewFile))
    {
        // Error.
        AddError(WASABI_API_LNGSTRINGW(IDS_ERROR_UNABLE_TO_SAVE_THE_FILE), 6.0f, ERR_PRESET, true);
    }
    else
    {
        // Pop up confirmation.
        AddError(WASABI_API_LNGSTRINGW(IDS_SAVE_SUCCESSFUL), 3.0f, ERR_NOTIFY, false);

        // Update `m_pState->m_szDesc` with the new name.
        wcscpy_s(m_pState->m_szDesc, m_waitstring.szText);

        // Refresh file listing.
        UpdatePresetList(false, true);
    }
}

// Note: Assumes that `m_nPresetListCurPos` indicates
//       the slot that the to-be-deleted preset occupies!
void CPlugin::DeletePresetFile(wchar_t* szDelFile)
{
    // Delete file.
    if (!DeleteFile(szDelFile))
    {
        // Error.
        AddError(WASABI_API_LNGSTRINGW(IDS_ERROR_UNABLE_TO_DELETE_THE_FILE), 6.0f, ERR_MISC, true);
    }
    else
    {
        // Pop up confirmation.
        wchar_t buf[1024];
        swprintf_s(buf, WASABI_API_LNGSTRINGW(IDS_PRESET_X_DELETED), m_presets[m_nPresetListCurPos].szFilename.c_str());
        AddError(buf, 3.0f, ERR_NOTIFY, false);

        // Refresh file listing & re-select the next file after the one deleted.
        int newPos = m_nPresetListCurPos;
        UpdatePresetList(false, true);
        m_nPresetListCurPos = std::max(0, std::min(m_nPresets - 1, newPos));
    }
}

// Note: This function additionally assumes that `m_nPresetListCurPos` indicates
//       the slot that the to-be-renamed preset occupies!
void CPlugin::RenamePresetFile(wchar_t* szOldFile, wchar_t* szNewFile)
{
    if (GetFileAttributes(szNewFile) != INVALID_FILE_ATTRIBUTES) // check if file already exists
    {
        // Error.
        AddError(WASABI_API_LNGSTRINGW(IDS_ERROR_A_FILE_ALREADY_EXISTS_WITH_THAT_FILENAME), 6.0f, ERR_PRESET, true);

        // (user remains in UI_LOAD_RENAME mode to try another filename)
    }
    else
    {
        // Rename.
        if (!MoveFile(szOldFile, szNewFile))
        {
            // Error.
            AddError(WASABI_API_LNGSTRINGW(IDS_ERROR_UNABLE_TO_RENAME_FILE), 6.0f, ERR_MISC, true);
        }
        else
        {
            // Pop up confirmation.
            AddError(WASABI_API_LNGSTRINGW(IDS_RENAME_SUCCESSFUL), 3.0f, ERR_NOTIFY, false);

            // If this preset was the active one, update `m_pState->m_szDesc` with the new name.
            wchar_t buf[512] = {0};
            swprintf_s(buf, L"%s.milk", m_pState->m_szDesc);
            if (wcscmp(m_presets[m_nPresetListCurPos].szFilename.c_str(), buf) == 0)
            {
                wcscpy_s(m_pState->m_szDesc, m_waitstring.szText);
            }

            // Refresh file listing and do a trick to make it re-select the renamed file.
            wchar_t buf2[512] = {0};
            wcscpy_s(buf2, m_waitstring.szText);
            wcscat_s(buf2, L".milk");
            m_presets[m_nPresetListCurPos].szFilename = buf2;
            UpdatePresetList(false, true, false);

            // Jump to (highlight) the new file.
            m_nPresetListCurPos = 0;
            wchar_t* p = wcsrchr(szNewFile, L'\\');
            if (p)
            {
                p++;
                for (int i = m_nDirs; i < m_nPresets; i++)
                {
                    if (wcscmp(p, m_presets[i].szFilename.c_str()) == 0)
                    {
                        m_nPresetListCurPos = i;
                        break;
                    }
                }
            }
        }

        // Exit waitstring mode (return to load menu).
        m_UI_mode = UI_LOAD;
        m_waitstring.bActive = false;
    }
}

void CPlugin::SetCurrentPresetRating(float fNewRating)
{
    if (!m_bEnableRating)
        return;

    if (fNewRating < 0)
        fNewRating = 0;
    if (fNewRating > 5)
        fNewRating = 5;
    float change = (fNewRating - m_pState->m_fRating);

    // Update the file on disk.
    //char szPresetFileNoPath[512];
    //char szPresetFileWithPath[512];
    //sprintf_s(szPresetFileNoPath, "%s.milk", m_pState->m_szDesc);
    //sprintf_s(szPresetFileWithPath, "%s%s.milk", GetPresetDir(), m_pState->m_szDesc);
    WritePrivateProfileFloat(fNewRating, L"fRating", m_szCurrentPresetFile, L"preset00");

    // Update the copy of the preset in memory.
    m_pState->m_fRating = fNewRating;

    // Update the cumulative internal listing.
    m_presets[m_nCurrentPreset].fRatingThis += change;
    if (m_nCurrentPreset != -1) //&& m_nRatingReadProgress >= m_nCurrentPreset) // (can be -1 if dir. changed but no new preset was loaded yet)
        for (int i = m_nCurrentPreset; i < m_nPresets; i++)
            m_presets[i].fRatingCum += change;

    /* keep in view:
        -test switching dirs w/o loading a preset, and trying to change the rating
            ->m_nCurrentPreset is out of range!
        -soln: when adjusting rating:
            1. file to modify is m_szCurrentPresetFile
            2. only update CDF if m_nCurrentPreset is not -1
        -> set m_nCurrentPreset to -1 whenever dir. changes
        -> set m_szCurrentPresetFile whenever you load a preset
    */

    // Show a message.
    if (!m_bShowRating)
    {
        // See also `DrawText()` in `milkdropfs.cpp`.
        m_fShowRatingUntilThisTime = GetTime() + 2.0f;
    }
}

void CPlugin::ReadCustomMessages()
{
    // First, clear all old data
    for (int n = 0; n < MAX_CUSTOM_MESSAGE_FONTS; n++)
    {
        wcscpy_s(m_customMessageFont[n].szFace, L"Arial");
        m_customMessageFont[n].bBold = false;
        m_customMessageFont[n].bItal = false;
        m_customMessageFont[n].nColorR = 255;
        m_customMessageFont[n].nColorG = 255;
        m_customMessageFont[n].nColorB = 255;
    }

    for (int n = 0; n < MAX_CUSTOM_MESSAGES; n++)
    {
        m_customMessage[n].szText[0] = 0;
        m_customMessage[n].nFont = 0;
        m_customMessage[n].fSize = 50.0f; // [0..100]  note that size is not absolute, but relative to the size of the window
        m_customMessage[n].x = 0.5f;
        m_customMessage[n].y = 0.5f;
        m_customMessage[n].randx = 0;
        m_customMessage[n].randy = 0;
        m_customMessage[n].growth = 1.0f;
        m_customMessage[n].fTime = 1.5f;
        m_customMessage[n].fFade = 0.2f;

        m_customMessage[n].bOverrideBold = false;
        m_customMessage[n].bOverrideItal = false;
        m_customMessage[n].bOverrideFace = false;
        m_customMessage[n].bOverrideColorR = false;
        m_customMessage[n].bOverrideColorG = false;
        m_customMessage[n].bOverrideColorB = false;
        m_customMessage[n].bBold = false;
        m_customMessage[n].bItal = false;
        wcscpy_s(m_customMessage[n].szFace, L"Arial");
        m_customMessage[n].nColorR = 255;
        m_customMessage[n].nColorG = 255;
        m_customMessage[n].nColorB = 255;
        m_customMessage[n].nRandR = 0;
        m_customMessage[n].nRandG = 0;
        m_customMessage[n].nRandB = 0;
    }

    // Then read in the new file.
    if (!FileExists(m_szMsgIniFile))
        return;

    for (int n = 0; n < MAX_CUSTOM_MESSAGE_FONTS; n++)
    {
        wchar_t szSectionName[32];
        swprintf_s(szSectionName, L"font%02d", n);

        // Get face, bold, italic, x, y for this custom message FONT.
        GetPrivateProfileString(szSectionName, L"face", L"Arial", m_customMessageFont[n].szFace, ARRAYSIZE(m_customMessageFont[n].szFace), m_szMsgIniFile);
        m_customMessageFont[n].bBold = GetPrivateProfileBool(szSectionName, L"bold", m_customMessageFont[n].bBold, m_szMsgIniFile);
        m_customMessageFont[n].bItal = GetPrivateProfileBool(szSectionName, L"ital", m_customMessageFont[n].bItal, m_szMsgIniFile);
        m_customMessageFont[n].nColorR = GetPrivateProfileInt(szSectionName, L"r", m_customMessageFont[n].nColorR, m_szMsgIniFile);
        m_customMessageFont[n].nColorG = GetPrivateProfileInt(szSectionName, L"g", m_customMessageFont[n].nColorG, m_szMsgIniFile);
        m_customMessageFont[n].nColorB = GetPrivateProfileInt(szSectionName, L"b", m_customMessageFont[n].nColorB, m_szMsgIniFile);
    }

    for (int n = 0; n < MAX_CUSTOM_MESSAGES; n++)
    {
        wchar_t szSectionName[64];
        swprintf_s(szSectionName, L"message%02d", n);

        // Get fontID, size, text, etc. for this custom message.
        GetPrivateProfileString(szSectionName, L"text", L"", m_customMessage[n].szText, ARRAYSIZE(m_customMessage[n].szText), m_szMsgIniFile);
        if (m_customMessage[n].szText[0])
        {
            m_customMessage[n].nFont = GetPrivateProfileInt(szSectionName, L"font", m_customMessage[n].nFont, m_szMsgIniFile);
            m_customMessage[n].fSize = GetPrivateProfileFloat(szSectionName, L"size", m_customMessage[n].fSize, m_szMsgIniFile);
            m_customMessage[n].x = GetPrivateProfileFloat(szSectionName, L"x", m_customMessage[n].x, m_szMsgIniFile);
            m_customMessage[n].y = GetPrivateProfileFloat(szSectionName, L"y", m_customMessage[n].y, m_szMsgIniFile);
            m_customMessage[n].randx = GetPrivateProfileFloat(szSectionName, L"randx", m_customMessage[n].randx, m_szMsgIniFile);
            m_customMessage[n].randy = GetPrivateProfileFloat(szSectionName, L"randy", m_customMessage[n].randy, m_szMsgIniFile);

            m_customMessage[n].growth = GetPrivateProfileFloat(szSectionName, L"growth", m_customMessage[n].growth, m_szMsgIniFile);
            m_customMessage[n].fTime = GetPrivateProfileFloat(szSectionName, L"time", m_customMessage[n].fTime, m_szMsgIniFile);
            m_customMessage[n].fFade = GetPrivateProfileFloat(szSectionName, L"fade", m_customMessage[n].fFade, m_szMsgIniFile);
            m_customMessage[n].nColorR = GetPrivateProfileInt(szSectionName, L"r", m_customMessage[n].nColorR, m_szMsgIniFile);
            m_customMessage[n].nColorG = GetPrivateProfileInt(szSectionName, L"g", m_customMessage[n].nColorG, m_szMsgIniFile);
            m_customMessage[n].nColorB = GetPrivateProfileInt(szSectionName, L"b", m_customMessage[n].nColorB, m_szMsgIniFile);
            m_customMessage[n].nRandR = GetPrivateProfileInt(szSectionName, L"randr", m_customMessage[n].nRandR, m_szMsgIniFile);
            m_customMessage[n].nRandG = GetPrivateProfileInt(szSectionName, L"randg", m_customMessage[n].nRandG, m_szMsgIniFile);
            m_customMessage[n].nRandB = GetPrivateProfileInt(szSectionName, L"randb", m_customMessage[n].nRandB, m_szMsgIniFile);

            // Overrides: r,g,b,face,bold,ital
            GetPrivateProfileString(szSectionName, L"face", L"", m_customMessage[n].szFace, ARRAYSIZE(m_customMessage[n].szFace), m_szMsgIniFile);
            m_customMessage[n].bBold = GetPrivateProfileInt(szSectionName, L"bold", -1, m_szMsgIniFile);
            m_customMessage[n].bItal = GetPrivateProfileInt(szSectionName, L"ital", -1, m_szMsgIniFile);
            m_customMessage[n].nColorR = GetPrivateProfileInt(szSectionName, L"r", -1, m_szMsgIniFile);
            m_customMessage[n].nColorG = GetPrivateProfileInt(szSectionName, L"g", -1, m_szMsgIniFile);
            m_customMessage[n].nColorB = GetPrivateProfileInt(szSectionName, L"b", -1, m_szMsgIniFile);

            m_customMessage[n].bOverrideFace = (m_customMessage[n].szFace[0] != 0);
            m_customMessage[n].bOverrideBold = (m_customMessage[n].bBold != -1);
            m_customMessage[n].bOverrideItal = (m_customMessage[n].bItal != -1);
            m_customMessage[n].bOverrideColorR = (m_customMessage[n].nColorR != -1);
            m_customMessage[n].bOverrideColorG = (m_customMessage[n].nColorG != -1);
            m_customMessage[n].bOverrideColorB = (m_customMessage[n].nColorB != -1);
        }
    }
}

void CPlugin::LaunchCustomMessage(int nMsgNum)
{
    if (nMsgNum > 99)
        nMsgNum = 99;

    if (nMsgNum < 0)
    {
        int count = 0;
        // Choose randomly.
        for (nMsgNum = 0; nMsgNum < 100; nMsgNum++)
            if (m_customMessage[nMsgNum].szText[0])
                count++;

        int sel = (warand() % count) + 1;
        count = 0;
        for (nMsgNum = 0; nMsgNum < 100; nMsgNum++)
        {
            if (m_customMessage[nMsgNum].szText[0])
                count++;
            if (count == sel)
                break;
        }
    }

    if (nMsgNum < 0 || nMsgNum >= MAX_CUSTOM_MESSAGES || m_customMessage[nMsgNum].szText[0] == 0)
    {
        return;
    }

    int fontID = m_customMessage[nMsgNum].nFont;

    m_supertext.bRedrawSuperText = true;
    m_supertext.bIsSongTitle = false;
    wcscpy_s(m_supertext.szText, m_customMessage[nMsgNum].szText);

    // Regular properties.
    m_supertext.fFontSize = m_customMessage[nMsgNum].fSize;
    m_supertext.fX = m_customMessage[nMsgNum].x + m_customMessage[nMsgNum].randx * ((warand() % 1037) / 1037.0f * 2.0f - 1.0f);
    m_supertext.fY = m_customMessage[nMsgNum].y + m_customMessage[nMsgNum].randy * ((warand() % 1037) / 1037.0f * 2.0f - 1.0f);
    m_supertext.fGrowth = m_customMessage[nMsgNum].growth;
    m_supertext.fDuration = m_customMessage[nMsgNum].fTime;
    m_supertext.fFadeTime = m_customMessage[nMsgNum].fFade;

    // Overridables.
    if (m_customMessage[nMsgNum].bOverrideFace)
        wcscpy_s(m_supertext.nFontFace, m_customMessage[nMsgNum].szFace);
    else
        wcscpy_s(m_supertext.nFontFace, m_customMessageFont[fontID].szFace);
    m_supertext.bItal = (m_customMessage[nMsgNum].bOverrideItal) ? (m_customMessage[nMsgNum].bItal != 0) : (m_customMessageFont[fontID].bItal != 0);
    m_supertext.bBold = (m_customMessage[nMsgNum].bOverrideBold) ? (m_customMessage[nMsgNum].bBold != 0) : (m_customMessageFont[fontID].bBold != 0);
    m_supertext.nColorR = (m_customMessage[nMsgNum].bOverrideColorR) ? m_customMessage[nMsgNum].nColorR : m_customMessageFont[fontID].nColorR;
    m_supertext.nColorG = (m_customMessage[nMsgNum].bOverrideColorG) ? m_customMessage[nMsgNum].nColorG : m_customMessageFont[fontID].nColorG;
    m_supertext.nColorB = (m_customMessage[nMsgNum].bOverrideColorB) ? m_customMessage[nMsgNum].nColorB : m_customMessageFont[fontID].nColorB;

    // Randomize color.
    m_supertext.nColorR += (int)(m_customMessage[nMsgNum].nRandR * ((warand() % 1037) / 1037.0f * 2.0f - 1.0f));
    m_supertext.nColorG += (int)(m_customMessage[nMsgNum].nRandG * ((warand() % 1037) / 1037.0f * 2.0f - 1.0f));
    m_supertext.nColorB += (int)(m_customMessage[nMsgNum].nRandB * ((warand() % 1037) / 1037.0f * 2.0f - 1.0f));
    if (m_supertext.nColorR < 0) m_supertext.nColorR = 0;
    if (m_supertext.nColorG < 0) m_supertext.nColorG = 0;
    if (m_supertext.nColorB < 0) m_supertext.nColorB = 0;
    if (m_supertext.nColorR > 255) m_supertext.nColorR = 255;
    if (m_supertext.nColorG > 255) m_supertext.nColorG = 255;
    if (m_supertext.nColorB > 255) m_supertext.nColorB = 255;

    // Fix '&'s for display.
    /*{
        int pos = 0;
        int len = wcslen_s(m_supertext.szText);
        while (m_supertext.szText[pos] && pos < 255)
        {
            if (m_supertext.szText[pos] == '&')
            {
                for (int x = len; x >= pos; x--)
                    m_supertext.szText[x + 1] = m_supertext.szText[x];
                len++;
                pos++;
            }
            pos++;
        }
    }*/

    m_supertext.fStartTime = GetTime();
}

void CPlugin::LaunchSongTitleAnim()
{
    GetWinampSongTitle(GetWinampWindow(), m_supertext.szText, ARRAYSIZE(m_supertext.szText));
    if (wcscmp(m_supertext.szText, L"Stopped.") == 0 || wcscmp(m_supertext.szText, L"Opening...") == 0 || wcscmp(m_supertext.szText, L"") == 0)
        return;
    m_supertext.bRedrawSuperText = true;
    m_supertext.bIsSongTitle = true;
    wcscpy_s(m_supertext.nFontFace, m_fontinfo[SONGTITLE_FONT].szFace);
    m_supertext.fFontSize = static_cast<float>(m_fontinfo[SONGTITLE_FONT].nSize);
    m_supertext.bBold = m_fontinfo[SONGTITLE_FONT].bBold;
    m_supertext.bItal = m_fontinfo[SONGTITLE_FONT].bItalic;
    m_supertext.fX = 0.5f;
    m_supertext.fY = 0.5f;
    m_supertext.fGrowth = 1.0f;
    m_supertext.fDuration = m_fSongTitleAnimDuration;
    m_supertext.nColorR = 255;
    m_supertext.nColorG = 255;
    m_supertext.nColorB = 255;

    m_supertext.fStartTime = GetTime();
}

bool CPlugin::LaunchSprite(int nSpriteNum, int nSlot, const std::wstring& filename, const std::vector<uint8_t>& data)
{
    char initcode[8192], code[8192], sectionA[64];
    char szTemp[8192];
    wchar_t img[512], section[64];

    initcode[0] = '\0';
    code[0] = '\0';
    img[0] = '\0';
    swprintf_s(section, L"img%02d", nSpriteNum);
    sprintf_s(sectionA, "img%02d", nSpriteNum);

    // 1. Read in image filename.
    if (nSpriteNum >= 0 && nSpriteNum < 100)
    {
        GetPrivateProfileString(section, L"img", L"", img, ARRAYSIZE(img) - 1, m_szImgIniFile);
        if (img[0] == L'\0')
        {
            wchar_t buf[1024] = {0};
            swprintf_s(buf, WASABI_API_LNGSTRINGW(IDS_SPRITE_X_ERROR_COULD_NOT_FIND_IMG_OR_NOT_DEFINED), nSpriteNum);
            AddError(buf, 7.0f, ERR_MISC, false);
            return false;
        }

        if (img[1] != L':') //|| img[2] != '\\')
        {
            // It's not in the form "x:\blah\picture.jpg" so prepend plugin dir path.
            wchar_t temp[512] = {0};
            wcscpy_s(temp, img);
            swprintf_s(img, L"%s%s", m_szMilkdrop2Path, temp);
        }
    }
    else
    {
        if (!filename.empty())
        {
            wcsncpy_s(img, filename.c_str(), 512);
        }
        else if (!data.empty())
        {
        }
        else
            return false;
    }

    // 2. Get color key.
    //UINT ck_lo = GetPrivateProfileInt(section, "colorkey_lo", 0x00000000, m_szImgIniFile);
    //UINT ck_hi = GetPrivateProfileInt(section, "colorkey_hi", 0x00202020, m_szImgIniFile);
    // FIRST try 'colorkey_lo' (for backwards compatibility) and then try 'colorkey'
    UINT ck = GetPrivateProfileInt(section, L"colorkey_lo", 0x00000000, m_szImgIniFile);
    ck = GetPrivateProfileInt(section, L"colorkey", ck, m_szImgIniFile);

    // 3. Read in init code and per-frame code.
    for (int n = 0; n < 2; n++)
    {
        char* pStr = (n == 0) ? initcode : code;
        char szLineName[32] = {0};
        size_t len;

        int line = 1;
        size_t char_pos = 0;
        bool bDone = false;

        while (!bDone)
        {
            if (n == 0)
                sprintf_s(szLineName, "init_%d", line);
            else
                sprintf_s(szLineName, "code_%d", line);

            GetPrivateProfileStringA(sectionA, szLineName, "~!@#$", szTemp, 8192, AutoChar(m_szImgIniFile));
            len = strlen(szTemp);

            if ((strcmp(szTemp, "~!@#$") == 0) || // if the key was missing,
                (len >= 8191 - char_pos - 1))     // or if out of space
            {
                bDone = true;
            }
            else
            {
                sprintf_s(&pStr[char_pos], 8192 - char_pos, "%s%c", szTemp, LINEFEED_CONTROL_CHAR);
            }

            char_pos += len + 1;
            line++;
        }
        pStr[char_pos++] = '\0'; // null-terminate
    }

    if (nSlot == -1)
    {
        // Find first empty slot; if none, chuck the oldest sprite and take its slot.
        int oldest_index = 0;
        int oldest_frame = m_texmgr.m_tex[0].nStartFrame;
        for (int x = 0; x < NUM_TEX; x++)
        {
            if (!m_texmgr.m_tex[x].pSurface)
            {
                nSlot = x;
                break;
            }
            else if (m_texmgr.m_tex[x].nStartFrame < oldest_frame)
            {
                oldest_index = x;
                oldest_frame = m_texmgr.m_tex[x].nStartFrame;
            }
        }

        if (nSlot == -1)
        {
            nSlot = oldest_index;
            m_texmgr.KillTex(nSlot);
        }
    }

    int ret = -1;
    if ((nSpriteNum >= 0 && nSpriteNum < 100) || !filename.empty())
    {
        ret = m_texmgr.LoadTex(img, nSlot, initcode, code, GetTime(), GetFrame(), ck);
    }
    else if (!data.empty())
    {
        ret = m_texmgr.LoadTex(data, nSlot, initcode, code, GetTime(), GetFrame(), ck);
    }
    else
        return false;
    m_texmgr.m_tex[nSlot].nUserData = nSpriteNum;

    wchar_t buf[1024] = {0};
    switch (ret & TEXMGR_ERROR_MASK)
    {
        case TEXMGR_ERR_SUCCESS:
            switch (ret & TEXMGR_WARNING_MASK)
            {
                case TEXMGR_WARN_ERROR_IN_INIT_CODE:
                    swprintf_s(buf, WASABI_API_LNGSTRINGW(IDS_SPRITE_X_WARNING_ERROR_IN_INIT_CODE), nSpriteNum);
                    AddError(buf, 6.0f, ERR_MISC, true);
                    break;
                case TEXMGR_WARN_ERROR_IN_REG_CODE:
                    swprintf_s(buf, WASABI_API_LNGSTRINGW(IDS_SPRITE_X_WARNING_ERROR_IN_PER_FRAME_CODE), nSpriteNum);
                    AddError(buf, 6.0f, ERR_MISC, true);
                    break;
                default:
                    // success; no errors OR warnings.
                    break;
            }
            break;
        case TEXMGR_ERR_BAD_INDEX:
            swprintf_s(buf, WASABI_API_LNGSTRINGW(IDS_SPRITE_X_ERROR_BAD_SLOT_INDEX), nSpriteNum);
            AddError(buf, 6.0f, ERR_MISC, true);
            break;
        /*
        case TEXMGR_ERR_OPENING:              sprintf_s(m_szUserMessage, "sprite #%d error: unable to open imagefile", nSpriteNum); break;
        case TEXMGR_ERR_FORMAT:               sprintf_s(m_szUserMessage, "sprite #%d error: file is corrupt or non-jpeg image", nSpriteNum); break;
        case TEXMGR_ERR_IMAGE_NOT_24_BIT:     sprintf_s(m_szUserMessage, "sprite #%d error: image does not have 3 color channels", nSpriteNum); break;
        case TEXMGR_ERR_IMAGE_TOO_LARGE:      sprintf_s(m_szUserMessage, "sprite #%d error: image is too large", nSpriteNum); break;
        case TEXMGR_ERR_CREATESURFACE_FAILED: sprintf_s(m_szUserMessage, "sprite #%d error: createsurface() failed", nSpriteNum); break;
        case TEXMGR_ERR_LOCKSURFACE_FAILED:   sprintf_s(m_szUserMessage, "sprite #%d error: lock() failed", nSpriteNum); break;
        case TEXMGR_ERR_CORRUPT_JPEG:         sprintf_s(m_szUserMessage, "sprite #%d error: jpeg is corrupt", nSpriteNum); break;
        */
        case TEXMGR_ERR_BADFILE:
            swprintf_s(buf, WASABI_API_LNGSTRINGW(IDS_SPRITE_X_ERROR_IMAGE_FILE_MISSING_OR_CORRUPT), nSpriteNum);
            AddError(buf, 6.0f, ERR_MISC, true);
            break;
        case TEXMGR_ERR_OUTOFMEM:
            swprintf_s(buf, WASABI_API_LNGSTRINGW(IDS_SPRITE_X_ERROR_OUT_OF_MEM), nSpriteNum);
            AddError(buf, 6.0f, ERR_MISC, true);
            break;
    }

    return (ret & TEXMGR_ERROR_MASK) ? false : true;
}

void CPlugin::KillSprite(int iSlot)
{
    m_texmgr.KillTex(iSlot);
}

void CPlugin::DoCustomSoundAnalysis()
{
    std::copy(m_sound.fWaveform[0].begin(), m_sound.fWaveform[0].end(), mdsound.fWave[0].begin());
    std::copy(m_sound.fWaveform[1].begin(), m_sound.fWaveform[1].end(), mdsound.fWave[1].begin());

    // Do our own [UN-NORMALIZED] fft.
    std::vector<float> fWaveLeft(NUM_AUDIO_BUFFER_SAMPLES);
    for (int i = 0; i < NUM_AUDIO_BUFFER_SAMPLES; i++)
        fWaveLeft[i] = m_sound.fWaveform[0][i];

    std::fill(mdsound.fSpecLeft.begin(), mdsound.fSpecLeft.end(), 0.0f);

    mdfft.TimeToFrequencyDomain(fWaveLeft, mdsound.fSpecLeft);
    //for (i = 0; i < NUM_FFT_SAMPLES; i++) fSpecLeft[i] = sqrtf(fSpecLeft[i] * fSpecLeft[i] + fSpecTemp[i] * fSpecTemp[i]);

    // Sum spectrum up into 3 bands.
    for (int i = 0; i < 3; i++)
    {
        // Note: only look at bottom half of spectrum!  (hence divide by 6 instead of 3)
        int start = NUM_FFT_SAMPLES * i / 6;
        int end = NUM_FFT_SAMPLES * (i + 1) / 6;
        int j;

        mdsound.imm[i] = 0;

        for (j = start; j < end; j++)
            mdsound.imm[i] += mdsound.fSpecLeft[j];
    }

    // Do temporal blending to create attenuated and super-attenuated versions.
    for (int i = 0; i < 3; i++)
    {
        float rate;

        if (mdsound.imm[i] > mdsound.avg[i])
            rate = 0.2f;
        else
            rate = 0.5f;
        rate = AdjustRateToFPS(rate, 30.0f, GetFps());
        mdsound.avg[i] = mdsound.avg[i] * rate + mdsound.imm[i] * (1 - rate);

        if (GetFrame() < 50)
            rate = 0.9f;
        else
            rate = 0.992f;
        rate = AdjustRateToFPS(rate, 30.0f, GetFps());
        mdsound.long_avg[i] = mdsound.long_avg[i] * rate + mdsound.imm[i] * (1 - rate);

        // Also get bass/mid/treble levels *relative to the past*.
        if (fabsf(mdsound.long_avg[i]) < 0.001f)
            mdsound.imm_rel[i] = 1.0f;
        else
            mdsound.imm_rel[i] = mdsound.imm[i] / mdsound.long_avg[i];

        if (fabsf(mdsound.long_avg[i]) < 0.001f)
            mdsound.avg_rel[i] = 1.0f;
        else
            mdsound.avg_rel[i] = mdsound.avg[i] / mdsound.long_avg[i];
    }
}

// Finds the pixel shader body and replaces it with custom code.
void CPlugin::GenWarpPShaderText(char* szShaderText, float decay, bool bWrap)
{
    strcpy_s(szShaderText, MAX_BIGSTRING_LEN, m_szDefaultWarpPShaderText);
    char LF = LINEFEED_CONTROL_CHAR;
    char* p = strrchr(szShaderText, '{');
    if (!p)
        return;
    p++;
    p += sprintf_s(p, MAX_BIGSTRING_LEN - (p - &szShaderText[0]), "%c", 1);

    p += sprintf_s(p, MAX_BIGSTRING_LEN - (p - &szShaderText[0]), "    // sample previous frame%c", LF);
    p += sprintf_s(p, MAX_BIGSTRING_LEN - (p - &szShaderText[0]), "    ret = tex2D( sampler%ls_main, uv ).xyz;%c", bWrap ? L"" : L"_fc", LF);
    p += sprintf_s(p, MAX_BIGSTRING_LEN - (p - &szShaderText[0]), "    %c", LF);
    p += sprintf_s(p, MAX_BIGSTRING_LEN - (p - &szShaderText[0]), "    // darken (decay) over time%c", LF);
    p += sprintf_s(p, MAX_BIGSTRING_LEN - (p - &szShaderText[0]), "    ret *= %.2f; //or try: ret -= 0.004;%c", decay, LF);
    //p += sprintf_s(p, MAX_BIGSTRING_LEN - (p - &szShaderText[0]), "    %c", LF);
    //p += sprintf_s(p, MAX_BIGSTRING_LEN - (p - &szShaderText[0]), "    ret.w = vDiffuse.w; // pass alpha along - req'd for preset blending%c", LF);
    p += sprintf_s(p, MAX_BIGSTRING_LEN - (p - &szShaderText[0]), "}%c", LF);
}

// Finds the pixel shader body and replaces it with custom code.
void CPlugin::GenCompPShaderText(char* szShaderText, float brightness, float ve_alpha, float ve_zoom, int ve_orient, float hue_shader, bool bBrighten, bool bDarken, bool bSolarize, bool bInvert)
{
    strcpy_s(szShaderText, MAX_BIGSTRING_LEN, m_szDefaultCompPShaderText);
    char LF = LINEFEED_CONTROL_CHAR;
    char* p = strrchr(szShaderText, '{');
    if (!p)
        return;
    p++;
    p += sprintf_s(p, MAX_BIGSTRING_LEN - (p - &szShaderText[0]), "%c", 1);

    if (ve_alpha > 0.001f)
    {
        int orient_x = (ve_orient % 2) ? -1 : 1;
        int orient_y = (ve_orient >= 2) ? -1 : 1;
        p += sprintf_s(p, MAX_BIGSTRING_LEN - (p - &szShaderText[0]), "    float2 uv_echo = (uv - 0.5)*%.3f*float2(%d,%d) + 0.5;%c", 1.0f / ve_zoom, orient_x, orient_y, LF);
        p += sprintf_s(p, MAX_BIGSTRING_LEN - (p - &szShaderText[0]), "    ret = lerp( tex2D(sampler_main, uv).xyz, %c", LF);
        p += sprintf_s(p, MAX_BIGSTRING_LEN - (p - &szShaderText[0]), "                tex2D(sampler_main, uv_echo).xyz, %c", LF);
        p += sprintf_s(p, MAX_BIGSTRING_LEN - (p - &szShaderText[0]), "                %.2f %c", ve_alpha, LF);
        p += sprintf_s(p, MAX_BIGSTRING_LEN - (p - &szShaderText[0]), "              ); //video echo%c", LF);
        p += sprintf_s(p, MAX_BIGSTRING_LEN - (p - &szShaderText[0]), "    ret *= %.2f; //gamma%c", brightness, LF);
    }
    else
    {
        p += sprintf_s(p, MAX_BIGSTRING_LEN - (p - &szShaderText[0]), "    ret = tex2D(sampler_main, uv).xyz;%c", LF);
        p += sprintf_s(p, MAX_BIGSTRING_LEN - (p - &szShaderText[0]), "    ret *= %.2f; //gamma%c", brightness, LF);
    }
    if (hue_shader >= 1.0f)
        p += sprintf_s(p, MAX_BIGSTRING_LEN - (p - &szShaderText[0]), "    ret *= hue_shader; //old hue shader effect%c", LF);
    else if (hue_shader > 0.001f)
        p += sprintf_s(p, MAX_BIGSTRING_LEN - (p - &szShaderText[0]), "    ret *= %.2f + %.2f*hue_shader; //old hue shader effect%c", 1 - hue_shader, hue_shader, LF);

    if (bBrighten)
        p += sprintf_s(p, MAX_BIGSTRING_LEN - (p - &szShaderText[0]), "    ret = sqrt(ret); //brighten%c", LF);
    if (bDarken)
        p += sprintf_s(p, MAX_BIGSTRING_LEN - (p - &szShaderText[0]), "    ret *= ret; //darken%c", LF);
    if (bSolarize)
        p += sprintf_s(p, MAX_BIGSTRING_LEN - (p - &szShaderText[0]), "    ret = ret*(1-ret)*4; //solarize%c", LF);
    if (bInvert)
        p += sprintf_s(p, MAX_BIGSTRING_LEN - (p - &szShaderText[0]), "    ret = 1 - ret; //invert%c", LF);
    //p += sprintf_s(p, MAX_BIGSTRING_LEN - (p - &szShaderText[0]), "    ret.w = vDiffuse.w; // pass alpha along - req'd for preset blending%c", LF);
    p += sprintf_s(p, MAX_BIGSTRING_LEN - (p - &szShaderText[0]), "}%c", LF);
}