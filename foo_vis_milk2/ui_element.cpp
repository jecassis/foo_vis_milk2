#include "pch.h"
#include "vis.h"

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
};

static float sin1 = 0;
static float sin2 = 0;

std::unique_ptr<Vis> g_vis;

class milk2_ui_element_instance :
    public ui_element_instance,
    public CWindowImpl<milk2_ui_element_instance>
{
  public:
    DECLARE_WND_CLASS_EX(TEXT("MilkDrop2"), CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS, (-1));

    void initialize_window(HWND parent) { WIN32_OP(Create(parent, nullptr, nullptr, 0, WS_EX_STATICEDGE) != NULL); }

    // clang-format off
    BEGIN_MSG_MAP_EX(milk2_ui_element_instance)
        MSG_WM_CREATE(OnCreate)
        MSG_WM_DESTROY(OnDestroy)
        MSG_WM_TIMER(OnTimer)
        MSG_WM_PAINT(OnPaint)
        MSG_WM_SIZE(OnSize)
        MSG_WM_MOVE(OnMove)
        MSG_WM_ENTERSIZEMOVE(OnEnterSizeMove)
        MSG_WM_EXITSIZEMOVE(OnExitSizeMove)
        MSG_WM_ERASEBKGND(OnEraseBkgnd)
        MSG_WM_ACTIVATEAPP(OnActivateApp)
        MSG_WM_SYSKEYDOWN(OnSysKeyDown)
        MSG_WM_CONTEXTMENU(OnContextMenu)
        MSG_WM_LBUTTONDBLCLK(OnLButtonDblClk)
    END_MSG_MAP()
    // clang-format on

    milk2_ui_element_instance(ui_element_config::ptr config, ui_element_instance_callback_ptr p_callback);
    HWND get_wnd() { return *this; }
    void set_configuration(ui_element_config::ptr config) { m_config = config; }
    ui_element_config::ptr get_configuration() { return m_config; }
    static GUID g_get_guid() { return guid_milk2; }
    static GUID g_get_subclass() { return ui_element_subclass_playback_visualisation; }
    static void g_get_name(pfc::string_base& out) { out = "MilkDrop"; }
    static ui_element_config::ptr g_get_default_configuration(); //{ return ui_element_config::g_create_empty(g_get_guid()); }
    static const char* g_get_description() { return "MilkDrop 2 Visualization using DirectX 11."; }

    void notify(const GUID& p_what, t_size p_param1, const void* p_param2, t_size p_param2size);

  private:
    int OnCreate(LPCREATESTRUCT cs);
    void OnDestroy();
    void OnTimer(UINT_PTR nIDEvent);
    void OnPaint(CDCHandle dc);
    void OnSize(UINT nType, CSize size);
    void OnMove(CPoint ptPos);
    void OnEnterSizeMove();
    void OnExitSizeMove();
    BOOL OnEraseBkgnd(CDCHandle dc);
    void OnActivateApp(BOOL bActive, DWORD dwThreadID);
    void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    void OnContextMenu(CWindow wnd, CPoint point);
    void OnLButtonDblClk(UINT nFlags, CPoint point) { ToggleFullScreen(); }

    void ToggleFullScreen() { static_api_ptr_t<ui_element_common_methods_v2>()->toggle_fullscreen(g_get_guid(), core_api::get_main_window()); }
    void AudioData(const float* pAudioData, size_t iAudioDataLength);
    void AddPCM();
    void UpdateChannelMode();

    ui_element_config::ptr m_config; //milk2_config m_config;
    visualisation_stream_v3::ptr m_vis_stream;

    ULONGLONG m_last_refresh;
    DWORD m_refresh_interval = 33;
    double m_last_time = 0.0;
    bool m_IsInitialized = false;

    Vis* vis;
    bool s_in_sizemove;
    bool s_in_suspend;
    bool s_minimized;

    enum
    {
        ID_REFRESH_TIMER = 1
    };

    enum
    {
        IDM_TOGGLE_FULLSCREEN = 1,
        IDM_HW_RENDERING_ENABLED,
        IDM_DOWNMIX_ENABLED,
        IDM_RESAMPLE_ENABLED,
        IDM_WINDOW_DURATION_50,
        IDM_WINDOW_DURATION_100,
        IDM_ZOOM_50,
        IDM_ZOOM_75,
        IDM_ZOOM_100,
        IDM_REFRESH_RATE_LIMIT_20,
        IDM_REFRESH_RATE_LIMIT_60
    };

  protected:
    const ui_element_instance_callback_ptr m_callback;
};

milk2_ui_element_instance::milk2_ui_element_instance(ui_element_config::ptr config, ui_element_instance_callback_ptr p_callback) :
    m_callback(p_callback),
    m_config(config)
{
    s_in_sizemove = false;
    s_in_suspend = false;
    s_minimized = false;

    //set_configuration(config);
    g_vis = std::make_unique<Vis>();
}

ui_element_config::ptr milk2_ui_element_instance::g_get_default_configuration()
{
    //ui_element_config_builder builder;
    //oscilloscope_config config;
    //config.build(builder);
    //return builder.finish(g_get_guid());
    return ui_element_config::g_create_empty(g_get_guid());
}

void milk2_ui_element_instance::notify(const GUID& p_what, t_size p_param1, const void* p_param2, t_size p_param2size)
{
    if (p_what == ui_element_notify_colors_changed || p_what == ui_element_notify_font_changed)
    {
        Invalidate();
    }
}

//void milk2_ui_element_instance::set_configuration(ui_element_config::ptr p_data) {
//    ui_element_config_parser parser(p_data);
//    oscilloscope_config config;
//    config.parse(parser);
//    m_config = config;
//
//    UpdateChannelMode();
//    UpdateRefreshRateLimit();
//}

//ui_element_config::ptr milk2_ui_element_instance::get_configuration() {
//    ui_element_config_builder builder;
//    m_config.build(builder);
//    return builder.finish(g_get_guid());
//}

int milk2_ui_element_instance::OnCreate(LPCREATESTRUCT cs)
{
#ifndef NDEBUG
    console::formatter() << core_api::get_my_file_name() << ": OnCreate " << cs->x << ", " << cs->y;
#endif
    std::string base_path = core_api::get_my_full_path();
    std::string::size_type t = base_path.rfind('\\');
    if (t != std::string::npos)
        base_path.erase(t + 1);
    g_vis->SetPwd(base_path);

    SetWindowLongPtr(GWLP_USERDATA, reinterpret_cast<LONG_PTR>(g_vis.get()));

    HRESULT hr = S_OK;
    int w, h;
    g_vis->GetDefaultSize(w, h);

    //CRect rd = {0, 0, static_cast<LONG>(w), static_cast<LONG>(h)};
    CRect r{};
    WIN32_OP_D(GetClientRect(&r))
    if (r.right - r.left > 0 && r.bottom - r.top)
    {
        w = r.right - r.left;
        h = r.bottom - r.top;
    }
    g_vis->Initialize(get_wnd(), w, h);
    //console::formatter() << core_api::get_my_file_name() << ": Could not initialize MilkDrop";
    console::formatter() << core_api::get_my_file_name() << ": OnCreate2 " << r.right << ", " << r.left << ", " << r.top << ", " << r.bottom;
    try
    {
        static_api_ptr_t<visualisation_manager> vis_manager;

        vis_manager->create_stream(m_vis_stream, 0);

        m_vis_stream->request_backlog(0.8);
        UpdateChannelMode();
    }
    catch (std::exception& exc)
    {
        console::formatter() << core_api::get_my_file_name() << ": Exception while creating visualization stream - " << exc;
    }

    return hr;
}

void milk2_ui_element_instance::OnDestroy()
{
#ifndef NDEBUG
    console::formatter() << core_api::get_my_file_name() << ": OnDestroy";
#endif
    m_vis_stream.release();

    if (m_IsInitialized)
    {
        g_vis->OnDeviceLost();
        g_vis.reset();
        m_IsInitialized = false;
    }
}

void milk2_ui_element_instance::OnTimer(UINT_PTR nIDEvent)
{
#ifndef NDEBUG
    //console::formatter() << core_api::get_my_file_name() << ": OnTimer";
#endif
    KillTimer(ID_REFRESH_TIMER);
    Invalidate();
}

void milk2_ui_element_instance::OnPaint(CDCHandle dc)
{
#ifndef NDEBUG
    //console::formatter() << core_api::get_my_file_name() << ": OnPaint";
#endif
    auto vis = reinterpret_cast<Vis*>(GetWindowLongPtr(GWLP_USERDATA));
    if (vis) //s_in_sizemove && vis
    {
        vis->Tick(); //g_vis->Tick();
    }
    else
    {
        PAINTSTRUCT ps;
        std::ignore = BeginPaint(&ps);
        EndPaint(&ps);
    }
    ValidateRect(nullptr);
    AddPCM();

    ULONGLONG now = GetTickCount64();
    if (m_vis_stream.is_valid())
    {
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

void milk2_ui_element_instance::OnMove(CPoint ptPos)
{
    auto vis = reinterpret_cast<Vis*>(GetWindowLongPtr(GWLP_USERDATA));
    if (vis)
    {
        vis->OnWindowMoved();
    }
}

void milk2_ui_element_instance::OnSize(UINT nType, CSize size)
{
#ifndef NDEBUG
    //console::formatter() << core_api::get_my_file_name() << ": OnSize " << size.cx << ", " << size.cy;
#endif
    auto vis = reinterpret_cast<Vis*>(GetWindowLongPtr(GWLP_USERDATA));
    if (nType == SIZE_MINIMIZED)
    {
        if (!s_minimized)
        {
            s_minimized = true;
            if (!s_in_suspend && vis)
                vis->OnSuspending();
            s_in_suspend = true;
        }
    }
    else if (s_minimized)
    {
        s_minimized = false;
        if (s_in_suspend && vis)
            vis->OnResuming();
        s_in_suspend = false;
    }
    else if (!s_in_sizemove && vis)
    {
        //HRESULT hr = S_OK;
        //RECT r;
        //GetClientRect(&r);
        int width = size.cx; //r.right - r.left;
        int height = size.cy; //r.bottom - r.top;

        if (!width || !height)
            return;
        if (width < 128)
            width = 128;
        if (height < 128)
            height = 128;
        vis->OnWindowSizeChanged(size.cx, size.cy);
    }
}

BOOL milk2_ui_element_instance::OnEraseBkgnd(CDCHandle dc)
{
    CRect r;
    WIN32_OP_D(GetClientRect(&r));
    CBrush brush;
    WIN32_OP_D(brush.CreateSolidBrush(m_callback->query_std_color(ui_color_background)) != NULL);
    WIN32_OP_D(dc.FillRect(&r, brush));
    return TRUE;
}

void milk2_ui_element_instance::OnEnterSizeMove()
{
    s_in_sizemove = true;
}

void milk2_ui_element_instance::OnExitSizeMove()
{
    s_in_sizemove = false;
    auto vis = reinterpret_cast<Vis*>(GetWindowLongPtr(GWLP_USERDATA));
    if (vis)
    {
        CRect r;
        WIN32_OP_D(GetClientRect(&r));
        vis->OnWindowSizeChanged(r.right - r.left, r.bottom - r.top);
    }
}

void milk2_ui_element_instance::OnActivateApp(BOOL bActive, DWORD dwThreadID)
{
    auto vis = reinterpret_cast<Vis*>(GetWindowLongPtr(GWLP_USERDATA));
    if (vis)
    {
        if (bActive)
        {
            vis->OnActivated();
        }
        else
        {
            vis->OnDeactivated();
        }
    }
}

// Bit 29: The context code. The value is 1 if the ALT key is down while the key is pressed; it is 0 if the WM_SYSKEYDOWN message is posted to the active window because no window has the keyboard focus.
// Bit 30: The previous key state. The value is 1 if the key is down before the message is sent, or it is 0 if the key is up.
void milk2_ui_element_instance::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if (nChar == VK_RETURN && (nFlags & 0x6000) == 0x2000)
    {
        ToggleFullScreen();
    }
}

void milk2_ui_element_instance::OnContextMenu(CWindow wnd, CPoint point)
{
#ifndef NDEBUG
    //console::formatter() << core_api::get_my_file_name() << ": OnContextMenu " << point.x << ", " << point.y;
#endif
    if (m_callback->is_edit_mode_enabled())
    {
        SetMsgHandled(FALSE);
    }
    else
    {
        CMenu menu;
        menu.CreatePopupMenu();
        menu.AppendMenu(MF_STRING, IDM_TOGGLE_FULLSCREEN, TEXT("Toggle Full-Screen Mode"));
        menu.AppendMenu(MF_SEPARATOR);
        menu.AppendMenu(MF_STRING /* | (m_config.m_downmix_enabled ? MF_CHECKED : 0) */, IDM_DOWNMIX_ENABLED, TEXT("Downmix Channels"));

        CMenu durationMenu;
        durationMenu.CreatePopupMenu();
        durationMenu.AppendMenu(MF_STRING /* | ((m_config.m_window_duration_millis == 50) ? MF_CHECKED : 0) */, IDM_WINDOW_DURATION_50, TEXT("50 ms"));
        durationMenu.AppendMenu(MF_STRING /* | ((m_config.m_window_duration_millis == 100) ? MF_CHECKED : 0) */, IDM_WINDOW_DURATION_100, TEXT("100 ms"));

        menu.AppendMenu(MF_STRING, durationMenu, TEXT("Window Duration"));

        CMenu zoomMenu;
        zoomMenu.CreatePopupMenu();
        zoomMenu.AppendMenu(MF_STRING /* | ((m_config.m_zoom_percent == 50) ? MF_CHECKED : 0) */, IDM_ZOOM_50, TEXT("50 %"));
        zoomMenu.AppendMenu(MF_STRING /* | ((m_config.m_zoom_percent == 75) ? MF_CHECKED : 0) */, IDM_ZOOM_75, TEXT("75 %"));
        zoomMenu.AppendMenu(MF_STRING /* | ((m_config.m_zoom_percent == 100) ? MF_CHECKED : 0) */, IDM_ZOOM_100, TEXT("100 %"));

        menu.AppendMenu(MF_STRING, zoomMenu, TEXT("Zoom"));

        CMenu refreshRateLimitMenu;
        refreshRateLimitMenu.CreatePopupMenu();
        refreshRateLimitMenu.AppendMenu(MF_STRING /* | ((m_config.m_refresh_rate_limit_hz == 20) ? MF_CHECKED : 0) */, IDM_REFRESH_RATE_LIMIT_20, TEXT("20 Hz"));
        refreshRateLimitMenu.AppendMenu(MF_STRING /* | ((m_config.m_refresh_rate_limit_hz == 60) ? MF_CHECKED : 0) */, IDM_REFRESH_RATE_LIMIT_60, TEXT("60 Hz"));

        menu.AppendMenu(MF_STRING, refreshRateLimitMenu, TEXT("Refresh Rate Limit"));

        menu.AppendMenu(MF_SEPARATOR);

        menu.AppendMenu(MF_STRING /* | (m_config.m_resample_enabled ? MF_CHECKED : 0) */, IDM_RESAMPLE_ENABLED, TEXT("Resample For Display"));
        menu.AppendMenu(MF_STRING /* | (m_config.m_hw_rendering_enabled ? MF_CHECKED : 0) */, IDM_HW_RENDERING_ENABLED, TEXT("Allow Hardware Rendering"));

        menu.SetMenuDefaultItem(IDM_TOGGLE_FULLSCREEN);

        int cmd = menu.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, *this);

        switch (cmd)
        {
            case IDM_TOGGLE_FULLSCREEN:
                //ToggleFullScreen();
                break;
            case IDM_HW_RENDERING_ENABLED:
                //m_config.m_hw_rendering_enabled = !m_config.m_hw_rendering_enabled;
                //DiscardDeviceResources();
                break;
            case IDM_DOWNMIX_ENABLED:
                //m_config.m_downmix_enabled = !m_config.m_downmix_enabled;
                UpdateChannelMode();
                break;
            case IDM_RESAMPLE_ENABLED:
                //m_config.m_resample_enabled = !m_config.m_resample_enabled;
                break;
            case IDM_WINDOW_DURATION_50:
                //m_config.m_window_duration_millis = 50;
                break;
            case IDM_WINDOW_DURATION_100:
                //m_config.m_window_duration_millis = 100;
                break;
            case IDM_ZOOM_50:
                //m_config.m_zoom_percent = 50;
                break;
            case IDM_ZOOM_75:
                //m_config.m_zoom_percent = 75;
                break;
            case IDM_ZOOM_100:
                //m_config.m_zoom_percent = 100;
                break;
            case IDM_REFRESH_RATE_LIMIT_20:
                //m_config.m_refresh_rate_limit_hz = 20;
                //UpdateRefreshRateLimit();
                break;
            case IDM_REFRESH_RATE_LIMIT_60:
                //m_config.m_refresh_rate_limit_hz = 60;
                //UpdateRefreshRateLimit();
                break;
        }

        Invalidate();
    }
}

void milk2_ui_element_instance::AudioData(const float* pAudioData, size_t iAudioDataLength)
{
    int ipos = 0;
    while (ipos < 576)
    {
        for (int i = 0; static_cast<unsigned int>(i) < iAudioDataLength; i += 2)
        {
            g_vis->waves[0][ipos] = static_cast<unsigned char>(pAudioData[i] * 255.0f);
            g_vis->waves[1][ipos] = static_cast<unsigned char>(pAudioData[i + 1] * 255.0f);
            ipos++;
            if (ipos >= 576)
                break;
        }
    }
}

void milk2_ui_element_instance::AddPCM()
{
    if (!m_vis_stream.is_valid())
        return;

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
    if (dt > max_time)
        dt = max_time;

    audio_chunk_impl chunk;
    if (use_fake || !m_vis_stream->get_chunk_absolute(chunk, time - dt, dt))
        m_vis_stream->make_fake_chunk_absolute(chunk, time - dt, dt);
    auto count = chunk.get_sample_count();
    auto channels = chunk.get_channel_count();
    std::vector<int16_t> data(count * channels, 0);
    audio_math::convert_to_int16(chunk.get_data(), count * channels, data.data(), 1.0);
    //if (channels == 2)
    //    milk2_pcm_add_int16(data.data(), count, STEREO);
    //else
    //    milk2_pcm_add_int16(data.data(), count, MONO);
    float sin1add = 0.05f;
    float sin2add = 0.08f;

    //float color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    //pImmediateContext->ClearRenderTargetView(pRenderTargetView, color);

    //  sin1 += 10;
    //  sin2 += 20;

    float sin1start = sin1;
    float sin2start = sin2;

    float Current = 0;
    for (int i = 0; i < 576; i++)
    {
        //if ( ( rand() % 10) > 4)
        //iCurrent += (short)(rand() % (255));
        //else
        //iCurrent -= (short)(rand() % (255));
        Current = sinf(sin1 + sin2);
        //Current += sinf(sin2);
        sin1 += sin1add;
        sin2 += sin2add;
        g_vis->waves[0][i] = static_cast<unsigned char>(Current * 0.2f);
        g_vis->waves[1][i] = static_cast<unsigned char>(Current * 0.2f);
        //waves[0][i] = (rand() % 128 ) / 128.0f;//iCurrent;
        //waves[1][i] = (rand() % 128 ) / 128.0f;//iCurrent;
    }
    sin1 = sin1start + sin1add;
    sin2 = sin2start + sin2add * 7;
}

void milk2_ui_element_instance::UpdateChannelMode()
{
    if (m_vis_stream.is_valid())
    {
        m_vis_stream->set_channel_mode(false /* m_config.m_downmix_enabled */ ? visualisation_stream_v3::channel_mode_mono : visualisation_stream_v3::channel_mode_default);
    }
}

class ui_element_milk2 : public ui_element_impl_visualisation<milk2_ui_element_instance> {};

// Service factory publishes the class.
static service_factory_single_t<ui_element_milk2> g_ui_element_milk2_factory;

void ExitVis() noexcept
{
    //PostQuitMessage(0);
}

} // namespace