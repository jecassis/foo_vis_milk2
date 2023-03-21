#include "pch.h"

#include "../vis_milk2/plugin.h"
#include "DeviceResources.h"

CPlugin g_plugin;

// Anonymous namespace is standard practice in foobar2000 components.
// Nothing outside should have any reason to see these symbols and do not want
// funny results if another component has similarly named classes.
namespace 
{
static const GUID guid_milk2 = {0x204b0345, 0x4df5, 0x4b47, {0xad, 0xd3, 0x98, 0x9f, 0x81, 0x1b, 0xd9, 0xa5}};

class milk2_ui_element_instance : public ui_element_instance, public CWindowImpl<milk2_ui_element_instance> //,public DX::IDeviceNotify
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
        //MSG_WM_TIMER(OnSizeTimer)
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
    LRESULT OnCreate(LPCREATESTRUCT cs);
    void OnDestroy();
    void OnTimer(UINT_PTR nIDEvent);
    void OnPaint(CDCHandle dc);
    void OnSize(UINT nType, CSize size);
    //void OnSizeTimer(UINT_PTR id);
    void OnContextMenu(CWindow wnd, CPoint point);
    void OnLButtonDblClk(UINT nFlags, CPoint point) { ToggleFullScreen(); }

    void ToggleFullScreen() { static_api_ptr_t<ui_element_common_methods_v2>()->toggle_fullscreen(g_get_guid(), core_api::get_main_window()); }
    HRESULT Render();
    HRESULT RenderChunk(const audio_chunk& chunk);
    bool PrevPreset();
    bool NextPreset();
    bool LoadPreset(int select);
    bool RandomPreset();
    bool LockPreset(bool lockUnlock);
    bool IsLocked() { return g_plugin.m_bPresetLockedByUser; }
    bool GetPresets(std::vector<std::string>& presets);
    int GetActivePreset();
    char* WideToUTF8(const wchar_t* WFilename);
    void AudioData(const float* pAudioData, size_t iAudioDataLength);
    void AddPCM();
    void UpdateChannelMode();

    ui_element_config::ptr m_config; //milk2_config m_config;
    visualisation_stream_v3::ptr m_vis_stream;

    DWORD m_last_refresh;
    DWORD m_refresh_interval;
    double m_last_time = 0.0;
    bool m_IsInitialized = false;
    unsigned char waves[2][576];

    std::unique_ptr<DX::DeviceResources> m_deviceResources;

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
    //set_configuration(config);
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D24_UNORM_S8_UINT, 2, D3D_FEATURE_LEVEL_11_0);
    //m_deviceResources->RegisterDeviceNotify(this);
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

//    UpdateChannelMode();
//    UpdateRefreshRateLimit();
//}

//ui_element_config::ptr milk2_ui_element_instance::get_configuration() {
//    ui_element_config_builder builder;
//    m_config.build(builder);
//    return builder.finish(g_get_guid());
//}

LRESULT milk2_ui_element_instance::OnCreate(LPCREATESTRUCT cs)
{
    console::formatter() << core_api::get_my_file_name() << ": OnCreate " << cs->x << ", " << cs->y;

    HRESULT hr = S_OK;

    m_deviceResources->SetWindow(get_wnd(), 0, 0);

    m_deviceResources->CreateDeviceResources();
    //CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    //CreateWindowSizeDependentResources();

    std::string base_path = core_api::get_my_full_path();
    std::string::size_type t = base_path.rfind('\\');
    if (t != std::string::npos)
        base_path.erase(t + 1);
    swprintf_s(g_plugin.m_szPluginsDirPath, L"%hs" /* L"%hs\\resources\\" */, const_cast<char*>(base_path.c_str()));

    if (FALSE == g_plugin.PluginPreInitialize(0, 0))
        hr = E_FAIL;

    CRect rcClient;
    GetClientRect(rcClient);
    console::formatter() << core_api::get_my_file_name() << ": OnCreate2 " << rcClient.Width() << ", " << rcClient.Height();

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
    console::formatter() << core_api::get_my_file_name() << ": OnDestroy";
    m_vis_stream.release();

    if (m_IsInitialized)
    {
        g_plugin.PluginQuit();
        m_IsInitialized = false;
    }
}

void milk2_ui_element_instance::OnTimer(UINT_PTR nIDEvent)
{
    //console::formatter() << core_api::get_my_file_name() << ": OnTimer";
    KillTimer(ID_REFRESH_TIMER);
    Invalidate();
}

void milk2_ui_element_instance::OnPaint(CDCHandle dc)
{
    console::formatter() << core_api::get_my_file_name() << ": OnPaint";
    Render();
    ValidateRect(nullptr);
    AddPCM();

    DWORD now = GetTickCount();
    if (m_vis_stream.is_valid())
    {
        DWORD next_refresh = m_last_refresh + m_refresh_interval;
        // (next_refresh < now) would break when GetTickCount() overflows
        if ((long)(next_refresh - now) < 0)
        {
            next_refresh = now;
        }
        SetTimer(ID_REFRESH_TIMER, next_refresh - now);
    }
    m_last_refresh = now;
}

void milk2_ui_element_instance::OnSize(UINT nType, CSize size)
{
    //if (size.cx && size.cy)
    //{
    //    SetTimer(1, 100);
        console::formatter() << core_api::get_my_file_name() << ": OnSize " << size.cx << ", " << size.cy;
    //}
//}
//
//void milk2_ui_element_instance::OnSizeTimer(UINT_PTR id)
//{
//    KillTimer(1);

    HRESULT hr = S_OK;
    RECT r;
    GetClientRect(&r);

    int width = r.right - r.left;
    int height = r.bottom - r.top;

    if (!width || !height)
        return;

    if (width < 128)
        width = 128;

    if (height < 128)
        height = 128;

    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    //CreateWindowSizeDependentResources();
    console::formatter() << core_api::get_my_file_name() << ": OnSizeTimer " << width << ", " << height;
    if (FALSE == g_plugin.PluginInitialize(m_deviceResources->GetD3DDeviceContext(), 0, 0, width, height, static_cast<float>(static_cast<double>(width) / static_cast<double>(height))))
        hr = E_FAIL;
    else
        m_IsInitialized = true;

    if (FAILED(hr))
        console::formatter() << core_api::get_my_file_name() << ": Could not initialize MilkDrop";
}

void milk2_ui_element_instance::OnContextMenu(CWindow wnd, CPoint point)
{
    console::formatter() << core_api::get_my_file_name() << ": OnContextMenu " << point.x << ", " << point.y;
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

HRESULT milk2_ui_element_instance::Render()
{
    g_plugin.PluginRender(waves[0], waves[1]);

    return S_OK;
}

bool milk2_ui_element_instance::NextPreset()
{
    g_plugin.NextPreset(1.0f);
    return true;
}

bool milk2_ui_element_instance::PrevPreset()
{
    g_plugin.PrevPreset(1.0f);
    return true;
}

bool milk2_ui_element_instance::LoadPreset(int select)
{
    g_plugin.m_nCurrentPreset = select + g_plugin.m_nDirs;

    wchar_t szFile[MAX_PATH] = {0};
    wcscpy_s(szFile, g_plugin.m_szPresetDir); // Note: m_szPresetDir always ends with '\'
    wcscat_s(szFile, g_plugin.m_presets[g_plugin.m_nCurrentPreset].szFilename.c_str());

    g_plugin.LoadPreset(szFile, 1.0f);
    return true;
}

bool milk2_ui_element_instance::LockPreset(bool lockUnlock)
{
    g_plugin.m_bPresetLockedByUser = lockUnlock;
    return true;
}

bool milk2_ui_element_instance::RandomPreset()
{
    g_plugin.LoadRandomPreset(1.0f);
    return true;
}

bool milk2_ui_element_instance::GetPresets(std::vector<std::string>& presets)
{
    if (!m_IsInitialized)
        return false;

    while (!g_plugin.m_bPresetListReady) {}

    for (int i = 0; i < g_plugin.m_nPresets - g_plugin.m_nDirs; ++i)
    {
        PresetInfo& Info = g_plugin.m_presets[i + g_plugin.m_nDirs];
        presets.push_back(WideToUTF8(Info.szFilename.c_str()));
    }

    return true;
}

int milk2_ui_element_instance::GetActivePreset()
{
    if (m_IsInitialized)
    {
        int CurrentPreset = g_plugin.m_nCurrentPreset;
        CurrentPreset -= g_plugin.m_nDirs;
        return CurrentPreset;
    }

    return -1;
}

char* milk2_ui_element_instance::WideToUTF8(const wchar_t* WFilename)
{
    int SizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &WFilename[0], -1, NULL, 0, NULL, NULL);
    char* utf8Name = new char[SizeNeeded];
    WideCharToMultiByte(CP_UTF8, 0, &WFilename[0], -1, &utf8Name[0], SizeNeeded, NULL, NULL);
    return utf8Name;
}

void milk2_ui_element_instance::AudioData(const float* pAudioData, size_t iAudioDataLength)
{
    int ipos = 0;
    while (ipos < 576)
    {
        for (int i = 0; static_cast<unsigned int>(i) < iAudioDataLength; i += 2)
        {
            waves[0][ipos] = static_cast<char>(pAudioData[i] * 255.0f);
            waves[1][ipos] = static_cast<char>(pAudioData[i + 1] * 255.0f);
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

} // namespace
