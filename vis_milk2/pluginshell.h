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

#ifndef __NULLSOFT_DX_PLUGIN_SHELL_H__
#define __NULLSOFT_DX_PLUGIN_SHELL_H__

#include <vector>
#include "defines.h"
#include "shell_defines.h"
#include "fft.h"
#include "dxcontext.h"
#include "d3d11shim.h"

#define TIME_HIST_SLOTS 128 // number of slots used if FPS > 60. Half this many if fps == 30.
#define MAX_SONGS_PER_PAGE 40

typedef struct
{
    wchar_t szFace[256];
    int nSize; // size requested at font creation time
    int bBold;
    int bItalic;
    int bAntiAliased;
} td_fontinfo;

typedef struct
{
    float imm[2][3]; // bass, mids, treble, no damping, for each channel (long-term average is 1)
    float avg[2][3]; // bass, mids, treble, some damping, for each channel (long-term average is 1)
    float med_avg[2][3]; // bass, mids, treble, more damping, for each channel (long-term average is 1)
    float long_avg[2][3]; // bass, mids, treble, heavy damping, for each channel (long-term average is 1)
    float infinite_avg[2][3]; // bass, mids, treble: winamp's average output levels (1)
    float fWaveform[2][576]; // Not all 576 are valid! - only `NUM_WAVEFORM_SAMPLES` samples are valid for each channel (note: `NUM_WAVEFORM_SAMPLES` is declared in "shell_defines.h")
    float fSpectrum[2][NUM_FREQUENCIES]; // NUM_FREQUENCIES samples for each channel (note: NUM_FREQUENCIES is declared in `shell_defines.h`)
} td_soundinfo; // ...range is 0 Hz to 22050 Hz, evenly spaced.

class CPluginShell
{
  public:
    CPluginShell();
    virtual ~CPluginShell();

    int PluginPreInitialize(HWND hWinampWnd, HINSTANCE hWinampInstance); // called by "vis.cpp" on behalf of Winamp
    int PluginInitialize(int iWidth, int iHeight);
    int PluginRender(unsigned char* pWaveL, unsigned char* pWaveR);
    void PluginQuit();
    void OnWindowSizeChanged(int width, int height);
    void OnWindowSwap(HWND window, int width, int height);
    void OnWindowMoved();
    void OnDisplayChange();

    void ToggleFullScreen();
    void ToggleHelp();
    void TogglePlaylist();

    void ReadFont(const int n);
    void WriteFont(const int n);

    // Configuration panel and Windows messaging processes.
    static LRESULT CALLBACK WindowProc(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK DesktopWndProc(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK VJModeWndProc(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ConfigDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK TabCtrlProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK FontDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK DesktopOptionsDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK DualheadDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // GET METHODS
    // ------------------------------------------------------------
    int GetFrame(); // returns current frame number (starts at zero)
    float GetTime(); // returns current animation time (in seconds) (starts at zero) (updated once per frame)
    float GetFps(); // returns current estimate of framerate (frames per second)
    eScrMode GetScreenMode(); // returns WINDOWED, FULLSCREEN, FAKE_FULLSCREEN, DESKTOP, or NOT_YET_KNOWN (if called before or during OverrideDefaults()).
    HWND GetWinampWindow(); // returns handle to Winamp main window
    void SetWinampWindow(HWND window); // sets the Winamp main window handle
    HINSTANCE GetInstance(); // returns handle to the plugin DLL module; used for things like loading resources (dialogs, bitmaps, icons...) that are built into the plugin.
    wchar_t* GetPluginsDirPath(); // usually returns 'c:\\program files\\winamp\\plugins\\'
    wchar_t* GetConfigIniFile(); // usually returns 'c:\\program files\\winamp\\plugins\\something.ini' - filename is determined from identifiers in "defines.h"
    char* GetConfigIniFileA();

  protected:
    // GET METHODS THAT ONLY WORK ONCE DIRECTX IS READY
    // ------------------------------------------------------------
    //  The following "Get" methods are only available after DirectX has been initialized.
    //  Calling these from `OverrideDefaults()`, `MilkDropPreInitialize()`, or `MilkDropReadConfig()`,
    //  they will return NULL (zero).
    // ------------------------------------------------------------
    HWND GetPluginWindow(); // returns handle to the plugin window.  NOT persistent; can change!
    int GetWidth(); // returns width of plugin window interior, in pixels.  Note: in windowed mode, this is a fudged, larger, aligned value, and on final display, it gets cropped.
    int GetHeight(); // returns height of plugin window interior, in pixels. Note: in windowed mode, this is a fudged, larger, aligned value, and on final display, it gets cropped.
    int GetBitDepth(); // returns 8, 16, 24 (rare), or 32
    D3D11Shim* GetDevice(); // returns a pointer to the DirectX 11 device. NOT persistent; can change!
    //D3DCAPS9* GetCaps();           // returns a pointer to the D3DCAPS9 structer for the device.  NOT persistent; can change.
    //D3DFORMAT GetBackBufFormat();  // returns the pixelformat of the back buffer (probably D3DFMT_R8G8B8, D3DFMT_A8R8G8B8, D3DFMT_X8R8G8B8, D3DFMT_R5G6B5, D3DFMT_X1R5G5B5, D3DFMT_A1R5G5B5, D3DFMT_A4R4G4B4, D3DFMT_R3G3B2, D3DFMT_A8R3G3B2, D3DFMT_X4R4G4B4, or D3DFMT_UNKNOWN)
    //D3DFORMAT GetBackBufZFormat(); // returns the pixelformat of the back buffer's Z buffer (probably D3DFMT_D16_LOCKABLE, D3DFMT_D32, D3DFMT_D15S1, D3DFMT_D24S8, D3DFMT_D16, D3DFMT_D24X8, D3DFMT_D24X4S4, or D3DFMT_UNKNOWN)

    char* GetDriverFilename(); // returns a text string with the filename of the current display adapter driver, such as "nv4_disp.dll"
    char* GetDriverDescription(); // returns a text string describing the current display adapter, such as "NVIDIA GeForce4 Ti 4200"

    // PURE VIRTUAL FUNCTIONS (...must be implemented by derived classes)
    // ------------------------------------------------------------
    virtual void OverrideDefaults() = 0;
    virtual void MilkDropPreInitialize() = 0;
    virtual void MilkDropReadConfig() = 0;
    virtual void MilkDropWriteConfig() = 0;
    virtual int AllocateMilkDropNonDX11() = 0;
    virtual void CleanUpMilkDropNonDX11() = 0;
    virtual int AllocateMilkDropDX11() = 0;
    virtual void CleanUpMilkDropDX11(int final_cleanup) = 0;
    virtual void MilkDropRenderFrame(int redraw) = 0;
    virtual void MilkDropRenderUI(int* upper_left_corner_y, int* upper_right_corner_y, int* lower_left_corner_y, int* lower_right_corner_y, int xL, int xR) = 0;
    //virtual LRESULT MilkDropWindowProc(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam) = 0;
    //virtual BOOL MilkDropConfigTabProc(int nPage, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) = 0;
    //virtual void OnAltK(){}; // does not *have* to be implemented
    virtual void DumpDebugMessage(const wchar_t* s) = 0;
    virtual void PopupMessage(int message_id, int title_id, bool dump = false) = 0;
    virtual void ConsoleMessage(int message_id, int title_id) = 0;

    // FONTS & TEXT
    // ------------------------------------------------------------
  public:
    IUnknown* GetFont(eFontIndex idx); // returns a D3DX font handle for drawing text; see shell_defines.h for the definition of the 'eFontIndex' enum.
    int GetFontHeight(eFontIndex idx); // returns the height of the font, in pixels; see shell_defines.h for the definition of the 'eFontIndex' enum.
    //CTextManager m_text;

    // MISCELLANEOUS
    // ------------------------------------------------------------
  protected:
    td_soundinfo m_sound; // a structure always containing the most recent sound analysis information; defined in "pluginshell.h".
    //void SuggestHowToFreeSomeMem(); // gives the user a 'smart' message box that suggests how they can free up some video memory.

    // CONFIG PANEL SETTINGS
    // ------------------------------------------------------------
    // *** only read/write these values during CPlugin::OverrideDefaults! ***
    int m_start_fullscreen;          // 0 or 1
    int m_start_desktop;             // 0 or 1
    int m_fake_fullscreen_mode;      // 0 or 1
    int m_max_fps_fs;                // 1-120, or 0 for 'unlimited'
    int m_max_fps_dm;                // 1-120, or 0 for 'unlimited'
    int m_max_fps_w;                 // 1-120, or 0 for 'unlimited'
    int m_show_press_f1_msg;         // 0 or 1
    int m_allow_page_tearing_w;      // 0 or 1
    int m_allow_page_tearing_fs;     // 0 or 1
    int m_allow_page_tearing_dm;     // 0 or 1
    int m_minimize_winamp;           // 0 or 1
    int m_desktop_show_icons;        // 0 or 1
    int m_desktop_textlabel_boxes;   // 0 or 1
    int m_desktop_manual_icon_scoot; // 0 or 1
    int m_desktop_555_fix;           // 0 = 555, 1 = 565, 2 = 888
    int m_dualhead_horz;             // 0 = both, 1 = left, 2 = right
    int m_dualhead_vert;             // 0 = both, 1 = top, 2 = bottom
    int m_save_cpu;                  // 0 or 1
    int m_skin;                      // 0 or 1
    int m_fix_slow_text;             // 0 or 1
    int m_enable_hdr;                // 0 or 1
    int m_enable_downmix;            // 0 or 1
    int m_back_buffer_format;
    int m_depth_buffer_format;
    int m_back_buffer_count;         // 2
    int m_min_feature_level;
    td_fontinfo m_fontinfo[NUM_BASIC_FONTS + NUM_EXTRA_FONTS];
    DXGI_SAMPLE_DESC m_multisample_fullscreen;

  private:
    // GENERAL PRIVATE STUFF
    eScrMode m_screenmode; // WINDOWED, FULLSCREEN, or FAKE_FULLSCREEN (i.e. running in a full-screen-sized window)
    int m_frame; // current frame number, starting at zero
    float m_time; // current animation time in seconds; starts at zero.
    float m_fps; // current estimate of frames per second
    HWND m_hWndWinamp; // handle to Winamp window
    HINSTANCE m_hInstance; // handle to application instance
  public:
    std::unique_ptr<DXContext> m_lpDX, m_lpDX_paused; // pointer to DXContext object
    wchar_t m_szPluginsDirPath[MAX_PATH]; // usually 'c:\\program files\\winamp\\plugins\\'
    wchar_t m_szConfigIniFile[MAX_PATH]; // Unicode version: usually 'c:\\program files\\winamp\\plugins\\filename.ini' - filename is determined from identifiers in 'defines.h'
    char m_szConfigIniFileA[MAX_PATH]; // ANSI version
    //ID3D11Device* m_device;
    //ID3D11DeviceContext* m_context;
  private:
    // FONTS
    IUnknown* m_lpDDSText;
    IUnknown* m_d3dx_font[NUM_BASIC_FONTS + NUM_EXTRA_FONTS];
    IUnknown* m_d3dx_desktop_font;
    HFONT m_font[NUM_BASIC_FONTS + NUM_EXTRA_FONTS];
    HFONT m_font_desktop;

    // PRIVATE CONFIG PANEL SETTINGS
    //DXGI_MODE_DESC1 m_disp_mode_fs;  // specifies the width, height, refresh rate, and color format to use when the plugin goes fullscreen
    DXGI_SAMPLE_DESC m_multisample_desktop;
    DXGI_SAMPLE_DESC m_multisample_windowed;
    LUID m_adapter_guid_fullscreen;
    LUID m_adapter_guid_desktop;
    LUID m_adapter_guid_windowed;
    wchar_t m_adapter_devicename_fullscreen[256];
    wchar_t m_adapter_devicename_desktop[256];
    wchar_t m_adapter_devicename_windowed[256];

    // PRIVATE RUNTIME SETTINGS
    int m_lost_focus; // ~mostly for fullscreen mode
    int m_hidden;     // ~mostly for windowed mode
    int m_resizing;   // ~mostly for windowed mode
    int m_show_help;
    int m_show_playlist;
    LRESULT m_playlist_pos;        // current selection on (plugin's) playlist menu
    int m_playlist_pageups;        // can be + or -
    int m_playlist_top_idx;        // used to track when our little playlist cache (m_playlist) needs updated.
    int m_playlist_btm_idx;        // used to track when our little playlist cache (m_playlist) needs updated.
    float m_playlist_width_pixels; // considered invalid whenever 'm_playlist_top_idx' is -1.
    wchar_t m_playlist[MAX_SONGS_PER_PAGE][256]; // considered invalid whenever 'm_playlist_top_idx' is -1.
    int m_exiting;
    int m_upper_left_corner_y;
    int m_lower_left_corner_y;
    int m_upper_right_corner_y;
    int m_lower_right_corner_y;
    int m_left_edge;
    int m_right_edge;
    int m_force_accept_WM_WINDOWPOSCHANGING;

    // PRIVATE - GDI STUFF
    HMENU m_main_menu;
    HMENU m_context_menu;

    // PRIVATE - MORE TIMEKEEPING
  protected:
    double m_last_raw_time;
    LARGE_INTEGER m_high_perf_timer_freq; // 0 if high-precision timer not available
  private:
    float m_time_hist[TIME_HIST_SLOTS]; // cumulative
    int m_time_hist_pos;
    LARGE_INTEGER m_prev_end_of_frame;

    // PRIVATE AUDIO PROCESSING DATA
    FFT m_fftobj;
    float m_oldwave[2][576]; // for wave alignment
    int m_prev_align_offset[2]; // for wave alignment
    int m_align_weights_ready;

    void DrawAndDisplay(int redraw);
    void ReadConfig();
    void WriteConfig();
    void DoTime();
    void AnalyzeNewSound(unsigned char* pWaveL, unsigned char* pWaveR);
    void AlignWaves();
    int InitDirectX();
    void CleanUpDirectX();
    int AllocateDX11();
    void CleanUpDX11(int final_cleanup);
    int InitNonDX11();
    void CleanUpNonDX11();
    //int  AllocateFonts(IDirect3DDevice9 *pDevice);
    void CleanUpFonts();
    void AllocateTextSurface();
    void ToggleDesktop();
    void RenderBuiltInTextMsgs();
    int GetCanvasMarginX(); // returns the number of pixels that exist on the canvas, on each side, that the user will never see. Mainly used in windowed mode, where sometimes, up to 15 pixels get cropped at edges of the screen.
    int GetCanvasMarginY(); // returns the number of pixels that exist on the canvas, on each side, that the user will never see. Mainly used in windowed mode, where sometimes, up to 15 pixels get cropped at edges of the screen.
  public:
    void DrawDarkTranslucentBox(RECT* pr);
    void StuffParams(DXCONTEXT_PARAMS* pParams);

  protected:
    void RenderPlaylist();
    void EnforceMaxFPS();

    // DESKTOP MODE FUNCTIONS (found in desktop_mode.cpp)
    //int InitDesktopMode();
    //void CleanUpDesktopMode();
    //int CreateDesktopIconTexture(IDirect3DTexture9** ppTex);
    void DeselectDesktop();
    void UpdateDesktopBitmaps();
    int StuffIconBitmaps(int iStartIconIdx, int iTexNum, int* show_msgs);
    void RenderDesktop();

    // SEPARATE TEXT WINDOW (FOR VJ MODE)
    int m_vj_mode;
    int m_hidden_textwnd;
    int m_resizing_textwnd;
    HWND m_hTextWnd;
  private:
    int m_nTextWndWidth;
    int m_nTextWndHeight;
    bool m_bTextWindowClassRegistered;
    //LPDIRECT3D9 m_vjd3d9;
    //D3D11Shim* m_vjd3d9_device;
    //HDC m_memDC; // memory device context
    //HBITMAP m_memBM, m_oldBM;
    //HBRUSH  m_hBlackBrush;

    // WINDOWPROC FUNCTIONS
    //LRESULT PluginShellWindowProc(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam); // in "windowproc.cpp"
    //LRESULT PluginShellDesktopWndProc(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam);
    //LRESULT PluginShellVJModeWndProc(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam);

    // CONFIGURATION PANEL FUNCTIONS
    LRESULT PluginShellConfigDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT PluginShellConfigTab1Proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT PluginShellFontDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT PluginShellDesktopOptionsDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT PluginShellDualheadDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    bool InitConfig(HWND hDialogWnd);
    void EndConfig();
    void UpdateAdapters(int screenmode);
    void UpdateFSAdapterDispModes(); // fullscreen-only
    void UpdateDispModeMultiSampling(int screenmode);
    void UpdateMaxFps(int screenmode);
    int GetCurrentlySelectedAdapter(int screenmode);
    void SaveDisplayMode();
    void SaveMultiSamp(int screenmode);
    void SaveAdapter(int screenmode);
    void SaveMaxFps(int screenmode);
    void OnTabChanged(int nNewTab);
    //D3D11Shim* GetTextDevice() { return (m_vjd3d9_device) ? m_vjd3d9_device : m_lpDX->m_lpDevice.get(); }

    // CHANGES
    friend class CShaderParams;
};

#endif