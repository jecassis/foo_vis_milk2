/*
 *  ui_element.cpp - Implements the MilkDrop 2 visualization component.
 */

#include "pch.h"
#include "config.h"
#include "steptimer.h"
#include "resource.h"
#include "version.h"

//#ifdef __clang__
//#pragma clang diagnostic ignored "-Wcovered-switch-default"
//#pragma clang diagnostic ignored "-Wswitch-enum"
//#endif

CPlugin g_plugin;
HWND g_hWindow;

// Indicates to hybrid graphics systems to prefer the discrete part by default.
//extern "C" {
//__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
//__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
//}

// Anonymous namespace is standard practice in foobar2000 components
// to prevent name collisions between them.
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
static bool s_milk2 = false;
static ULONGLONG s_count = 0ull;
static constexpr ULONGLONG s_debug_limit = 1ull;
static milk2_config s_config;

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
        WIN32_OP(Create(parent, nullptr, nullptr, 0, WS_EX_STATICEDGE) != NULL);
    }

    // clang-format off
    BEGIN_MSG_MAP_EX(milk2_ui_element)
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
        MSG_WM_COPYDATA(OnCopyData)
        MSG_WM_DISPLAYCHANGE(OnDisplayChange)
        MSG_WM_DPICHANGED(OnDpiChanged)
        MSG_WM_GETMINMAXINFO(OnGetMinMaxInfo);
        MSG_WM_ACTIVATEAPP(OnActivateApp)
        MSG_WM_CHAR(OnChar)
        MSG_WM_KEYDOWN(OnKeyDown)
        MSG_WM_SYSKEYDOWN(OnSysKeyDown)
        MSG_WM_SYSCHAR(OnSysChar)
        MSG_WM_GETDLGCODE(OnGetDlgCode)
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
    UINT OnGetDlgCode(LPMSG lpMsg);
    void OnContextMenu(CWindow wnd, CPoint point);
    void OnLButtonDblClk(UINT nFlags, CPoint point);
    LRESULT OnImeNotify(UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT OnQuit(UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT OnMilk2Message(UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT OnConfigurationChange(UINT uMsg, WPARAM wParam, LPARAM lParam);
    PWCHAR GetWnd() { swprintf_s(m_szWnd, TEXT("0x%p"), get_wnd()); return m_szWnd; }

    //ui_element_config::ptr m_config;
    visualisation_stream_v3::ptr m_vis_stream;

    ULONGLONG m_last_refresh;
    DWORD m_refresh_interval;
    double m_last_time;

    bool s_in_sizemove;
    bool s_in_suspend;
    bool s_minimized;

    enum milk2_ui_timer
    {
        ID_REFRESH_TIMER = 1
    };

    enum milk2_ui_menu_id
    {
        IDM_TOGGLE_FULLSCREEN = 1,
        IDM_CURRENT_PRESET,
        IDM_NEXT_PRESET,
        IDM_PREVIOUS_PRESET,
        IDM_LOCK_PRESET,
        IDM_SHUFFLE_PRESET,
        IDM_ENABLE_DOWNMIX,
        IDM_SHOW_HELP,
        IDM_SHOW_PLAYLIST
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
    void SetPwd(std::string pwd) noexcept;
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

    // Component paths
    std::string m_pwd;

    // Audio data
    unsigned char waves[2][576];

  protected:
    const ui_element_instance_callback_ptr m_callback;

  private:
    // Playback control
    static_api_ptr_t<playback_control> m_playback_control;
    titleformat_object::ptr m_script;
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

milk2_ui_element::milk2_ui_element(ui_element_config::ptr config, ui_element_instance_callback_ptr p_callback) :
    m_callback(p_callback), 
    m_bMsgHandled(TRUE),
    play_callback_impl_base(flag_on_playback_starting | flag_on_playback_new_track | flag_on_playback_stop),
    playlist_callback_impl_base(flag_on_items_added | flag_on_items_reordered | flag_on_items_removed | flag_on_items_selection_change |
                                flag_on_item_focus_change | flag_on_items_modified | flag_on_playlist_activate | flag_on_playlists_reorder |
                                flag_on_playlists_removed | flag_on_playback_order_changed)
{
    m_milk2 = false;
    m_refresh_interval = 33;
    m_last_time = 0.0;
    s_in_sizemove = false;
    s_in_suspend = false;
    s_minimized = false;

    m_pwd = ".\\";
    set_configuration(config);
}

ui_element_config::ptr milk2_ui_element::g_get_default_configuration()
{
    try
    {
        ui_element_config_builder builder;
        milk2_config config;
        config.build(builder);
        return builder.finish(g_get_guid());
    }
    catch (exception_io& exc)
    {
        FB2K_console_print(core_api::get_my_file_name(), ": Exception while building default configuration data - ", exc);
        return ui_element_config::g_create_empty(g_get_guid());
    }
}

void milk2_ui_element::set_configuration(ui_element_config::ptr p_data)
{
    //LPVOID dataptr = const_cast<LPVOID>(p_data->get_data());
    //if (dataptr && p_data->get_data_size() >= 4 && static_cast<DWORD*>(dataptr)[0] == ('M' << 24 | 'I' << 16 | 'L' << 8 | 'K'))
    //    s_config = p_data;
    //else
    //    s_config = g_get_default_configuration();

    if (s_milk2)
        return;

    ui_element_config_parser parser(p_data);
    s_config.parse(parser);
}

ui_element_config::ptr milk2_ui_element::get_configuration()
{
    ui_element_config_builder builder;
    s_config.build(builder);
    return builder.finish(g_get_guid());
}

void milk2_ui_element::notify(const GUID& p_what, size_t p_param1, const void* p_param2, size_t p_param2size)
{
    if (p_what == ui_element_notify_colors_changed || p_what == ui_element_notify_font_changed)
        Invalidate();
}

int milk2_ui_element::OnCreate(LPCREATESTRUCT cs)
{
    MILK2_CONSOLE_LOG("OnCreate ", cs->x, ", ", cs->y, ", ", GetWnd())

    if (!s_milk2)
        s_config.init();

    if (!m_milk2)
    {
        std::string base_path = core_api::get_my_full_path();
        std::string::size_type t = base_path.rfind('\\');
        if (t != std::string::npos)
            base_path.erase(t + 1);
        SetPwd(base_path);

        try
        {
            static_api_ptr_t<visualisation_manager> vis_manager;

            vis_manager->create_stream(m_vis_stream, 0);

            m_vis_stream->request_backlog(0.8);
            UpdateChannelMode();
        }
        catch (std::exception& exc)
        {
            FB2K_console_print(core_api::get_my_file_name(), ": Exception while creating visualization stream - ", exc);
        }
    }

    HRESULT hr = S_OK;
    int w, h;
    GetDefaultSize(w, h);

    CRect r{};
    WIN32_OP_D(GetClientRect(&r))
    if (r.right - r.left > 0 && r.bottom - r.top > 0)
    {
        w = r.right - r.left;
        h = r.bottom - r.top;
    }
    if (!Initialize(get_wnd(), w, h))
    {
        FB2K_console_print(core_api::get_my_file_name(), ": Could not initialize MilkDrop");
    }
    MILK2_CONSOLE_LOG("OnCreate2 ", r.right, ", ", r.left, ", ", r.top, ", ", r.bottom, ", ", GetWnd())

    return hr;
}

void milk2_ui_element::OnClose()
{
    MILK2_CONSOLE_LOG("OnClose ", GetWnd())
    //DestroyWindow();
}

void milk2_ui_element::OnDestroy()
{
    MILK2_CONSOLE_LOG("OnDestroy ", GetWnd())
    m_vis_stream.release();
    //DestroyMenu();

    if (m_milk2)
        m_milk2 = false;
    s_count = 0ull;

    if (!s_in_toggle)
    {
        MILK2_CONSOLE_LOG("ExitVis")
        s_fullscreen = false;
        s_in_toggle = false;
        s_milk2 = false;
        KillTimer(ID_REFRESH_TIMER);
        g_plugin.PluginQuit();
        //PostQuitMessage(0);
    }
    else
    {
        s_in_toggle = false;
    }
}

void milk2_ui_element::OnTimer(UINT_PTR nIDEvent)
{
    MILK2_CONSOLE_LOG_LIMIT("OnTimer ", GetWnd())
    KillTimer(ID_REFRESH_TIMER);
    InvalidateRect(NULL, TRUE);
}

void milk2_ui_element::OnPaint(CDCHandle dc)
{
    MILK2_CONSOLE_LOG_LIMIT("OnPaint ", GetWnd())
    if (s_in_sizemove && m_milk2) // foobar2000 does not enter/exit size/move
    {
        Tick();
    }
    else
    {
        PAINTSTRUCT ps;
        std::ignore = BeginPaint(&ps);
        EndPaint(&ps);
    }
    ValidateRect(NULL);

    ULONGLONG now = GetTickCount64();
    if (m_vis_stream.is_valid())
    {
        BuildWaves();
        ULONGLONG next_refresh = m_last_refresh + m_refresh_interval;
        // (next_refresh < now) would break when GetTickCount() overflows
        if (static_cast<LONGLONG>(next_refresh - now) < 0)
        {
            next_refresh = now;
        }
        SetTimer(ID_REFRESH_TIMER, static_cast<UINT>(next_refresh - now));
    }
    m_last_refresh = now;
}

BOOL milk2_ui_element::OnEraseBkgnd(CDCHandle dc)
{
    MILK2_CONSOLE_LOG_LIMIT("OnEraseBkgnd ", GetWnd())
    ++s_count;

#if 0
    CRect r;
    WIN32_OP_D(GetClientRect(&r));
    CBrush brush;
    WIN32_OP_D(brush.CreateSolidBrush(m_callback->query_std_color(ui_color_background)) != NULL);
    WIN32_OP_D(dc.FillRect(&r, brush));
#else
    if (!m_milk2 && !s_fullscreen && s_milk2)
    {
        HRESULT hr = S_OK;
        int w, h;
        GetDefaultSize(w, h);

        CRect r{};
        WIN32_OP_D(GetClientRect(&r))
        if (r.right - r.left > 0 && r.bottom - r.top > 0)
        {
            w = r.right - r.left;
            h = r.bottom - r.top;
        }
        if (!Initialize(get_wnd(), w, h))
        {
            FB2K_console_print(core_api::get_my_file_name(), ": Could not initialize MilkDrop");
        }
    }
    Tick();
#endif

    return TRUE;
}

void milk2_ui_element::OnMove(CPoint ptPos)
{
    MILK2_CONSOLE_LOG("OnMove ", GetWnd())
    if (m_milk2)
    {
        g_plugin.OnWindowMoved();
    }
}

void milk2_ui_element::OnSize(UINT nType, CSize size)
{
    MILK2_CONSOLE_LOG("OnSize ", nType, ", ", size.cx, ", ", size.cy, ", ", GetWnd());
    if (nType == SIZE_MINIMIZED)
    {
        if (!s_minimized)
        {
            s_minimized = true;
            if (!s_in_suspend && m_milk2)
                OnSuspending();
            s_in_suspend = true;
        }
    }
    else if (s_minimized)
    {
        s_minimized = false;
        if (s_in_suspend && m_milk2)
            OnResuming();
        s_in_suspend = false;
    }
    else if (!s_in_sizemove && m_milk2)
    {
        int width = size.cx;
        int height = size.cy;

        if (!width || !height)
            return;
        if (width < 128)
            width = 128;
        if (height < 128)
            height = 128;
        MILK2_CONSOLE_LOG("OnSize2 ", nType, ", ", size.cx, ", ", size.cy, ", ", GetWnd())
        g_plugin.OnWindowSizeChanged(size.cx, size.cy);
    }
}

void milk2_ui_element::OnEnterSizeMove()
{
    MILK2_CONSOLE_LOG("OnEnterSizeMove ", GetWnd())
    s_in_sizemove = true;
}

void milk2_ui_element::OnExitSizeMove()
{
    MILK2_CONSOLE_LOG("OnExitSizeMove ", GetWnd())
    s_in_sizemove = false;
    if (m_milk2)
    {
        RECT rc;
        WIN32_OP_D(GetClientRect(&rc));
        g_plugin.OnWindowSizeChanged(rc.right - rc.left, rc.bottom - rc.top);
    }
}

BOOL milk2_ui_element::OnCopyData(CWindow wnd, PCOPYDATASTRUCT pcds)
{
    typedef struct
    {
        wchar_t error[1024];
    } ErrorCopy;

    MILK2_CONSOLE_LOG("OnCopyData ", GetWnd())
    switch (pcds->dwData)
    {
        case 0x09: // PRINT STDOUT
            {
                LPCTSTR lpszString = (LPCTSTR)((ErrorCopy*)(pcds->lpData))->error;
                FB2K_console_print(core_api::get_my_file_name(), ": ", lpszString);
                break;
            }
    }

    return TRUE;
}

void milk2_ui_element::OnDisplayChange(UINT uBitsPerPixel, CSize sizeScreen)
{
    MILK2_CONSOLE_LOG("OnDisplayChange ", GetWnd())
    if (m_milk2)
    {
        g_plugin.OnDisplayChange();
    }
}

void milk2_ui_element::OnDpiChanged(UINT nDpiX, UINT nDpiY, PRECT pRect)
{
}

void milk2_ui_element::OnGetMinMaxInfo(LPMINMAXINFO lpMMI)
{
    lpMMI->ptMinTrackSize.x = 320;
    lpMMI->ptMinTrackSize.y = 200;
}

void milk2_ui_element::OnActivateApp(BOOL bActive, DWORD dwThreadID)
{
    MILK2_CONSOLE_LOG("OnActivateApp ", GetWnd())
    if (m_milk2)
    {
        if (bActive)
        {
            OnActivated();
        }
        else
        {
            OnDeactivated();
        }
    }
}

BOOL milk2_ui_element::OnPowerBroadcast(DWORD dwPowerEvent, DWORD_PTR dwData)
{
    MILK2_CONSOLE_LOG("OnPowerBroadcast ", GetWnd())
    switch (dwPowerEvent)
    {
        case PBT_APMQUERYSUSPEND:
            if (!s_in_suspend && m_milk2)
                OnSuspending();
            s_in_suspend = true;
            break;
        case PBT_APMRESUMESUSPEND:
            if (!s_minimized)
            {
                if (s_in_suspend && m_milk2)
                    OnResuming();
                s_in_suspend = false;
            }
            break;
        default:
            break;
    }

    return TRUE;
}

void milk2_ui_element::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    MILK2_CONSOLE_LOG("OnLButtonDblClk ", GetWnd())
    ToggleFullScreen();
}

void milk2_ui_element::OnChar(TCHAR chChar, UINT nRepCnt, UINT nFlags)
{
    MILK2_CONSOLE_LOG("OnChar ", GetWnd())
    if (g_plugin.m_show_playlist)
    {
        auto api = playlist_manager::get();
        switch (chChar)
        {
            case 'j':
            case 'J':
                {
                    size_t pos = api->activeplaylist_get_focus_item();
                    if (pos == pfc::infinite_size)
                        pos = -1;
                    g_plugin.m_playlist_pos = static_cast<LRESULT>(pos);
                }
                return;
            default:
                {
                    int nSongs = static_cast<int>(api->activeplaylist_get_item_count());
                    bool found = false;
                    LRESULT orig_pos = g_plugin.m_playlist_pos;
                    int inc = (chChar >= 'A' && chChar <= 'Z') ? -1 : 1;
                    while (true)
                    {
                        if (inc == 1 && g_plugin.m_playlist_pos >= nSongs - 1)
                            break;
                        if (inc == -1 && g_plugin.m_playlist_pos <= 0)
                            break;
                        g_plugin.m_playlist_pos += inc;

                        if (m_script.is_empty())
                        {
                            pfc::string8 pattern = "%title%";
                            static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(m_script, pattern);
                        }
                        pfc::string_formatter state;
                        metadb_handle_list list;
                        api->activeplaylist_get_all_items(list);
                        if (!(list.get_item(static_cast<size_t>(g_plugin.m_playlist_pos)))->format_title(NULL, state, m_script, NULL))
                            state = "";
                        m_szBuffer = pfc::wideFromUTF8(state);

                        TCHAR buf[32];
                        wcsncpy_s(buf, m_szBuffer.c_str(), 31);
                        buf[31] = L'\0';

                        // Remove song number and period from beginning.
                        PTCHAR p = buf;
                        while (*p >= '0' && *p <= '9')
                            ++p;
                        if (*p == '.' && *(p + 1) == ' ')
                        {
                            p += 2;
                            int pos = 0;
                            while (*p != L'\0')
                            {
                                buf[pos++] = *p;
                                p++;
                            }
                            buf[pos++] = L'\0';
                        }

                        TCHAR chChar2 = (chChar >= 'A' && chChar <= 'Z') ? (chChar + 'a' - 'A') : (chChar + 'A' - 'a');
                        if (unsigned(buf[0]) == chChar || unsigned(buf[0]) == chChar2)
                        {
                            found = true;
                            break;
                        }
                    }

                    if (!found)
                        g_plugin.m_playlist_pos = orig_pos;
                }
                return;
        }
    }
    else
    {
        wchar_t buf[256] = {0};
        switch (chChar)
        {
            case 'z':
            case 'Z':
                m_playback_control->previous();
                return;
            case 'x':
            case 'X':
                m_playback_control->start();
                return;
            case 'c':
            case 'C':
                m_playback_control->toggle_pause();
                return;
            case 'v':
            case 'V':
                m_playback_control->stop();
                return;
            case 'b':
            case 'B':
                m_playback_control->next();
                return;
            case 'u': //g_plugin.m_pState->m_fWarpScale /= 1.1f; return;
            case 'U': //g_plugin.m_pState->m_fWarpScale *= 1.1f; return;
                {
                    const char* szMode = ToggleShuffle(chChar == 'u' || chChar == 'U');
                    swprintf_s(buf, TEXT("Playback Order: %hs"), szMode);
                    g_plugin.AddError(buf, 3.0f, ERR_NOTIFY, false);
                }
                return;
            case 'r':
            case 'R':
                g_plugin.m_bSequentialPresetOrder = !g_plugin.m_bSequentialPresetOrder;
                {
                    LoadString(core_api::get_my_instance(), IDS_PRESET_ORDER_IS_NOW_X, &buf[64], 64);
                    LoadString(core_api::get_my_instance(), g_plugin.m_bSequentialPresetOrder ? IDS_SEQUENTIAL : IDS_RANDOM, &buf[128], 64);
                    swprintf_s(buf, 64, &buf[64], &buf[128]);
                    g_plugin.AddError(buf, 3.0f, ERR_NOTIFY, false);
                }
                // Erase all history, too.
                g_plugin.m_presetHistory[0] = g_plugin.m_szCurrentPresetFile;
                g_plugin.m_presetHistoryPos = 0;
                g_plugin.m_presetHistoryFwdFence = 1;
                g_plugin.m_presetHistoryBackFence = 0;
                return;
            case 'p':
            case 'P':
                TogglePlaylist();
                return;
            case 'h':
            case 'H':
                if (g_plugin.m_UI_mode == UI_MASHUP)
                {
                    if (chChar == 'h')
                    {
                        g_plugin.m_nMashPreset[g_plugin.m_nMashSlot] = g_plugin.m_nDirs + (warand() % (g_plugin.m_nPresets - g_plugin.m_nDirs));
                        g_plugin.m_nLastMashChangeFrame[g_plugin.m_nMashSlot] = g_plugin.GetFrame() + MASH_APPLY_DELAY_FRAMES; // causes instant apply
                    }
                    else
                    {
                        for (int mash = 0; mash < MASH_SLOTS; mash++)
                        {
                            g_plugin.m_nMashPreset[mash] = g_plugin.m_nDirs + (warand() % (g_plugin.m_nPresets - g_plugin.m_nDirs));
                            g_plugin.m_nLastMashChangeFrame[mash] = g_plugin.GetFrame() + MASH_APPLY_DELAY_FRAMES; // causes instant apply
                        }
                    }
                }
                else
                {
                    // Instant hard cut.
                    NextPreset(0.0f);
                    g_plugin.m_fHardCutThresh *= 2.0f; // make it a little less likely that a random hard cut follows soon
                }
                return;
            case '-':
                SetPresetRating(-1.0f);
                return;
            case '+':
                SetPresetRating(1.0f);
                return;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                {
                    int digit = chChar - '0';
                    g_plugin.m_nNumericInputNum = (g_plugin.m_nNumericInputNum * 10) + digit;
                    g_plugin.m_nNumericInputDigits++;

                    if (g_plugin.m_nNumericInputDigits >= 2)
                    {
                        if (g_plugin.m_nNumericInputMode == NUMERIC_INPUT_MODE_CUST_MSG)
                            ; //g_plugin.LaunchCustomMessage(g_plugin.m_nNumericInputNum);
                        else if (g_plugin.m_nNumericInputMode == NUMERIC_INPUT_MODE_SPRITE)
                            ; //g_plugin.LaunchSprite(g_plugin.m_nNumericInputNum, -1);
                        else if (g_plugin.m_nNumericInputMode == NUMERIC_INPUT_MODE_SPRITE_KILL)
                        {
                            for (int x = 0; x < NUM_TEX; x++)
                                if (g_plugin.m_texmgr.m_tex[x].nUserData == g_plugin.m_nNumericInputNum)
                                    g_plugin.m_texmgr.KillTex(x);
                        }

                        g_plugin.m_nNumericInputDigits = 0;
                        g_plugin.m_nNumericInputNum = 0;
                    }
                }
                return;
            case 'q':
                g_plugin.m_pState->m_fVideoEchoZoom /= 1.05f;
                return;
            case 'Q':
                g_plugin.m_pState->m_fVideoEchoZoom *= 1.05f;
                return;
            case 'w':
                g_plugin.m_pState->m_nWaveMode++;
                if (g_plugin.m_pState->m_nWaveMode >= NUM_WAVES)
                    g_plugin.m_pState->m_nWaveMode = 0;
                return;
            case 'W':
                g_plugin.m_pState->m_nWaveMode--;
                if (g_plugin.m_pState->m_nWaveMode < 0)
                    g_plugin.m_pState->m_nWaveMode = NUM_WAVES - 1;
                return;
            case 'e':
                g_plugin.m_pState->m_fWaveAlpha -= 0.1f;
                if (g_plugin.m_pState->m_fWaveAlpha.eval(-1) < 0.0f)
                    g_plugin.m_pState->m_fWaveAlpha = 0.0f;
                return;
            case 'E':
                g_plugin.m_pState->m_fWaveAlpha += 0.1f;
                //if (g_plugin.m_pState->m_fWaveAlpha.eval(-1) > 1.0f)
                //    g_plugin.m_pState->m_fWaveAlpha = 1.0f;
                return;
            case 'i':
                g_plugin.m_pState->m_fZoom += 0.01f; //g_plugin.m_pState->m_fWarpAnimSpeed /= 1.1f;
                return;
            case 'I':
                g_plugin.m_pState->m_fZoom -= 0.01f; //g_plugin.m_pState->m_fWarpAnimSpeed *= 1.1f;
                return;
            case 'n':
            case 'N':
                g_plugin.m_bShowDebugInfo = !g_plugin.m_bShowDebugInfo;
                return;
            case 't':
            case 'T':
                //g_plugin.LaunchSongTitleAnim();
                return;
            case 'o':
                g_plugin.m_pState->m_fWarpAmount /= 1.1f;
                return;
            case 'O':
                g_plugin.m_pState->m_fWarpAmount *= 1.1f;
                return;
            case '!':
                // Randomize warp shader.
                {
                    bool bWarpLock = g_plugin.m_bWarpShaderLock;
                    wchar_t szOldPreset[MAX_PATH];
                    wcscpy_s(szOldPreset, g_plugin.m_szCurrentPresetFile);
                    g_plugin.m_bWarpShaderLock = false;
                    g_plugin.LoadRandomPreset(0.0f);
                    g_plugin.m_bWarpShaderLock = true;
                    g_plugin.LoadPreset(szOldPreset, 0.0f);
                    g_plugin.m_bWarpShaderLock = bWarpLock;
                }
                break;
            case '@':
                // Randomize comp shader.
                {
                    bool bCompLock = g_plugin.m_bCompShaderLock;
                    wchar_t szOldPreset[MAX_PATH];
                    wcscpy_s(szOldPreset, g_plugin.m_szCurrentPresetFile);
                    g_plugin.m_bCompShaderLock = false;
                    g_plugin.LoadRandomPreset(0.0f);
                    g_plugin.m_bCompShaderLock = true;
                    g_plugin.LoadPreset(szOldPreset, 0.0f);
                    g_plugin.m_bCompShaderLock = bCompLock;
                }
                return;
            case 'a':
            case 'A':
                // Load a random preset, a random warp shader and a random comp shader.
                // Not quite as extreme as a mash-up.
                {
                    bool bCompLock = g_plugin.m_bCompShaderLock;
                    bool bWarpLock = g_plugin.m_bWarpShaderLock;
                    g_plugin.m_bCompShaderLock = false;
                    g_plugin.m_bWarpShaderLock = false;
                    g_plugin.LoadRandomPreset(0.0f);
                    g_plugin.m_bCompShaderLock = true;
                    g_plugin.m_bWarpShaderLock = false;
                    g_plugin.LoadRandomPreset(0.0f);
                    g_plugin.m_bCompShaderLock = false;
                    g_plugin.m_bWarpShaderLock = true;
                    g_plugin.LoadRandomPreset(0.0f);
                    g_plugin.m_bCompShaderLock = bCompLock;
                    g_plugin.m_bWarpShaderLock = bWarpLock;
                }
                return;
            case 'd':
            case 'D':
                if (!g_plugin.m_bCompShaderLock && !g_plugin.m_bWarpShaderLock)
                {
                    g_plugin.m_bCompShaderLock = true;
                    g_plugin.m_bWarpShaderLock = false;
                    LoadString(core_api::get_my_instance(), IDS_COMPSHADER_LOCKED, buf, 256);
                    g_plugin.AddError(buf, 3.0f, ERR_NOTIFY, false);
                }
                else if (g_plugin.m_bCompShaderLock && !g_plugin.m_bWarpShaderLock)
                {
                    g_plugin.m_bCompShaderLock = false;
                    g_plugin.m_bWarpShaderLock = true;
                    LoadString(core_api::get_my_instance(), IDS_WARPSHADER_LOCKED, buf, 256);
                    g_plugin.AddError(buf, 3.0f, ERR_NOTIFY, false);
                }
                else if (!g_plugin.m_bCompShaderLock && g_plugin.m_bWarpShaderLock)
                {
                    g_plugin.m_bCompShaderLock = true;
                    g_plugin.m_bWarpShaderLock = true;
                    LoadString(core_api::get_my_instance(), IDS_ALLSHADERS_LOCKED, buf, 256);
                    g_plugin.AddError(buf, 3.0f, ERR_NOTIFY, false);
                }
                else
                {
                    g_plugin.m_bCompShaderLock = false;
                    g_plugin.m_bWarpShaderLock = false;
                    LoadString(core_api::get_my_instance(), IDS_ALLSHADERS_UNLOCKED, buf, 256);
                    g_plugin.AddError(buf, 3.0f, ERR_NOTIFY, false);
                }
                return;
            /*
            case 'a':
                g_plugin.m_pState->m_fVideoEchoAlpha -= 0.1f;
                if (g_plugin.m_pState->m_fVideoEchoAlpha.eval(-1) < 0)
                    m_pState->m_fVideoEchoAlpha = 0.0f;
                return;
            case 'A':
                g_plugin.m_pState->m_fVideoEchoAlpha += 0.1f;
                if (g_plugin.m_pState->m_fVideoEchoAlpha.eval(-1) > 1.0f)
                    g_plugin.m_pState->m_fVideoEchoAlpha = 1.0f;
                return;
            case 'd':
                g_plugin.m_pState->m_fDecay += 0.01f;
                if (g_plugin.m_pState->m_fDecay.eval(-1) > 1.0f)
                    g_plugin.m_pState->m_fDecay = 1.0f;
                return;
            case 'D':
                g_plugin.m_pState->m_fDecay -= 0.01f;
                if (g_plugin.m_pState->m_fDecay.eval(-1) < 0.9f)
                    g_plugin.m_pState->m_fDecay = 0.9f;
                return;
            */
            case 'f':
            case 'F':
                g_plugin.m_pState->m_nVideoEchoOrientation = (g_plugin.m_pState->m_nVideoEchoOrientation + 1) % 4;
                return;
            case 'g':
                g_plugin.m_pState->m_fGammaAdj -= 0.1f;
                if (g_plugin.m_pState->m_fGammaAdj.eval(-1) < 0.0f)
                    g_plugin.m_pState->m_fGammaAdj = 0.0f;
                return;
            case 'G':
                g_plugin.m_pState->m_fGammaAdj += 0.1f;
                //if (g_plugin.m_pState->m_fGammaAdj > 1.0f)
                //    m_pState->m_fGammaAdj = 1.0f;
                return;
            case 'j':
                g_plugin.m_pState->m_fWaveScale *= 0.9f;
                return;
            case 'J':
                g_plugin.m_pState->m_fWaveScale /= 0.9f;
                return;
            case 'k':
            case 'K':
                {
                    USHORT mask = 1 << (sizeof(SHORT) * 8 - 1); // Get the highest-order bit
                    bool bShiftHeldDown = (GetKeyState(VK_SHIFT) & mask) != 0;

                    if (bShiftHeldDown)
                        g_plugin.m_nNumericInputMode = NUMERIC_INPUT_MODE_SPRITE_KILL;
                    else
                        g_plugin.m_nNumericInputMode = NUMERIC_INPUT_MODE_SPRITE;
                    g_plugin.m_nNumericInputNum = 0;
                    g_plugin.m_nNumericInputDigits = 0;
                }
                return;
            case '[':
                g_plugin.m_pState->m_fXPush -= 0.005f;
                return;
            case ']':
                g_plugin.m_pState->m_fXPush += 0.005f;
                return;
            case '{':
                g_plugin.m_pState->m_fYPush -= 0.005f;
                return;
            case '}':
                g_plugin.m_pState->m_fYPush += 0.005f;
                return;
            case '<':
                g_plugin.m_pState->m_fRot += 0.02f;
                return;
            case '>':
                g_plugin.m_pState->m_fRot -= 0.02f;
                return;
            case 's':
            case 'S':
                // Save preset.
                if (g_plugin.m_UI_mode == UI_REGULAR)
                {
                    //g_plugin.m_bPresetLockedByCode = true;
                    g_plugin.m_UI_mode = UI_SAVEAS;

                    // Enter WaitString mode.
                    g_plugin.m_waitstring.bActive = true;
                    g_plugin.m_waitstring.bFilterBadChars = true;
                    g_plugin.m_waitstring.bDisplayAsCode = false;
                    g_plugin.m_waitstring.nSelAnchorPos = -1;
                    g_plugin.m_waitstring.nMaxLen = std::min(sizeof(g_plugin.m_waitstring.szText) - 1, static_cast<size_t>(MAX_PATH - lstrlenW(g_plugin.GetPresetDir()) - 6)); // 6 for the extension + null char. Set this because Win32 LoadFile, MoveFile, etc. barf if the path+filename+ext are > MAX_PATH chars.
                    wcscpy_s(g_plugin.m_waitstring.szText, g_plugin.m_pState->m_szDesc); // initial string is the filename, minus the extension
                    LoadString(core_api::get_my_instance(), IDS_SAVE_AS, g_plugin.m_waitstring.szPrompt, 512);
                    g_plugin.m_waitstring.szToolTip[0] = L'\0';
                    g_plugin.m_waitstring.nCursorPos = wcsnlen_s(g_plugin.m_waitstring.szText, 48000); // set the starting edit position
                }
                else
                {
                    const char* szMode = ToggleShuffle(chChar == 'u' || chChar == 'U');
                    swprintf_s(buf, TEXT("Playback Order: %hs"), szMode);
                    g_plugin.AddError(buf, 3.0f, ERR_NOTIFY, false);
                }
                return;
            case 'l':
            case 'L':
                // Load preset.
                if (g_plugin.m_UI_mode == UI_LOAD)
                {
                    g_plugin.m_UI_mode = UI_REGULAR;
                    return;
                }
                else if (g_plugin.m_UI_mode == UI_REGULAR || g_plugin.m_UI_mode == UI_MENU)
                {
                    g_plugin.UpdatePresetList(); // make sure list is completely ready
                    g_plugin.m_UI_mode = UI_LOAD;
                    g_plugin.m_bUserPagedUp = false;
                    g_plugin.m_bUserPagedDown = false;
                    return;
                }
                break;
            case 'm':
            case 'M':
                if (g_plugin.m_UI_mode == UI_MENU)
                    g_plugin.m_UI_mode = UI_REGULAR;
                else if (g_plugin.m_UI_mode == UI_REGULAR || g_plugin.m_UI_mode == UI_LOAD)
                    g_plugin.m_UI_mode = UI_MENU;
                return;
            case '*':
                g_plugin.m_nNumericInputDigits = 0;
                g_plugin.m_nNumericInputNum = 0;
                return;
            case 'y':
            case 'Y':
                g_plugin.m_nNumericInputMode = NUMERIC_INPUT_MODE_CUST_MSG;
                g_plugin.m_nNumericInputNum = 0;
                g_plugin.m_nNumericInputDigits = 0;
                return;
        }
    }
}

void milk2_ui_element::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    MILK2_CONSOLE_LOG("OnKeyDown ", GetWnd())
    USHORT mask = 1 << (sizeof(SHORT) * 8 - 1); // Get the highest-order bit
    bool bShiftHeldDown = (GetKeyState(VK_SHIFT) & mask) != 0; // or "< 0" without masking
    bool bCtrlHeldDown = (GetKeyState(VK_CONTROL) & mask) != 0; // or "< 0" without masking
    //bool bAltHeldDown: most keys come in under WM_SYSKEYDOWN when ALT is depressed.

    if (g_plugin.m_show_playlist)
    {
        auto api = playlist_manager::get();
        switch (nChar)
        {
            case VK_ESCAPE:
                if (g_plugin.m_show_playlist)
                    TogglePlaylist();
                return;
            case VK_UP:
                if (GetKeyState(VK_SHIFT) & mask)
                    g_plugin.m_playlist_pos -= 10 * nRepCnt;
                else
                    g_plugin.m_playlist_pos -= nRepCnt;
                SetSelectionSingle(static_cast<size_t>(g_plugin.m_playlist_pos));
                return;
            case VK_DOWN:
                if (GetKeyState(VK_SHIFT) & mask)
                    g_plugin.m_playlist_pos += 10 * nRepCnt;
                else
                    g_plugin.m_playlist_pos += nRepCnt;
                SetSelectionSingle(static_cast<size_t>(g_plugin.m_playlist_pos));
                return;
            case VK_HOME:
                g_plugin.m_playlist_pos = 0;
                SetSelectionSingle(static_cast<size_t>(g_plugin.m_playlist_pos));
                return;
            case VK_END:
                {
                    const size_t count = api->activeplaylist_get_item_count();
                    g_plugin.m_playlist_pos = count - 1;
                    SetSelectionSingle(static_cast<size_t>(g_plugin.m_playlist_pos));
                }
                return;
            case VK_PRIOR:
                if (GetKeyState(VK_SHIFT) & mask)
                    g_plugin.m_playlist_pageups += 10;
                else
                    ++g_plugin.m_playlist_pageups;
                return;
            case VK_NEXT:
                if (GetKeyState(VK_SHIFT) & mask)
                    g_plugin.m_playlist_pageups -= 10;
                else
                    --g_plugin.m_playlist_pageups;
                return;
            case VK_RETURN:
                {
                    size_t active = api->get_active_playlist();
                    if (active == pfc::infinite_size)
                        return;
                    SetSelectionSingle(static_cast<size_t>(g_plugin.m_playlist_pos));
                    api->set_playing_playlist(active);
                    m_playback_control->start(playback_control::track_command_settrack);
                }
                return;
        }
    }
    else
    {
        switch (nChar)
        {
            case VK_ESCAPE:
                if (g_plugin.m_show_help)
                    ToggleHelp();
                else if (s_fullscreen)
                    ToggleFullScreen();
                return;
            case VK_SPACE:
                //if (g_plugin.m_UI_mode == UI_LOAD)
                //    goto HitEnterFromLoadMenu;
                if (!IsPresetLock())
                    RandomPreset();
                return;
            case VK_BACK:
                PrevPreset(0.0f);
                g_plugin.m_fHardCutThresh *= 2.0f; // make it a little less likely that a random hard cut follows soon.
                return;
            case VK_UP:
                m_playback_control->volume_up();
                return;
            case VK_DOWN:
                m_playback_control->volume_down();
                return;
            case VK_LEFT:
            case VK_RIGHT:
                Seek(nRepCnt, bShiftHeldDown, nChar == VK_LEFT ? -5.0 : 5.0);
                return;
            case VK_SUBTRACT:
                SetPresetRating(-1.0f);
                return;
            case VK_ADD:
                SetPresetRating(1.0f);
                return;
            case VK_F1:
                //g_plugin.m_show_press_f1_msg = 0u;
                ToggleHelp();
                return;
            case VK_F2:
                ToggleSongTitle();
                return;
            case VK_F3:
                ToggleSongLength();
                return;
            case VK_F4:
                TogglePresetInfo();
                return;
            case VK_F5:
                ToggleFps();
                return;
            case VK_F6:
                ToggleRating();
                return;
            case VK_F7:
                if (g_plugin.m_nNumericInputMode == NUMERIC_INPUT_MODE_CUST_MSG)
                    //g_plugin.ReadCustomMessages(); // re-read custom messages
                return;
            case VK_F8:
                return;
            case VK_F9:
                ToggleShaderHelp();
                return;
            case VK_SCROLL:
                {
                    SHORT lock = GetKeyState(VK_SCROLL) & 0x0001;
                    LockPreset(static_cast<bool>(lock));
                }
                return;
            case 'T':
            case 'Y':
                if (bCtrlHeldDown)
                {
                    // Stop display of custom message or song title.
                    g_plugin.m_supertext.fStartTime = -1.0f;
                }
                return;
            case 'K':
                if (bCtrlHeldDown)
                {
                    // Kill all sprites.
                    for (int x = 0; x < NUM_TEX; x++)
                        if (g_plugin.m_texmgr.m_tex[x].pSurface)
                            g_plugin.m_texmgr.KillTex(x);
                }
                return;
        }
    }
}

void milk2_ui_element::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    MILK2_CONSOLE_LOG("OnSysKeyDown ", GetWnd())
    // Bit 29: The context code. The value is 1 if the ALT key is down while the
    //         key is pressed; it is 0 if the WM_SYSKEYDOWN message is posted to
    //         the active window because no window has the keyboard focus.
    // Bit 30: The previous key state. The value is 1 if the key is down before
    //         the message is sent, or it is 0 if the key is up.
    if ((nChar == VK_RETURN && (nFlags & 0x6000) == 0x2000) && g_plugin.GetFrame() > 0)
    {
        ToggleFullScreen();
        return;
    }
}

void milk2_ui_element::OnSysChar(TCHAR chChar, UINT nRepCnt, UINT nFlags)
{
    if (chChar == 'k' || chChar == 'K')
    {
        g_plugin.OnAltK();
        return;
    }
    if ((chChar == 'd' || chChar == 'D') && g_plugin.GetFrame() > 0)
    {
        //g_plugin.ToggleDesktop();
        return;
    }
}

UINT milk2_ui_element::OnGetDlgCode(LPMSG lpMsg)
{
    MILK2_CONSOLE_LOG("OnGetDlgCode ", GetWnd())
    return WM_GETDLGCODE;
}

void milk2_ui_element::OnContextMenu(CWindow wnd, CPoint point)
{
    MILK2_CONSOLE_LOG("OnContextMenu ", point.x, ", ", point.y, ", ", GetWnd());
    if (m_callback->is_edit_mode_enabled())
    {
        SetMsgHandled(FALSE);
        return;
    }
    //_context_menu = make_context_menu();

    // A (-1,-1) point is due to context menu key rather than right click.
    // `GetContextMenuPoint()` fixes that, returning a proper point at which the menu should be shown.
    //point = m_list.GetContextMenuPoint(point);
    CMenu menu;
    WIN32_OP_D(menu.CreatePopupMenu());
    //BOOL b = TRUE;
    //CMenu original;
    //b = menu.LoadMenu(IDR_WINDOWED_CONTEXT_MENU);
    //menu.AppendMenu(MF_STRING, menu.GetSubMenu(0), TEXT("Winamp"));
    menu.AppendMenu(MF_GRAYED, IDM_CURRENT_PRESET, GetCurrentPreset().c_str());
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING, IDM_NEXT_PRESET, TEXT("Next Preset"));
    menu.AppendMenu(MF_STRING, IDM_PREVIOUS_PRESET, TEXT("Previous Preset"));
    menu.AppendMenu(MF_STRING, IDM_SHUFFLE_PRESET, TEXT("Random Preset"));
    menu.AppendMenu(MF_STRING | (IsPresetLock() ? MF_CHECKED : 0), IDM_LOCK_PRESET, TEXT("Lock Preset"));
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING | (s_config.settings.m_bEnableDownmix ? MF_CHECKED : 0), IDM_ENABLE_DOWNMIX, TEXT("Downmix Channels"));
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING | (g_plugin.m_show_help ? MF_CHECKED : 0), IDM_SHOW_HELP, TEXT("Show Help"));
    menu.AppendMenu(MF_STRING | MF_DISABLED | (g_plugin.m_show_playlist ? MF_CHECKED : 0), IDM_SHOW_PLAYLIST, TEXT("Show Playlist"));
#ifndef NO_FULLSCREEN
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING | (s_fullscreen ? MF_CHECKED : 0), IDM_TOGGLE_FULLSCREEN, TEXT("Fullscreen"));
#endif

    //auto submenu = std::make_unique<CMenu>(menu.GetSubMenu(0));
    //b = menu.RemoveMenu(0, MF_BYPOSITION);
    //return submenu;

    //int cmd = menu.GetSubMenu(0).TrackPopupMenu(TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, *this);
    int cmd = menu.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, *this);

    switch (cmd)
    {
        case IDM_TOGGLE_FULLSCREEN:
            if (g_plugin.GetFrame() > 0)
                ToggleFullScreen();
            break;
        case IDM_NEXT_PRESET:
            NextPreset();
            break;
        case IDM_PREVIOUS_PRESET:
            PrevPreset();
            break;
        case IDM_LOCK_PRESET:
            LockPreset(!IsPresetLock());
            break;
        case IDM_SHUFFLE_PRESET:
            RandomPreset();
            break;
        case IDM_ENABLE_DOWNMIX:
            s_config.settings.m_bEnableDownmix = !s_config.settings.m_bEnableDownmix;
            UpdateChannelMode();
            break;
        case IDM_SHOW_HELP:
            ToggleHelp();
            break;
        case IDM_SHOW_PLAYLIST:
            TogglePlaylist();
            break;
        case IDM_QUIT:
            break;
    }

    Invalidate();
}

LRESULT milk2_ui_element::OnImeNotify(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg != WM_IME_NOTIFY)
        return 1;
    if (wParam == IMN_CLOSESTATUSWINDOW)
    {
        if (s_fullscreen)
            ToggleFullScreen();
    }
    return 0;
}

LRESULT milk2_ui_element::OnQuit(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    MILK2_CONSOLE_LOG("OnQuit ", GetWnd())
    return 0;
}

LRESULT milk2_ui_element::OnMilk2Message(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg != WM_MILK2)
        return -1;

    auto api = playlist_manager::get();
    if (LOBYTE(wParam) == 0x21 && HIBYTE(wParam) == 0x09)
    {
        wchar_t buf[2048], title[64];
        LoadString(core_api::get_my_instance(), LOWORD(lParam), buf, 2048);
        LoadString(core_api::get_my_instance(), HIWORD(lParam), title, 64);
        MILK2_CONSOLE_LOG("milk2 -> title: ", title, ", message: ", buf)
        return 1;
    }
    else if (lParam == IPC_GETVERSION)
    {
        MILK2_CONSOLE_LOG("IPC_GETVERSION")
        //const t_core_version_data v = core_version_info_v2::get_version();
        return 0;
    }
    else if (lParam == IPC_GETVERSIONSTRING)
    {
        MILK2_CONSOLE_LOG("IPC_GETVERSIONSTRING")
        return reinterpret_cast<LRESULT>(core_version_info_v2::g_get_version_string()); // "foobar2000 v2.1.2"
        return 0;
    }
    else if (lParam == IPC_ISPLAYING)
    {
        MILK2_CONSOLE_LOG("IPC_ISPLAYING")
        if (m_playback_control->is_playing())
            return 1;
        else if (m_playback_control->is_paused())
            return 3;
        else
            return 0;
    }
    else if (lParam == IPC_SETPLAYLISTPOS)
    {
        //MILK2_CONSOLE_LOG("IPC_SETPLAYLISTPOS")
        api->activeplaylist_set_focus_item(static_cast<size_t>(wParam));
        return static_cast<LRESULT>(wParam);
    }
    else if (lParam == IPC_GETLISTLENGTH)
    {
        //MILK2_CONSOLE_LOG("IPC_GETLISTLENGTH")
        const size_t count = api->activeplaylist_get_item_count();
        return static_cast<LRESULT>(count);
    }
    else if (lParam == IPC_GETLISTPOS)
    {
        //MILK2_CONSOLE_LOG("IPC_GETLISTPOS")
        if (m_playback_control->is_playing())
        {
            size_t playing_index;
            size_t playing_playlist;
            api->get_playing_item_location(&playing_playlist, &playing_index);
            if (playing_playlist == api->get_active_playlist())
                return static_cast<LRESULT>(playing_index);
        }
        return -1;
    }
    else if (lParam == IPC_GETPLAYLISTTITLEW)
    {
        //MILK2_CONSOLE_LOG("IPC_GETPLAYLISTTITLEW")
        if (m_script.is_empty())
        {
            pfc::string8 pattern = "%title%";
            static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(m_script, pattern);
        }
        pfc::string_formatter state;
        metadb_handle_list list;
        api->activeplaylist_get_all_items(list);
        if (wParam == -1 || !(list.get_item(static_cast<size_t>(wParam)))->format_title(NULL, state, m_script, NULL))
            if (m_playback_control->is_playing())
                state = "Opening...";
            else
                state = "Stopped.";
        m_szBuffer = pfc::wideFromUTF8(state);
        return reinterpret_cast<LRESULT>(m_szBuffer.c_str());
    }
    else if (lParam == IPC_GETOUTPUTTIME)
    {
        //MILK2_CONSOLE_LOG("IPC_GETOUTPUTTIME")
        if (wParam == 0)
        {
            if (m_playback_control->is_playing())
                return static_cast<LRESULT>(m_playback_control->playback_get_position() * 1000);
        }
        else if (wParam == 1)
        {
            if (m_playback_control->is_playing())
                return static_cast<LRESULT>(m_playback_control->playback_get_length());
        }
        else if (wParam == 2)
        {
            if (m_playback_control->is_playing())
                return static_cast<LRESULT>(m_playback_control->playback_get_length() * 1000);
        }
        return -1;
    }
    else if (lParam == IPC_GETPLUGINDIRECTORYW)
    {
        m_szBuffer = s_config.settings.m_szPluginsDirPath;
        return reinterpret_cast<LRESULT>(m_szBuffer.c_str());
    }
    else if (lParam == IPC_GETINIDIRECTORYW)
    {
        m_szBuffer = s_config.settings.m_szConfigIniFile;
        return reinterpret_cast<LRESULT>(m_szBuffer.c_str());
    }

    return 1;
}

LRESULT milk2_ui_element::OnConfigurationChange(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    MILK2_CONSOLE_LOG("OnConfigurationChange ", GetWnd())
    if (uMsg != WM_CONFIG_CHANGE)
        return 1;
    switch (wParam)
    {
        case 0: // Preferences Dialog
            {
                s_config.reset();
                m_refresh_interval = static_cast<DWORD>(lround(1000.0f / s_config.settings.m_max_fps_fs));
                g_plugin.PanelSettings(&s_config.settings);
                break;
            }
        case 1: // Advanced Preferences
            {
                break;
            }
    }
    SetMsgHandled(TRUE);

    return 0;
}

// Initialize the Direct3D resources required to run.
bool milk2_ui_element::Initialize(HWND window, int width, int height)
{
    if (!s_milk2)
    {
        swprintf_s(g_plugin.m_szPluginsDirPath, L"%hs", const_cast<char*>(m_pwd.c_str()));
        swprintf_s(g_plugin.m_szConfigIniFile, L"%hs%ls", const_cast<char*>(m_pwd.c_str()), INIFILE);

        if (FALSE == g_plugin.PluginPreInitialize(window, core_api::get_my_instance()))
            return false;
        if (!g_plugin.PanelSettings(&s_config.settings))
            return false;

        if (!s_fullscreen)
            g_hWindow = get_wnd();
    }

    g_plugin.SetWinampWindow(window);

    if (!s_milk2)
    {
        if (FALSE == g_plugin.PluginInitialize(width, height))
            return false;
        s_milk2 = true;
    }
    else
    {
        g_plugin.OnWindowSwap(window, width, height);
    }

    m_milk2 = true;

    return true;
}

#pragma region Frame Update
// Executes the render.
void milk2_ui_element::Tick()
{
    m_timer.Tick([&]() { Update(m_timer); });

    Render();
}

// Updates the world.
void milk2_ui_element::Update(DX::StepTimer const& timer)
{
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
HRESULT milk2_ui_element::Render()
{
    // Do not try to render anything before the first `Update()`.
    if (m_timer.GetFrameCount() == 0)
    {
        return S_OK;
    }

    Clear();

    return g_plugin.PluginRender(waves[0], waves[1]);
}

// Clears the back buffers.
void milk2_ui_element::Clear()
{
}

void milk2_ui_element::BuildWaves()
{
    //if (!m_vis_stream.is_valid())
    //    return;

    double time;
    if (!m_vis_stream->get_absolute_time(time))
        return;

    double dt = time - m_last_time;
    m_last_time = time;

    constexpr double min_time = 1.0 / 1000.0;
    constexpr double max_time = 1.0 / 10.0;

    bool use_fake = false;

    if (dt < min_time)
    {
        dt = min_time;
        use_fake = true;
    }
    else if (dt > max_time)
        dt = max_time;

    audio_chunk_impl chunk;
    if (use_fake || !m_vis_stream->get_chunk_absolute(chunk, time - dt, dt))
    {
        //m_vis_stream->make_fake_chunk_absolute(chunk, time - dt, dt);
        for (uint32_t i = 0; i < 576U; ++i)
        {
            waves[0][i] = waves[1][i] = 0U;
        }
        return;
    }
    auto count = chunk.get_sample_count();
    auto sample_rate = chunk.get_srate();
    auto channels = chunk.get_channel_count() > 1 ? 2 : 1;
    std::vector<int16_t> audio_data(count * channels, 0);
    audio_math::convert_to_int16(chunk.get_data(), count * channels, audio_data.data(), 1.0);

    size_t top = std::min(count / channels, static_cast<size_t>(576));
    for (size_t i = 0; i < top; ++i)
    {
        waves[0][i] = static_cast<unsigned char>(audio_data[i * channels] * 255.0f);
        if (channels >= 2)
            waves[1][i] = static_cast<unsigned char>(audio_data[i * channels + 1] * 255.0f);
    }
}
#pragma endregion

#pragma region Message Handlers
void milk2_ui_element::OnActivated()
{
}

void milk2_ui_element::OnDeactivated()
{
}

void milk2_ui_element::OnSuspending()
{
}

void milk2_ui_element::OnResuming()
{
    m_timer.ResetElapsedTime();
}
#pragma endregion

// Properties
void milk2_ui_element::GetDefaultSize(int& width, int& height) const noexcept
{
    width = 640;
    height = 480;
}

void milk2_ui_element::SetPwd(std::string pwd) noexcept
{
    m_pwd.assign(pwd);
}

void milk2_ui_element::ToggleFullScreen()
{
#ifndef NO_FULLSCREEN
    MILK2_CONSOLE_LOG("ToggleFullScreen ", GetWnd())
    if (m_milk2)
    {
        if (!s_fullscreen)
        {
            g_hWindow = get_wnd();
            m_milk2 = false;
        }
        s_fullscreen = !s_fullscreen;
        s_in_toggle = true;
        static_api_ptr_t<ui_element_common_methods_v2>()->toggle_fullscreen(g_get_guid(), core_api::get_main_window());
#if 0
        if (s_fullscreen)
        {
            SetWindowLongPtr(GWL_STYLE, WS_OVERLAPPEDWINDOW);
            SetWindowLongPtr(GWL_EXSTYLE, 0);

            int width = 800;
            int height = 600;

            ShowWindow(SW_SHOWNORMAL);

            SetWindowPos(HWND_TOP, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }
        else
        {
            SetWindowLongPtr(GWL_STYLE, WS_POPUP);
            SetWindowLongPtr(GWL_EXSTYLE, WS_EX_TOPMOST);

            SetWindowPos(HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

            ShowWindow(SW_SHOWMAXIMIZED);
        }
#endif
        MILK2_CONSOLE_LOG("ToggleFullScreen2 ", GetWnd())
    }
#endif
}

void milk2_ui_element::ToggleHelp()
{
    g_plugin.ToggleHelp();
}

void milk2_ui_element::TogglePlaylist()
{
    g_plugin.TogglePlaylist();
}

void milk2_ui_element::ToggleSongTitle()
{
    s_config.settings.m_bShowSongTitle = !s_config.settings.m_bShowSongTitle;
    g_plugin.m_bShowSongTitle = s_config.settings.m_bShowSongTitle;
}

void milk2_ui_element::ToggleSongLength()
{
    if (s_config.settings.m_bShowSongTime && s_config.settings.m_bShowSongLen)
    {
        s_config.settings.m_bShowSongTime = false;
        s_config.settings.m_bShowSongLen = false;
    }
    else if (s_config.settings.m_bShowSongTime && !s_config.settings.m_bShowSongLen)
    {
        s_config.settings.m_bShowSongLen = true;
    }
    else
    {
        s_config.settings.m_bShowSongTime = true;
        s_config.settings.m_bShowSongLen = false;
    }
    g_plugin.m_bShowSongTime = s_config.settings.m_bShowSongTime;
    g_plugin.m_bShowSongLen = s_config.settings.m_bShowSongLen;
}

void milk2_ui_element::TogglePresetInfo()
{
    s_config.settings.m_bShowPresetInfo = !s_config.settings.m_bShowPresetInfo;
    g_plugin.m_bShowPresetInfo = s_config.settings.m_bShowPresetInfo;
}

void milk2_ui_element::ToggleFps()
{
    s_config.settings.m_bShowFPS = !s_config.settings.m_bShowFPS;
    g_plugin.m_bShowFPS = s_config.settings.m_bShowFPS;
}

void milk2_ui_element::ToggleRating()
{
    s_config.settings.m_bShowRating = !s_config.settings.m_bShowRating;
    g_plugin.m_bShowRating = s_config.settings.m_bShowRating;
}

void milk2_ui_element::ToggleShaderHelp()
{
    s_config.settings.m_bShowShaderHelp = !s_config.settings.m_bShowShaderHelp;
    g_plugin.m_bShowShaderHelp = s_config.settings.m_bShowShaderHelp;
}

const char* milk2_ui_element::ToggleShuffle(bool forward = true)
{
    auto api = playlist_manager::get();
    size_t nModeCount = api->playback_order_get_count();
    size_t nCurrentMode = api->playback_order_get_active();
    size_t nNewMode = (forward ? ++nCurrentMode % nModeCount : (nCurrentMode == 0 ? nModeCount - 1 : --nCurrentMode));
    api->playback_order_set_active(nNewMode);
    return api->playback_order_get_name(nNewMode);
}

void milk2_ui_element::NextPreset(float fBlendTime)
{
    g_plugin.NextPreset(fBlendTime);
}

void milk2_ui_element::PrevPreset(float fBlendTime)
{
    g_plugin.PrevPreset(fBlendTime);
}

bool milk2_ui_element::LoadPreset(int select)
{
#if 0
    g_plugin.m_nCurrentPreset = select + g_plugin.m_nDirs;

    wchar_t szFile[MAX_PATH] = {0};
    wcscpy_s(szFile, g_plugin.m_szPresetDir); // Note: m_szPresetDir always ends with '\'
    wcscat_s(szFile, g_plugin.m_presets[g_plugin.m_nCurrentPreset].szFilename.c_str());

    g_plugin.LoadPreset(szFile, 1.0f);
#else
    UNREFERENCED_PARAMETER(select);
#endif
    return true;
}

std::wstring milk2_ui_element::GetCurrentPreset()
{
    return g_plugin.m_presets[g_plugin.m_nCurrentPreset].szFilename;
}

void milk2_ui_element::LockPreset(bool lockUnlock)
{
    g_plugin.m_bPresetLockedByUser = lockUnlock;
}

bool milk2_ui_element::IsPresetLock()
{
    return g_plugin.m_bPresetLockedByUser || g_plugin.m_bPresetLockedByCode;
}

void milk2_ui_element::RandomPreset(float fBlendTime)
{
    g_plugin.LoadRandomPreset(fBlendTime);
}

void milk2_ui_element::SetPresetRating(float inc_dec)
{
    g_plugin.SetCurrentPresetRating(g_plugin.m_pState->m_fRating + inc_dec);
}

// clang-format off
void milk2_ui_element::UpdateChannelMode()
{
    if (m_vis_stream.is_valid())
    {
        m_vis_stream->set_channel_mode(s_config.settings.m_bEnableDownmix ? visualisation_stream_v3::channel_mode_mono : visualisation_stream_v3::channel_mode_default);
    }
}

void milk2_ui_element::UpdateTrack()
{
    if (m_script.is_empty())
    {
        pfc::string8 pattern = "%title%"; //"%codec% | %bitrate% kbps | %__bitspersample% bit$ifequal(%__bitspersample%,1,,s) | %samplerate% Hz | $caps(%channels%) | %playback_time%[ / %length%]$if(%ispaused%, | paused,)";
        static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(m_script, pattern);
    }

    if (m_playback_control->playback_format_title(NULL, m_state, m_script, NULL, playback_control::display_level_all))
    {
        // Succeeded already.
    }
    else if (m_playback_control->is_playing())
    {
        // Starting playback but not done opening the first track yet.
        m_state = "Opening...";
    }
    else
    {
        m_state = "Stopped.";
    }
}
// clang-format on

void milk2_ui_element::UpdatePlaylist()
{
    auto api = playlist_manager::get();
    size_t total = api->activeplaylist_get_item_count();
    for (size_t i = 0; i < total; ++i)
    {
        if (api->activeplaylist_is_item_selected(i))
        {
            g_plugin.m_playlist_pos = i;
        }
    }
    g_plugin.m_playlist_top_idx = -1;
}

void milk2_ui_element::SetSelectionSingle(size_t idx)
{
    SetSelectionSingle(idx, false, true, true);
}

void milk2_ui_element::SetSelectionSingle(size_t idx, bool toggle, bool focus, bool single_only)
{
    auto api = playlist_manager::get();
    size_t total = api->activeplaylist_get_item_count();
    size_t idx_focus = api->activeplaylist_get_focus_item();

    bit_array_bittable mask(total);
    mask.set(idx, toggle ? !api->activeplaylist_is_item_selected(idx) : true);

    if (single_only || toggle || !api->activeplaylist_is_item_selected(idx))
        api->activeplaylist_set_selection(single_only ? (pfc::bit_array&)pfc::bit_array_true() : (pfc::bit_array&)pfc::bit_array_one(idx), mask);
    if (focus && idx_focus != idx)
        api->activeplaylist_set_focus_item(idx);
}

// clang-format off
class ui_element_milk2 : public ui_element_impl_visualisation<milk2_ui_element> {};
// clang-format on

// Service factory publishes the class.
static service_factory_single_t<ui_element_milk2> g_ui_element_milk2_factory;

class milk2_initquit : public initquit
{
  public:
    void on_init()
    {
        if (core_api::is_quiet_mode_enabled())
            return;
    }
    void on_quit() {}
};

FB2K_SERVICE_FACTORY(milk2_initquit);
} // namespace