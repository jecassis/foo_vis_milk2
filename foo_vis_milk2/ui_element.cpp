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

extern void ExitVis() noexcept;

// Indicates to hybrid graphics systems to prefer the discrete part by default
//extern "C" {
//__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
//__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
//}

// Anonymous namespace is standard practice in foobar2000 components.
// Nothing outside should have any reason to see these symbols and do not want
// funny results if another component has similarly named classes.
namespace
{
static const GUID guid_milk2 = {
    0x204b0345, 0x4df5, 0x4b47, {0xad, 0xd3, 0x98, 0x9f, 0x81, 0x1b, 0xd9, 0xa5}
}; // {204B0345-4DF5-4B47-ADD3-989F811BD9A5}

static const GUID VisMilk2LangGUID = {
    0xc5d175f1, 0xe4e4, 0x47ee, {0xb8, 0x5c, 0x4e, 0xdc, 0x6b, 0x2, 0x6a, 0x35}
}; // {C5D175F1-E4E4-47EE-B85C-4EDC6B026A35}

static bool s_fullscreen = false;
static bool s_milk2 = false;
static ULONGLONG s_count = 0ull;
static constexpr ULONGLONG DebugLimit = 50ull;

class milk2_ui_element : public ui_element_instance, public CWindowImpl<milk2_ui_element>
{
  public:
    DECLARE_WND_CLASS_EX(CLASSNAME, CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS, (-1));

    void initialize_window(HWND parent)
    {
#ifdef _DEBUG
        WCHAR szParent[11], szWnd[11];
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
        //MSG_WM_DISPLAYCHANGE(OnDisplayChange)
        MSG_WM_SIZE(OnSize)
        MSG_WM_MOVE(OnMove)
        MSG_WM_ENTERSIZEMOVE(OnEnterSizeMove)
        MSG_WM_EXITSIZEMOVE(OnExitSizeMove)
        MSG_WM_ERASEBKGND(OnEraseBkgnd)
        MSG_WM_COPYDATA(OnCopyData)
        MSG_WM_ACTIVATEAPP(OnActivateApp)
        MSG_WM_KEYDOWN(OnKeyDown)
        MSG_WM_SYSKEYDOWN(OnSysKeyDown)
        MSG_WM_CONTEXTMENU(OnContextMenu)
        MSG_WM_LBUTTONDBLCLK(OnLButtonDblClk)
        MSG_WM_POWERBROADCAST(OnPowerBroadcast)
        //MSG_WM_DPICHANGED(OnDPIChanged)
        MESSAGE_HANDLER(WM_MILK2, OnMilk2Message)
        MESSAGE_HANDLER_EX(WM_CONFIG_CHANGE, OnConfigurationChange)
    END_MSG_MAP()
    // clang-format on

    milk2_ui_element(ui_element_config::ptr config, ui_element_instance_callback_ptr p_callback);
    HWND get_wnd() { return *this; }
    void set_configuration(ui_element_config::ptr config);
    ui_element_config::ptr get_configuration();
    static GUID g_get_guid() { return guid_milk2; }
    static GUID g_get_subclass() { return ui_element_subclass_playback_visualisation; }
    static void g_get_name(pfc::string_base& out) { out = "MilkDrop"; }
    static ui_element_config::ptr g_get_default_configuration(); //{ return ui_element_config::g_create_empty(g_get_guid()); }
    static const char* g_get_description() { return "MilkDrop 2 Visualization using DirectX 11."; }

    void notify(const GUID& p_what, t_size p_param1, const void* p_param2, t_size p_param2size);

  private:
    int OnCreate(LPCREATESTRUCT cs);
    void OnClose();
    void OnDestroy();
    void OnTimer(UINT_PTR nIDEvent);
    void OnPaint(CDCHandle dc);
    void OnDisplayChange(UINT uBitsPerPixel, CSize sizeScreen);
    void OnSize(UINT nType, CSize size);
    void OnMove(CPoint ptPos);
    void OnEnterSizeMove();
    void OnExitSizeMove();
    BOOL OnEraseBkgnd(CDCHandle dc);
    BOOL OnCopyData(CWindow wnd, PCOPYDATASTRUCT pCopyDataStruct);
    void OnActivateApp(BOOL bActive, DWORD dwThreadID);
    BOOL OnPowerBroadcast(DWORD dwPowerEvent, DWORD_PTR dwData);
    void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    void OnContextMenu(CWindow wnd, CPoint point);
    void OnLButtonDblClk(UINT nFlags, CPoint point);
    LRESULT OnMilk2Message(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& handled);
    LRESULT OnConfigurationChange(UINT uMsg, WPARAM wParam, LPARAM lParam);
    PWCHAR GetWnd() { swprintf_s(m_szWnd, TEXT("0x%p"), get_wnd()); return m_szWnd; }

    milk2_config m_config;
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
        //IDM_DURATION_5,
        //IDM_DURATION_10,
        //IDM_DURATION_20,
        //IDM_DURATION_30,
        //IDM_DURATION_45,
        //IDM_DURATION_60,
        //IDM_HW_RENDERING_ENABLED,
        //IDM_RESAMPLE_ENABLED,
        //IDM_WINDOW_DURATION_50,
        //IDM_WINDOW_DURATION_100,
        //IDM_ZOOM_50,
        //IDM_ZOOM_75,
        //IDM_ZOOM_100,
        //IDM_REFRESH_RATE_LIMIT_20,
        //IDM_REFRESH_RATE_LIMIT_60
    };

    // Initialization and management
    bool Initialize(HWND window, int width, int height);

    // Basic visualization loop
    void Tick();

    void PrevPreset();
    void NextPreset();
    bool LoadPreset(int select);
    void RandomPreset();
    void LockPreset(bool lockUnlock);
    bool IsPresetLock();
    std::wstring GetCurrentPreset();
    bool GetPresets(std::vector<std::string>& presets);
    int GetActivePreset() const;
    char* WideToUTF8(const wchar_t* WFilename);

    // Window Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnDisplayChange();

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;
    void SetPwd(std::string pwd) noexcept;
    void UpdateChannelMode();
    void ToggleFullScreen();

    void Update(DX::StepTimer const& timer);
    HRESULT Render();
    void Clear();
    void BuildWaves();

    // MilkDrop status
    bool m_milk2;
    WCHAR m_szWnd[11];

    // Device resources
    std::unique_ptr<DXContext> g_game;

    // Rendering loop timer
    DX::StepTimer m_timer;

    // Component paths
    std::string m_pwd;

    unsigned char waves[2][576];

  protected:
    const ui_element_instance_callback_ptr m_callback;
};

milk2_ui_element::milk2_ui_element(ui_element_config::ptr config, ui_element_instance_callback_ptr p_callback) :
    m_callback(p_callback)
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
    ui_element_config_builder builder;
    try
    {
        milk2_config config;
        config.build(builder);
    }
    catch (exception_io& exc)
    {
        FB2K_console_print(core_api::get_my_file_name(), ": Exception while building default configuration data - ", exc);
    }
    return builder.finish(g_get_guid()); //return ui_element_config::g_create_empty(g_get_guid());
}

void milk2_ui_element::set_configuration(ui_element_config::ptr p_data)
{
    //LPVOID dataptr = const_cast<LPVOID>(p_data->get_data());
    //if (dataptr && p_data->get_data_size() > 4 && static_cast<DWORD*>(dataptr)[0] == ('M' << 24 | 'I' << 16 | 'L' << 8 | 'K'))
    //{
    //    m_config = p_data;
    //}
    //else
    //    m_config = g_get_default_configuration();
    //DWORD* in = reinterpret_cast<DWORD*>(dataptr);
    ui_element_config_parser parser(p_data);
    m_config.parse(parser);

    UpdateChannelMode();
    //UpdateRefreshRateLimit();
}

ui_element_config::ptr milk2_ui_element::get_configuration()
{
    ui_element_config_builder builder;
    m_config.build(builder);
    return builder.finish(g_get_guid());
}

void milk2_ui_element::notify(const GUID& p_what, t_size p_param1, const void* p_param2, t_size p_param2size)
{
    if (p_what == ui_element_notify_colors_changed || p_what == ui_element_notify_font_changed)
    {
        Invalidate();
    }
}

int milk2_ui_element::OnCreate(LPCREATESTRUCT cs)
{
    MILK2_CONSOLE_LOG("OnCreate ", cs->x, ", ", cs->y, ", ", GetWnd())

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
    {
        g_plugin.PluginQuit(FALSE);
        m_milk2 = false;
    }
    s_count = 0ull;
    //PostQuitMessage(0);
}

void milk2_ui_element::OnTimer(UINT_PTR nIDEvent)
{
    MILK2_CONSOLE_LOG_LIMIT("OnTimer ", GetWnd())
    KillTimer(ID_REFRESH_TIMER);
    Invalidate();
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
    //ValidateRect(NULL);

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

void milk2_ui_element::OnDisplayChange(UINT uBitsPerPixel, CSize sizeScreen)
{
    MILK2_CONSOLE_LOG("OnDisplayChange ", GetWnd())
    if (m_milk2)
    {
        g_plugin.OnDisplayChange();
    }
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
    Tick();
#endif

    return TRUE;
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
                MILK2_CONSOLE_LOG(WideToUTF8(lpszString))
                break;
            }
    }

    return TRUE;
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
#ifdef _DEBUG
    ToggleFullScreen();
#endif
}

void milk2_ui_element::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    MILK2_CONSOLE_LOG("OnSysKeyDown ", GetWnd())
#ifdef _DEBUG
    // Bit 29: The context code. The value is 1 if the ALT key is down while the
    //         key is pressed; it is 0 if the WM_SYSKEYDOWN message is posted to
    //         the active window because no window has the keyboard focus.
    // Bit 30: The previous key state. The value is 1 if the key is down before
    //         the message is sent, or it is 0 if the key is up.
    if (nChar == VK_RETURN && (nFlags & 0x6000) == 0x2000)
    {
        ToggleFullScreen();
    }
#endif
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
    /*       b = menu.LoadMenu(IDR_WINDOWED_CONTEXT_MENU);
        menu.AppendMenu(MF_STRING, menu.GetSubMenu(0), TEXT("Winamp"));*/
    menu.AppendMenu(MF_GRAYED, IDM_CURRENT_PRESET, GetCurrentPreset().c_str());
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING, IDM_NEXT_PRESET, TEXT("Next Preset"));
    menu.AppendMenu(MF_STRING, IDM_PREVIOUS_PRESET, TEXT("Previous Preset"));
    menu.AppendMenu(MF_STRING, IDM_SHUFFLE_PRESET, TEXT("Random Preset"));
    menu.AppendMenu(MF_STRING | (m_config.m_bPresetLockedByUser ? MF_CHECKED : 0), IDM_LOCK_PRESET, TEXT("Lock Preset"));
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING | (m_config.m_bEnableDownmix ? MF_CHECKED : 0), IDM_ENABLE_DOWNMIX, TEXT("Downmix Channels"));
#ifdef _DEBUG
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING | (s_fullscreen ? MF_CHECKED : 0), IDM_TOGGLE_FULLSCREEN, TEXT("Full Screen"));
#endif

    //CMenu durationMenu;
    //durationMenu.CreatePopupMenu();
    //durationMenu.AppendMenu(MF_STRING /* | ((m_config.m_window_duration_millis == 50) ? MF_CHECKED : 0) */, IDM_WINDOW_DURATION_50, TEXT("50 ms"));
    //durationMenu.AppendMenu(MF_STRING /* | ((m_config.m_window_duration_millis == 100) ? MF_CHECKED : 0) */, IDM_WINDOW_DURATION_100, TEXT("100 ms"));

    //auto submenu = std::make_unique<CMenu>(menu.GetSubMenu(0));
    //b = menu.RemoveMenu(0, MF_BYPOSITION);
    //return submenu;

    //menu.AppendMenu(MF_STRING, durationMenu, TEXT("Window Duration"));

    //CMenu zoomMenu;
    //zoomMenu.CreatePopupMenu();
    //zoomMenu.AppendMenu(MF_STRING /* | ((m_config.m_zoom_percent == 50) ? MF_CHECKED : 0) */, IDM_ZOOM_50, TEXT("50 %"));
    //zoomMenu.AppendMenu(MF_STRING /* | ((m_config.m_zoom_percent == 75) ? MF_CHECKED : 0) */, IDM_ZOOM_75, TEXT("75 %"));
    //zoomMenu.AppendMenu(MF_STRING /* | ((m_config.m_zoom_percent == 100) ? MF_CHECKED : 0) */, IDM_ZOOM_100, TEXT("100 %"));

    //menu.AppendMenu(MF_STRING, zoomMenu, TEXT("Zoom"));

    //CMenu refreshRateLimitMenu;
    //refreshRateLimitMenu.CreatePopupMenu();
    //refreshRateLimitMenu.AppendMenu(MF_STRING /* | ((m_config.m_refresh_rate_limit_hz == 20) ? MF_CHECKED : 0) */, IDM_REFRESH_RATE_LIMIT_20, TEXT("20 Hz"));
    //refreshRateLimitMenu.AppendMenu(MF_STRING /* | ((m_config.m_refresh_rate_limit_hz == 60) ? MF_CHECKED : 0) */, IDM_REFRESH_RATE_LIMIT_60, TEXT("60 Hz"));

    //menu.AppendMenu(MF_STRING, refreshRateLimitMenu, TEXT("Refresh Rate Limit"));

    //menu.AppendMenu(MF_SEPARATOR);

    //menu.AppendMenu(MF_STRING /* | (m_config.m_resample_enabled ? MF_CHECKED : 0) */, IDM_RESAMPLE_ENABLED, TEXT("Resample For Display"));
    //menu.AppendMenu(MF_STRING /* | (m_config.m_hw_rendering_enabled ? MF_CHECKED : 0) */, IDM_HW_RENDERING_ENABLED, TEXT("Allow Hardware Rendering"));

    //menu.SetMenuDefaultItem(IDM_TOGGLE_FULLSCREEN);

    //int cmd = menu.GetSubMenu(0).TrackPopupMenu(TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, *this);
    int cmd = menu.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, *this);

    switch (cmd)
    {
        case IDM_TOGGLE_FULLSCREEN:
            ToggleFullScreen();
            break;
        case IDM_NEXT_PRESET:
            NextPreset();
            break;
        case IDM_PREVIOUS_PRESET:
            PrevPreset();
            break;
        case IDM_LOCK_PRESET:
            m_config.m_bPresetLockedByUser = !m_config.m_bPresetLockedByUser;
            LockPreset(m_config.m_bPresetLockedByUser);
            break;
        case IDM_SHUFFLE_PRESET:
            RandomPreset();
            break;
        case IDM_ENABLE_DOWNMIX:
            m_config.m_bEnableDownmix = !m_config.m_bEnableDownmix;
            UpdateChannelMode();
            break;
            //case IDM_HW_RENDERING_ENABLED:
            //    m_config.m_hw_rendering_enabled = !m_config.m_hw_rendering_enabled;
            //    DiscardDeviceResources();
            //    break;
            //case IDM_RESAMPLE_ENABLED:
            //    m_config.m_resample_enabled = !m_config.m_resample_enabled;
            //    break;
            //case IDM_WINDOW_DURATION_50:
            //    m_config.m_window_duration_millis = 50;
            //    break;
            //case IDM_WINDOW_DURATION_100:
            //    m_config.m_window_duration_millis = 100;
            //    break;
            //case IDM_ZOOM_50:
            //    m_config.m_zoom_percent = 50;
            //    break;
            //case IDM_ZOOM_75:
            //    m_config.m_zoom_percent = 75;
            //    break;
            //case IDM_ZOOM_100:
            //    m_config.m_zoom_percent = 100;
            //    break;
            //case IDM_REFRESH_RATE_LIMIT_20:
            //    m_config.m_refresh_rate_limit_hz = 20;
            //    UpdateRefreshRateLimit();
            //    break;
            //case IDM_REFRESH_RATE_LIMIT_60:
            //    m_config.m_refresh_rate_limit_hz = 60;
            //    UpdateRefreshRateLimit();
            //    break;
    }

    Invalidate();
}

LRESULT milk2_ui_element::OnMilk2Message(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& handled)
{
    handled = TRUE;
    if (uMsg != WM_MILK2)
        return 1;
    if (LOBYTE(wParam) == 0x21 && HIBYTE(wParam) == 0x09)
    {
        wchar_t buf[2048], title[64];
        LoadString(core_api::get_my_instance(), LOWORD(lParam), buf, 2048);
        LoadString(core_api::get_my_instance(), HIWORD(lParam), title, 64);
        MILK2_CONSOLE_LOG("milk2 -> title: ", title, ", message: ", buf)
    }
    //else if (lparam == IPC_SETVISWND)
    //{
    //    //SendMessage(m_hwnd_winamp, WM_WA_IPC, (WPARAM)m_hwnd, IPC_SETVISWND);
    //}
    //else if (lparam == IPC_CB_VISRANDOM)
    //{
    //    //SendMessage(GetWinampWindow(), WM_WA_IPC, (m_bPresetLockOnAtStartup ? 0 : 1) << 16, IPC_CB_VISRANDOM);
    //}
    //else if (lparam == IPC_IS_PLAYING_VIDEO)
    //{
    //    //if (m_screenmode == FULLSCREEN && SendMessage(GetWinampWindow(), WM_WA_IPC, 0, IPC_IS_PLAYING_VIDEO) > 1)
    //}
    //else if (lparam == IPC_SET_VIS_FS_FLAG)
    //{
    //    //    SendMessage(GetWinampWindow(), WM_WA_IPC, 1, IPC_SET_VIS_FS_FLAG);
    //}
    //else if (lparam == IPC_GETPLUGINDIRECTORYW)
    //{
    //    //(p = (wchar_t*)SendMessage(hWinampWnd, WM_WA_IPC, 0, IPC_GETPLUGINDIRECTORYW))(
    //}
    //else if (lparam == IPC_GETINIDIRECTORYW)
    //{
    //    //    p = (wchar_t*)SendMessage(hWinampWnd, WM_WA_IPC, 0, IPC_GETINIDIRECTORYW))
    //}
    //else if (lparam == IPC_GETPLAYLISTTITLEW)
    //{
    //    //lstrcpyn(buf, (wchar_t*)SendMessage(m_hWndWinamp, WM_USER, j, IPC_GETPLAYLISTTITLEW),
    //}
    //else if (lparam == IPC_GETLISTPOS)
    //{
    //    //lstrcpyn(szSongTitle, (wchar_t *)SendMessage(hWndWinamp, WM_WA_IPC, SendMessage(hWndWinamp, WM_WA_IPC, 0, IPC_GETLISTPOS), IPC_GETPLAYLISTTITLEW), nSize);
    //}
    //else if (lparam == IPC_GET_D3DX9)
    //{
    //    //HMODULE d3dx9 = (HMODULE)SendMessage(winamp, WM_WA_IPC, 0, IPC_GET_D3DX9);
    //}
    //else if (lparam == IPC_GET_API_SERVICE)
    //{
    //    //WASABI_API_SVC = (api_service*)SendMessage(hwndParent, WM_WA_IPC, 0, IPC_GET_API_SERVICE);
    //}
    //else if (lparam == IPC_GETDIALOGBOXPARENT)
    //{
    //    //HWND parent = (HWND)SendMessage(winamp, WM_WA_IPC, 0, IPC_GETDIALOGBOXPARENT);
    //}
    //else if (lparam == IPC_GET_RANDFUNC)
    //{
    //    //warand = (int (*)(void))SendMessage(this_mod->hwndParent, WM_WA_IPC, 0, IPC_GET_RANDFUNC);
    //}
    //else if (lparam == IPC_ISPLAYING)
    //{
    //    //#define IPC_ISPLAYING 104
    //    /* int res = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_ISPLAYING);
    //    ** This is sent to retrieve the current playback state of Winamp.
    //    ** If it returns 1, Winamp is playing.
    //    ** If it returns 3, Winamp is paused.
    //    ** If it returns 0, Winamp is not playing.
    //    */
    //}

    return 0;
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
                m_config.reset();
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

        if (!s_fullscreen)
            g_hWindow = get_wnd();
        s_milk2 = true;
    }

    DXCONTEXT_PARAMS params{};
    g_plugin.StuffParams(&params);
    g_plugin.SetWinampWindow(window);
    g_game = std::make_unique<DXContext>(window, &params, g_plugin.m_szConfigIniFile);
    SetWindowLongPtr(GWLP_USERDATA, reinterpret_cast<LONG_PTR>(g_game.get()));

    if (FALSE == g_plugin.PluginInitialize(std::move(g_game), 0, 0, width, height, static_cast<float>(width) / static_cast<float>(height)))
        return false;

    m_milk2 = true;

    return true;
}

#pragma region Frame Update
// Executes the basic game loop.
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
// Message handlers
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
    MILK2_CONSOLE_LOG("ToggleFullScreen ", GetWnd())
    if (m_milk2)
    {
        static_api_ptr_t<ui_element_common_methods_v2>()->toggle_fullscreen(g_get_guid(), core_api::get_main_window());
        if (!s_fullscreen)
        {
            g_hWindow = get_wnd();
            m_milk2 = false;
        }
        //g_plugin.ToggleFullScreen();
        //g_plugin.PluginQuit();
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
        s_fullscreen = !s_fullscreen;
        MILK2_CONSOLE_LOG("ToggleFullScreen2 ", GetWnd())
    }
}

void milk2_ui_element::NextPreset()
{
    g_plugin.NextPreset(1.0f);
}

void milk2_ui_element::PrevPreset()
{
    g_plugin.PrevPreset(1.0f);
}

bool milk2_ui_element::LoadPreset(int select)
{
    //g_plugin.m_nCurrentPreset = select + g_plugin.m_nDirs;

    //wchar_t szFile[MAX_PATH] = {0};
    //wcscpy_s(szFile, g_plugin.m_szPresetDir); // Note: m_szPresetDir always ends with '\'
    //wcscat_s(szFile, g_plugin.m_presets[g_plugin.m_nCurrentPreset].szFilename.c_str());

    //g_plugin.LoadPreset(szFile, 1.0f);

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
    return g_plugin.m_bPresetLockedByUser;
}

void milk2_ui_element::RandomPreset()
{
    g_plugin.LoadRandomPreset(1.0f);
}

bool milk2_ui_element::GetPresets(std::vector<std::string>& presets)
{
    if (!m_milk2)
        return false;

    while (!g_plugin.m_bPresetListReady)
    {
    }

    for (int i = 0; i < g_plugin.m_nPresets - g_plugin.m_nDirs; ++i)
    {
        PresetInfo& Info = g_plugin.m_presets[i + g_plugin.m_nDirs];
        presets.push_back(WideToUTF8(Info.szFilename.c_str()));
    }

    return true;
}

int milk2_ui_element::GetActivePreset() const
{
    //if (m_milk2)
    //{
    //    int CurrentPreset = g_plugin.m_nCurrentPreset;
    //    CurrentPreset -= g_plugin.m_nDirs;
    //    return CurrentPreset;
    //}

    return -1;
}

char* milk2_ui_element::WideToUTF8(const wchar_t* WFilename)
{
    int SizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &WFilename[0], -1, NULL, 0, NULL, NULL);
    char* utf8Name = new char[SizeNeeded];
    WideCharToMultiByte(CP_UTF8, 0, &WFilename[0], -1, &utf8Name[0], SizeNeeded, NULL, NULL);
    return utf8Name;
}

// clang-format off
void milk2_ui_element::UpdateChannelMode()
{
    if (m_vis_stream.is_valid())
    {
        m_vis_stream->set_channel_mode(m_config.m_bEnableDownmix ? visualisation_stream_v3::channel_mode_mono : visualisation_stream_v3::channel_mode_default);
    }
}

class ui_element_milk2 : public ui_element_impl_visualisation<milk2_ui_element> {};

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

void ExitVis() noexcept
{
    MILK2_CONSOLE_LOG("ExitVis")
    g_plugin.PluginQuit(TRUE);
    PostQuitMessage(0);
    //g_game.reset();
}
// clang-format on