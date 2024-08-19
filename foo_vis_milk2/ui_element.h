/*
 * ui_element.h - Implements the MilkDrop 2 visualization component header.
 *
 * Copyright (c) 2023-2024 Jimmy Cassis
 * SPDX-License-Identifier: MPL-2.0
 */

#pragma once

#include "config.h"
#include "version.h"
#include "steptimer.h"
#define TIMER_TP

// Anonymous namespace is standard practice in foobar2000 components
// to prevent name collisions.
namespace
{
static const GUID guid_milk2 = {
    0x204b0345, 0x4df5, 0x4b47, {0xad, 0xd3, 0x98, 0x9f, 0x81, 0x1b, 0xd9, 0xa5}
}; // {204B0345-4DF5-4B47-ADD3-989F811BD9A5}

static const GUID VisMilk2LangGUID = {
    0xc5d175f1, 0xe4e4, 0x47ee, {0xb8, 0x5c, 0x4e, 0xdc, 0x6b, 0x2, 0x6a, 0x35}
}; // {C5D175F1-E4E4-47EE-B85C-4EDC6B026A35}

static bool s_fullscreen = false;
static bool s_in_toggle = false;
static bool s_was_topmost = false;
static bool s_milk2 = false;
static ULONGLONG s_count = 0ull;
static constexpr ULONGLONG s_debug_limit = 1ull;
static milk2_config s_config;
static std::wstring s_pwd;
#ifdef TIMER_TP
CRITICAL_SECTION s_cs;
#endif

#pragma region UI Element
class milk2_ui_element : public ui_element_instance, public CWindowImpl<milk2_ui_element>, private play_callback_impl_base, private playlist_callback_impl_base
{
  public:
    DECLARE_WND_CLASS_EX(CLASSNAME, CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS, (-1));

    void initialize_window(HWND parent)
    {
#ifdef _DEBUG
        WCHAR szParent[19]{}, szWnd[19]{};
        swprintf_s(szParent, TEXT("0x%p"), parent);
        swprintf_s(szWnd, TEXT("0x%p"), get_wnd());
        MILK2_CONSOLE_LOG("Init ", szParent, ", ", szWnd)
#endif
        WIN32_OP(Create(parent, nullptr, SHORTNAME, 0, WS_EX_STATICEDGE) != NULL);
    }

    // clang-format off
    BEGIN_MSG_MAP_EX(milk2_ui_element)
#ifdef _DEBUG
        OutputDebugMessage(APPLICATION_FILE_NAME ": ", get_wnd(), uMsg, wParam, lParam);
#endif
        MSG_WM_CREATE(OnCreate)
        MSG_WM_CLOSE(OnClose)
        MSG_WM_DESTROY(OnDestroy)
        MSG_WM_TIMER(OnTimer)
        MSG_WM_PAINT(OnPaint)
        MSG_WM_ERASEBKGND(OnEraseBkgnd)
        MSG_WM_SIZE(OnSize)
        MSG_WM_MOVE(OnMove)
        MSG_WM_ENTERSIZEMOVE(OnEnterSizeMove)
        MSG_WM_EXITSIZEMOVE(OnExitSizeMove)
        MSG_WM_NCCALCSIZE(OnNcCalcSize)
        MSG_WM_COPYDATA(OnCopyData)
        MSG_WM_DISPLAYCHANGE(OnDisplayChange)
        MSG_WM_DPICHANGED(OnDpiChanged)
        MSG_WM_GETMINMAXINFO(OnGetMinMaxInfo);
        MSG_WM_ACTIVATEAPP(OnActivateApp)
        MSG_WM_CHAR(OnChar)
        MSG_WM_KEYDOWN(OnKeyDown)
        MSG_WM_SYSKEYDOWN(OnSysKeyDown)
        MSG_WM_SYSCHAR(OnSysChar)
        MSG_WM_COMMAND(OnCommand)
        MSG_WM_GETDLGCODE(OnGetDlgCode)
        MSG_WM_SETFOCUS(OnSetFocus)
        MSG_WM_KILLFOCUS(OnKillFocus)
        MSG_WM_CONTEXTMENU(OnContextMenu)
        MSG_WM_LBUTTONDBLCLK(OnLButtonDblClk)
        MSG_WM_POWERBROADCAST(OnPowerBroadcast)
        MESSAGE_HANDLER_EX(WM_IME_NOTIFY, OnImeNotify)
        MESSAGE_HANDLER_EX(WM_QUIT, OnQuit)
        MESSAGE_HANDLER_EX(WM_MILK2, OnMilk2Message)
        MESSAGE_HANDLER_EX(WM_CONFIG_CHANGE, OnConfigurationChange)
    END_MSG_MAP()
    // clang-format on

    milk2_ui_element(ui_element_config::ptr config, ui_element_instance_callback_ptr p_callback);
    HWND get_wnd() { return *this; }
    void set_configuration(ui_element_config::ptr p_data);
    ui_element_config::ptr get_configuration();
    static GUID g_get_guid() { return guid_milk2; }
    static GUID g_get_subclass() { return ui_element_subclass_playback_visualisation; }
    static void g_get_name(pfc::string_base& out) { out = "MilkDrop"; }
    static ui_element_config::ptr g_get_default_configuration(); //{ return ui_element_config::g_create_empty(g_get_guid()); }
    static const char* g_get_description() { return "MilkDrop 2 Visualization using DirectX 11."; }

    void notify(const GUID& p_what, size_t p_param1, const void* p_param2, size_t p_param2size);

  protected:
    const ui_element_instance_callback_ptr m_callback;

  private:
    int OnCreate(LPCREATESTRUCT cs);
    void OnClose();
    void OnDestroy();
    void OnTimer(UINT_PTR nIDEvent);
    void OnPaint(CDCHandle dc);
    BOOL OnEraseBkgnd(CDCHandle dc);
    void OnSize(UINT nType, CSize size);
    void OnMove(CPoint ptPos);
    void OnEnterSizeMove();
    void OnExitSizeMove();
    LRESULT OnNcCalcSize(BOOL bCalcValidRects, LPARAM lParam);
    BOOL OnCopyData(CWindow wnd, PCOPYDATASTRUCT pCopyDataStruct);
    void OnDisplayChange(UINT uBitsPerPixel, CSize sizeScreen);
    void OnDpiChanged(UINT nDpiX, UINT nDpiY, PRECT pRect);
    void OnGetMinMaxInfo(LPMINMAXINFO lpMMI);
    void OnActivateApp(BOOL bActive, DWORD dwThreadID);
    BOOL OnPowerBroadcast(DWORD dwPowerEvent, DWORD_PTR dwData);
    void OnChar(TCHAR chChar, UINT nRepCnt, UINT nFlags);
    void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    void OnSysChar(TCHAR chChar, UINT nRepCnt, UINT nFlags);
    void OnCommand(UINT uNotifyCode, int nID, CWindow wndCtl);
    UINT OnGetDlgCode(LPMSG lpMsg);
    void OnSetFocus(CWindow wndOld);
    void OnKillFocus(CWindow wndFocus);
    void OnContextMenu(CWindow wnd, CPoint point);
    void OnLButtonDblClk(UINT nFlags, CPoint point);
    LRESULT OnImeNotify(UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT OnQuit(UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT OnMilk2Message(UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT OnConfigurationChange(UINT uMsg, WPARAM wParam, LPARAM lParam);

    PWCHAR GetWnd() { swprintf_s(m_szWnd, TEXT("0x%p"), get_wnd()); return m_szWnd; }

    //ui_element_config::ptr m_config;
    visualisation_stream_v3::ptr m_vis_stream;

    bool m_in_sizemove;
    bool m_in_suspend;
    bool m_minimized;

    DWORD m_refresh_interval;
    double m_last_time;

#ifndef TIMER_TP
    // Win32 timer
    ULONGLONG m_last_refresh;
    enum milk2_ui_timer
    {
        ID_REFRESH_TIMER = 1
    };
#endif

    enum milk2_ui_menu_id
    {
        IDM_TOGGLE_FULLSCREEN = ID_GO_FS, // ID_VIS_FS
        IDM_CURRENT_PRESET = 1,
        IDM_NEXT_PRESET = ID_VIS_NEXT,
        IDM_PREVIOUS_PRESET = ID_VIS_PREV,
        IDM_LOCK_PRESET = 2,
        IDM_SHUFFLE_PRESET = ID_VIS_RANDOM,
        IDM_ENABLE_DOWNMIX = 3,
        IDM_SHOW_HELP = ID_SHOWHELP, // ID_VIS_CFG
        IDM_SHOW_PLAYLIST = ID_SHOWPLAYLIST,
        IDM_QUIT = ID_QUIT
    };

    // Initialization and management
    bool Initialize(HWND window, int width, int height);
    void ReadConfig();

    // Visualization loop
    void Tick();
    void Update(DX::StepTimer const& timer);
    HRESULT Render();
    void Clear();
    void BuildWaves();

    // Preset information and navigation
    void PrevPreset(float fBlendTime = s_config.settings.m_fBlendTimeUser);
    void NextPreset(float fBlendTime = s_config.settings.m_fBlendTimeUser);
    bool LoadPreset(int select);
    void RandomPreset(float fBlendTime = s_config.settings.m_fBlendTimeUser);
    void LockPreset(bool lockUnlock);
    bool IsPresetLock();
    std::wstring GetCurrentPreset();
    void SetPresetRating(float inc_dec);
    void Seek(UINT nRepCnt, bool bShiftHeldDown, double seekDelta);

    // Window Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;
    void SetPwd(std::wstring pwd) noexcept;
    void UpdateChannelMode();
    void ToggleFullScreen();
    void ToggleHelp();
    void TogglePlaylist();
    void ToggleSongTitle();
    void ToggleSongLength();
    void TogglePresetInfo();
    void ToggleFps();
    void ToggleRating();
    void ToggleShaderHelp();
    const char* ToggleShuffle(bool forward);

    // MilkDrop status
    bool m_milk2;
    WCHAR m_szWnd[19]; // 19 = 2 ("0x") + 16 (64 / 4 -> 64-bit address in hexadecimal) + 1 ('\0')
    std::wstring m_szBuffer;

    // Rendering loop timer
    DX::StepTimer m_timer;

#ifdef TIMER_TP
    // Thread pool timer
    void StartTimer() noexcept;
    void StopTimer() noexcept;
    static VOID CALLBACK TimerCallback(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_TIMER Timer) noexcept;
    PTP_TIMER m_tpTimer;
#endif

    // Topmost setting
    bool SetTopMost() noexcept;

    // Component paths
    static void resolve_pwd();
    std::wstring m_pwd;

    // Audio data
    unsigned char waves[2][576];

    // Playback control
    static_api_ptr_t<playback_control> m_playback_control;
    titleformat_object::ptr m_script;
    titleformat_object::ptr m_search;
    pfc::string_formatter m_state;

    // Playback callback methods.
    void on_playback_starting(play_control::t_track_command p_command, bool p_paused) { UpdateTrack(); }
    void on_playback_new_track(metadb_handle_ptr p_track) { UpdateTrack(); }
    void on_playback_stop(play_control::t_stop_reason p_reason) { UpdateTrack(); }

    void UpdateTrack();

    // Playlist callback methods.
    void on_items_added(size_t p_playlist, size_t p_start, metadb_handle_list_cref p_data, const bit_array& p_selection) { UpdatePlaylist(); }
    void on_items_reordered(size_t p_playlist, const size_t* p_order, size_t p_count) { UpdatePlaylist(); }
    void on_items_removed(size_t p_playlist, const bit_array& p_mask, size_t p_old_count, size_t p_new_count) { UpdatePlaylist(); }
    void on_items_selection_change(size_t p_playlist, const bit_array& p_affected, const bit_array& p_state) { UpdatePlaylist(); }
    void on_item_focus_change(size_t p_playlist, size_t p_from, size_t p_to) { UpdatePlaylist(); }
    void on_items_modified(size_t p_playlist, const bit_array& p_mask) { UpdatePlaylist(); }
    void on_playlist_activate(t_size p_old, t_size p_new) { UpdatePlaylist(); }
    void on_playlists_reorder(const t_size* p_order, t_size p_count) { UpdatePlaylist(); }
    void on_playlists_removed(const bit_array& p_mask, t_size p_old_count, t_size p_new_count) { UpdatePlaylist(); }
    void on_playback_order_changed(t_size p_new_index) { UpdatePlaylist(); }

    void UpdatePlaylist();
    void SetSelectionSingle(size_t idx);
    void SetSelectionSingle(size_t idx, bool toggle, bool focus, bool single_only);
};
} // namespace