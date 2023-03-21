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
    void UpdateChannelMode();

    ui_element_config::ptr m_config; //milk2_config m_config;
    visualisation_stream_v3::ptr m_vis_stream;

    DWORD m_last_refresh;
    DWORD m_refresh_interval;
    bool m_IsInitialized = false;
    unsigned char waves[2][576];

    std::unique_ptr<DX::DeviceResources> m_deviceResources;

    enum
    {
        ID_REFRESH_TIMER = 1
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
        //m_pStrokeBrush.Release();
        //Invalidate();
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
    console::formatter() << "MilkDrop 2: OnCreate " << cs->x << ", " << cs->y;

    HRESULT hr = S_OK;

    m_deviceResources->SetWindow(get_wnd(), 0, 0);

    m_deviceResources->CreateDeviceResources();
    //CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    //CreateWindowSizeDependentResources();

    //if (FAILED(hr))
    //{
    //    console::formatter() << core_api::get_my_file_name() << ": could not create DirectX 11 factory";
    //}

    std::string base_path = core_api::get_my_full_path();
    std::string::size_type t = base_path.rfind('\\');
    if (t != std::string::npos)
        base_path.erase(t + 1);
    std::string data_zip = base_path + "data.zip";
    swprintf_s(g_plugin.m_szPluginsDirPath, L"%hs" /* L"%hs\\resources\\" */, const_cast<char*>(data_zip.c_str()));

    //if (FALSE == g_plugin.PluginPreInitialize(0, 0))
    //    hr = S_FALSE;

    //CRect rcClient;
    //GetClientRect(rcClient);
    //if (FALSE == g_plugin.PluginInitialize(static_cast<ID3D11DeviceContext*>(pImmediateContext), 0, 0, rcClient.Width(), rcClient.Height(), static_cast<float>(static_cast<double>(rcClient.Width()) / static_cast<double>(rcClient.Height()))))
    //    hr = S_FALSE;

    //if (FAILED(hr))
    //{
    //    console::formatter() << core_api::get_my_file_name() << ": could not initialize plugin";
    //}

    try
    {
        static_api_ptr_t<visualisation_manager> vis_manager;

        vis_manager->create_stream(m_vis_stream, 0);

        m_vis_stream->request_backlog(0.8);
        UpdateChannelMode();
    }
    catch (std::exception& exc)
    {
        console::formatter() << core_api::get_my_file_name() << ": exception while creating visualization stream: " << exc;
    }

    m_IsInitialized = true;
    return 0;
}

void milk2_ui_element_instance::OnDestroy()
{
    console::formatter() << "MilkDrop 2: OnDestroy";
    m_vis_stream.release();

    //m_pDirect2dFactory.Release();
    //m_pRenderTarget.Release();
    //m_pStrokeBrush.Release();
    
    if (m_IsInitialized)
    {
    //    g_plugin.PluginQuit();

    //    //pImmediateContext->Flush();
    //    //pImmediateContext->ClearState();

    //    //pRenderTargetView->Release();
    //    //pDepthStencilView->Release();
    //    //pSwapChain->Release();
    //    //pImmediateContext->Release();
    //    //pD3DDevice->Release();

        m_IsInitialized = false;
    }
}

void milk2_ui_element_instance::OnTimer(UINT_PTR nIDEvent)
{
    console::formatter() << "MilkDrop 2: OnTimer";
    //KillTimer(ID_REFRESH_TIMER);
    //Invalidate();
}

void milk2_ui_element_instance::OnPaint(CDCHandle dc)
{
    //console::formatter() << "MilkDrop 2: OnPaint";
    //Render();
    //ValidateRect(nullptr);

    //DWORD now = GetTickCount();
    //if (m_vis_stream.is_valid())
    //{
    //    DWORD next_refresh = m_last_refresh + m_refresh_interval;
    //    // (next_refresh < now) would break when GetTickCount() overflows
    //    if ((long)(next_refresh - now) < 0)
    //    {
    //        next_refresh = now;
    //    }
    //    SetTimer(ID_REFRESH_TIMER, next_refresh - now);
    //}
    //m_last_refresh = now;
}

void milk2_ui_element_instance::OnSize(UINT nType, CSize size)
{
    console::formatter() << "MilkDrop 2: OnSize " << size.cx << ", " << size.cy;

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
}

void milk2_ui_element_instance::OnContextMenu(CWindow wnd, CPoint point)
{
    console::formatter() << "MilkDrop 2: OnContextMenu " << point.x << ", " << point.y;
    if (m_callback->is_edit_mode_enabled())
    {
        SetMsgHandled(FALSE);
    }
    else
    {
        //CMenu menu;
        //menu.CreatePopupMenu();
        //menu.AppendMenu(MF_STRING, IDM_TOGGLE_FULLSCREEN, TEXT("Toggle Full-Screen Mode"));
        //menu.AppendMenu(MF_SEPARATOR);
        //menu.AppendMenu(MF_STRING | (m_config.m_downmix_enabled ? MF_CHECKED : 0), IDM_DOWNMIX_ENABLED, TEXT("Downmix Channels"));
        //menu.AppendMenu(MF_STRING | (m_config.m_low_quality_enabled ? MF_CHECKED : 0), IDM_LOW_QUALITY_ENABLED, TEXT("Low Quality Mode"));
        //menu.AppendMenu(MF_STRING | (m_config.m_trigger_enabled ? MF_CHECKED : 0), IDM_TRIGGER_ENABLED, TEXT("Trigger on Zero Crossing"));

        //CMenu durationMenu;
        //durationMenu.CreatePopupMenu();
        //durationMenu.AppendMenu(MF_STRING | ((m_config.m_window_duration_millis == 50) ? MF_CHECKED : 0), IDM_WINDOW_DURATION_50, TEXT("50 ms"));
        //durationMenu.AppendMenu(MF_STRING | ((m_config.m_window_duration_millis == 100) ? MF_CHECKED : 0), IDM_WINDOW_DURATION_100, TEXT("100 ms"));
        //durationMenu.AppendMenu(MF_STRING | ((m_config.m_window_duration_millis == 200) ? MF_CHECKED : 0), IDM_WINDOW_DURATION_200, TEXT("200 ms"));
        //durationMenu.AppendMenu(MF_STRING | ((m_config.m_window_duration_millis == 300) ? MF_CHECKED : 0), IDM_WINDOW_DURATION_300, TEXT("300 ms"));
        //durationMenu.AppendMenu(MF_STRING | ((m_config.m_window_duration_millis == 400) ? MF_CHECKED : 0), IDM_WINDOW_DURATION_400, TEXT("400 ms"));
        //durationMenu.AppendMenu(MF_STRING | ((m_config.m_window_duration_millis == 500) ? MF_CHECKED : 0), IDM_WINDOW_DURATION_500, TEXT("500 ms"));
        //durationMenu.AppendMenu(MF_STRING | ((m_config.m_window_duration_millis == 600) ? MF_CHECKED : 0), IDM_WINDOW_DURATION_600, TEXT("600 ms"));
        //durationMenu.AppendMenu(MF_STRING | ((m_config.m_window_duration_millis == 700) ? MF_CHECKED : 0), IDM_WINDOW_DURATION_700, TEXT("700 ms"));
        //durationMenu.AppendMenu(MF_STRING | ((m_config.m_window_duration_millis == 800) ? MF_CHECKED : 0), IDM_WINDOW_DURATION_800, TEXT("800 ms"));

        //menu.AppendMenu(MF_STRING, durationMenu, TEXT("Window Duration"));

        //CMenu zoomMenu;
        //zoomMenu.CreatePopupMenu();
        //zoomMenu.AppendMenu(MF_STRING | ((m_config.m_zoom_percent == 50) ? MF_CHECKED : 0), IDM_ZOOM_50, TEXT("50 %"));
        //zoomMenu.AppendMenu(MF_STRING | ((m_config.m_zoom_percent == 75) ? MF_CHECKED : 0), IDM_ZOOM_75, TEXT("75 %"));
        //zoomMenu.AppendMenu(MF_STRING | ((m_config.m_zoom_percent == 100) ? MF_CHECKED : 0), IDM_ZOOM_100, TEXT("100 %"));
        //zoomMenu.AppendMenu(MF_STRING | ((m_config.m_zoom_percent == 150) ? MF_CHECKED : 0), IDM_ZOOM_150, TEXT("150 %"));
        //zoomMenu.AppendMenu(MF_STRING | ((m_config.m_zoom_percent == 200) ? MF_CHECKED : 0), IDM_ZOOM_200, TEXT("200 %"));
        //zoomMenu.AppendMenu(MF_STRING | ((m_config.m_zoom_percent == 300) ? MF_CHECKED : 0), IDM_ZOOM_300, TEXT("300 %"));
        //zoomMenu.AppendMenu(MF_STRING | ((m_config.m_zoom_percent == 400) ? MF_CHECKED : 0), IDM_ZOOM_400, TEXT("400 %"));
        //zoomMenu.AppendMenu(MF_STRING | ((m_config.m_zoom_percent == 600) ? MF_CHECKED : 0), IDM_ZOOM_600, TEXT("600 %"));
        //zoomMenu.AppendMenu(MF_STRING | ((m_config.m_zoom_percent == 800) ? MF_CHECKED : 0), IDM_ZOOM_800, TEXT("800 %"));

        //menu.AppendMenu(MF_STRING, zoomMenu, TEXT("Zoom"));

        //CMenu refreshRateLimitMenu;
        //refreshRateLimitMenu.CreatePopupMenu();
        //refreshRateLimitMenu.AppendMenu(MF_STRING | ((m_config.m_refresh_rate_limit_hz == 20) ? MF_CHECKED : 0), IDM_REFRESH_RATE_LIMIT_20, TEXT("20 Hz"));
        //refreshRateLimitMenu.AppendMenu(MF_STRING | ((m_config.m_refresh_rate_limit_hz == 60) ? MF_CHECKED : 0), IDM_REFRESH_RATE_LIMIT_60, TEXT("60 Hz"));
        //refreshRateLimitMenu.AppendMenu(MF_STRING | ((m_config.m_refresh_rate_limit_hz == 100) ? MF_CHECKED : 0), IDM_REFRESH_RATE_LIMIT_100, TEXT("100 Hz"));
        //refreshRateLimitMenu.AppendMenu(MF_STRING | ((m_config.m_refresh_rate_limit_hz == 200) ? MF_CHECKED : 0), IDM_REFRESH_RATE_LIMIT_200, TEXT("200 Hz"));

        //menu.AppendMenu(MF_STRING, refreshRateLimitMenu, TEXT("Refresh Rate Limit"));

        //CMenu lineStrokeWidthMenu;
        //lineStrokeWidthMenu.CreatePopupMenu();
        //lineStrokeWidthMenu.AppendMenu(MF_STRING | ((m_config.m_line_stroke_width == 5) ? MF_CHECKED : 0), IDM_LINE_STROKE_WIDTH_5, TEXT("0.5 px"));
        //lineStrokeWidthMenu.AppendMenu(MF_STRING | ((m_config.m_line_stroke_width == 10) ? MF_CHECKED : 0), IDM_LINE_STROKE_WIDTH_10, TEXT("1.0 px"));
        //lineStrokeWidthMenu.AppendMenu(MF_STRING | ((m_config.m_line_stroke_width == 15) ? MF_CHECKED : 0), IDM_LINE_STROKE_WIDTH_15, TEXT("1.5 px"));
        //lineStrokeWidthMenu.AppendMenu(MF_STRING | ((m_config.m_line_stroke_width == 20) ? MF_CHECKED : 0), IDM_LINE_STROKE_WIDTH_20, TEXT("2.0 px"));
        //lineStrokeWidthMenu.AppendMenu(MF_STRING | ((m_config.m_line_stroke_width == 25) ? MF_CHECKED : 0), IDM_LINE_STROKE_WIDTH_25, TEXT("2.5 px"));
        //lineStrokeWidthMenu.AppendMenu(MF_STRING | ((m_config.m_line_stroke_width == 30) ? MF_CHECKED : 0), IDM_LINE_STROKE_WIDTH_30, TEXT("3.0 px"));

        //menu.AppendMenu(MF_STRING, lineStrokeWidthMenu, TEXT("Line Stroke Width"));

        //menu.AppendMenu(MF_SEPARATOR);

        //menu.AppendMenu(MF_STRING | (m_config.m_resample_enabled ? MF_CHECKED : 0), IDM_RESAMPLE_ENABLED, TEXT("Resample For Display"));
        //menu.AppendMenu(MF_STRING | (m_config.m_hw_rendering_enabled ? MF_CHECKED : 0), IDM_HW_RENDERING_ENABLED, TEXT("Allow Hardware Rendering"));

        //menu.SetMenuDefaultItem(IDM_TOGGLE_FULLSCREEN);

        //int cmd = menu.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, *this);

        //switch (cmd) {
        //case IDM_TOGGLE_FULLSCREEN:
        //	ToggleFullScreen();
        //	break;
        //case IDM_HW_RENDERING_ENABLED:
        //	m_config.m_hw_rendering_enabled = !m_config.m_hw_rendering_enabled;
        //	DiscardDeviceResources();
        //	break;
        //case IDM_DOWNMIX_ENABLED:
        //	m_config.m_downmix_enabled = !m_config.m_downmix_enabled;
        //	UpdateChannelMode();
        //	break;
        //case IDM_LOW_QUALITY_ENABLED:
        //	m_config.m_low_quality_enabled = !m_config.m_low_quality_enabled;
        //	break;
        //case IDM_TRIGGER_ENABLED:
        //	m_config.m_trigger_enabled = !m_config.m_trigger_enabled;
        //	break;
        //case IDM_RESAMPLE_ENABLED:
        //	m_config.m_resample_enabled = !m_config.m_resample_enabled;
        //	break;
        //case IDM_WINDOW_DURATION_50:
        //	m_config.m_window_duration_millis = 50;
        //	break;
        //case IDM_WINDOW_DURATION_100:
        //	m_config.m_window_duration_millis = 100;
        //	break;
        //case IDM_WINDOW_DURATION_200:
        //	m_config.m_window_duration_millis = 200;
        //	break;
        //case IDM_WINDOW_DURATION_300:
        //	m_config.m_window_duration_millis = 300;
        //	break;
        //case IDM_WINDOW_DURATION_400:
        //	m_config.m_window_duration_millis = 400;
        //	break;
        //case IDM_WINDOW_DURATION_500:
        //	m_config.m_window_duration_millis = 500;
        //	break;
        //case IDM_WINDOW_DURATION_600:
        //	m_config.m_window_duration_millis = 600;
        //	break;
        //case IDM_WINDOW_DURATION_700:
        //	m_config.m_window_duration_millis = 700;
        //	break;
        //case IDM_WINDOW_DURATION_800:
        //	m_config.m_window_duration_millis = 800;
        //	break;
        //case IDM_ZOOM_50:
        //	m_config.m_zoom_percent = 50;
        //	break;
        //case IDM_ZOOM_75:
        //	m_config.m_zoom_percent = 75;
        //	break;
        //case IDM_ZOOM_100:
        //	m_config.m_zoom_percent = 100;
        //	break;
        //case IDM_ZOOM_150:
        //	m_config.m_zoom_percent = 150;
        //	break;
        //case IDM_ZOOM_200:
        //	m_config.m_zoom_percent = 200;
        //	break;
        //case IDM_ZOOM_300:
        //	m_config.m_zoom_percent = 300;
        //	break;
        //case IDM_ZOOM_400:
        //	m_config.m_zoom_percent = 400;
        //	break;
        //case IDM_ZOOM_600:
        //	m_config.m_zoom_percent = 600;
        //	break;
        //case IDM_ZOOM_800:
        //	m_config.m_zoom_percent = 800;
        //	break;
        //case IDM_REFRESH_RATE_LIMIT_20:
        //	m_config.m_refresh_rate_limit_hz = 20;
        //	UpdateRefreshRateLimit();
        //	break;
        //case IDM_REFRESH_RATE_LIMIT_60:
        //	m_config.m_refresh_rate_limit_hz = 60;
        //	UpdateRefreshRateLimit();
        //	break;
        //case IDM_REFRESH_RATE_LIMIT_100:
        //	m_config.m_refresh_rate_limit_hz = 100;
        //	UpdateRefreshRateLimit();
        //	break;
        //case IDM_REFRESH_RATE_LIMIT_200:
        //	m_config.m_refresh_rate_limit_hz = 200;
        //	UpdateRefreshRateLimit();
        //	break;
        //case IDM_LINE_STROKE_WIDTH_5:
        //	m_config.m_line_stroke_width = 5;
        //	break;
        //case IDM_LINE_STROKE_WIDTH_10:
        //	m_config.m_line_stroke_width = 10;
        //	break;
        //case IDM_LINE_STROKE_WIDTH_15:
        //	m_config.m_line_stroke_width = 15;
        //	break;
        //case IDM_LINE_STROKE_WIDTH_20:
        //	m_config.m_line_stroke_width = 20;
        //	break;
        //case IDM_LINE_STROKE_WIDTH_25:
        //	m_config.m_line_stroke_width = 25;
        //	break;
        //case IDM_LINE_STROKE_WIDTH_30:
        //	m_config.m_line_stroke_width = 30;
        //	break;
        //}

        Invalidate();
    }
}

HRESULT milk2_ui_element_instance::Render()
{
    //g_plugin.PluginRender(waves[0], waves[1]);

    HRESULT hr = S_OK;

    //    if (m_vis_stream.is_valid()) {
    //        double time;
    //        if (m_vis_stream->get_absolute_time(time)) {
    //            double window_duration = m_config.get_window_duration();
    //            audio_chunk_impl chunk;
    //            if (m_vis_stream->get_chunk_absolute(chunk, time - window_duration / 2, window_duration * (m_config.m_trigger_enabled ? 2 : 1))) {
    //                RenderChunk(chunk);
    //            }
    //        }
    //    }


    return hr;
}

HRESULT milk2_ui_element_instance::RenderChunk(const audio_chunk& chunk)
{
    HRESULT hr = S_OK;

    RECT rt = m_deviceResources->GetOutputSize();
    struct MD2_SIZE_F
    {
        FLOAT width;
        FLOAT height;
    } rtSize = {static_cast<float>(rt.right - rt.left), static_cast<float>(rt.bottom - rt.top)};

    audio_chunk_impl chunk2;
    chunk2.copy(chunk);

    if (false) //m_config.m_resample_enabled)
    {
        unsigned int display_sample_rate = (unsigned)(rtSize.width / 0.1 /* m_config.get_window_duration() */);
        unsigned int target_sample_rate = chunk.get_sample_rate();
        while (target_sample_rate >= 2 && target_sample_rate > display_sample_rate)
        {
            target_sample_rate /= 2;
        }
        if (target_sample_rate != chunk.get_sample_rate())
        {
            dsp::ptr resampler;
            metadb_handle::ptr track;
            if (static_api_ptr_t<playback_control>()->get_now_playing(track) &&
                resampler_entry::g_create(resampler, chunk.get_sample_rate(), target_sample_rate, 1.0f))
            {
                dsp_chunk_list_impl chunk_list;
                chunk_list.add_chunk(&chunk);
                resampler->run(&chunk_list, track, dsp::FLUSH);
                resampler->flush();

                bool consistent_format = true;
                unsigned total_sample_count = 0;
                for (t_size chunk_index = 0; chunk_index < chunk_list.get_count(); ++chunk_index)
                {
                    if ((chunk_list.get_item(chunk_index)->get_sample_rate() ==
                         chunk_list.get_item(0)->get_sample_rate()) &&
                        (chunk_list.get_item(chunk_index)->get_channel_count() ==
                         chunk_list.get_item(0)->get_channel_count()))
                    {
                        total_sample_count += chunk_list.get_item(chunk_index)->get_sample_count();
                    }
                    else
                    {
                        consistent_format = false;
                        break;
                    }
                }
                if (consistent_format && chunk_list.get_count() > 0)
                {
                    unsigned channel_count = chunk_list.get_item(0)->get_channels();
                    unsigned sample_rate = chunk_list.get_item(0)->get_sample_rate();

                    pfc::array_t<audio_sample> buffer;
                    buffer.prealloc(channel_count * total_sample_count);
                    for (t_size chunk_index = 0; chunk_index < chunk_list.get_count(); ++chunk_index)
                    {
                        audio_chunk* c = chunk_list.get_item(chunk_index);
                        buffer.append_fromptr(c->get_data(), c->get_channel_count() * c->get_sample_count());
                    }

                    chunk2.set_data(buffer.get_ptr(), total_sample_count, channel_count, sample_rate);
                }
            }
        }
    }

    t_uint32 channel_count = chunk2.get_channel_count();
    t_uint32 sample_count_total = chunk2.get_sample_count();
    t_uint32 sample_count = false /* m_config.m_trigger_enabled */ ? sample_count_total / 2 : sample_count_total;
    const audio_sample* samples = chunk2.get_data();

    if (false) //m_config.m_trigger_enabled)
    {
        t_uint32 cross_min = sample_count;
        t_uint32 cross_max = 0;

        for (t_uint32 channel_index = 0; channel_index < channel_count; ++channel_index)
        {
            audio_sample sample0 = samples[channel_index];
            audio_sample sample1 = samples[1 * channel_count + channel_index];
            audio_sample sample2;
            for (t_uint32 sample_index = 2; sample_index < sample_count; ++sample_index)
            {
                sample2 = samples[sample_index * channel_count + channel_index];
                if ((sample0 < 0.0) && (sample1 >= 0.0) && (sample2 >= 0.0))
                {
                    if (cross_min > sample_index - 1)
                        cross_min = sample_index - 1;
                    if (cross_max < sample_index - 1)
                        cross_max = sample_index - 1;
                }
                sample0 = sample1;
                sample1 = sample2;
            }
        }

        samples += cross_min * channel_count;
    }

    for (t_uint32 channel_index = 0; channel_index < channel_count; ++channel_index)
    {
        float zoom = 1.0; //(float)m_config.get_zoom_factor();
        float channel_baseline = (float)(channel_index + 0.5) / (float)channel_count * rtSize.height;
        for (t_uint32 sample_index = 0; sample_index < sample_count; ++sample_index)
        {
            audio_sample sample = samples[sample_index * channel_count + channel_index];
            float x = (float)sample_index / (float)(sample_count - 1) * rtSize.width;
            float y = channel_baseline - sample * zoom * rtSize.height / 2 / channel_count + 0.5f;
            //if (sample_index == 0)
            //{
            //    pSink->BeginFigure(D2D1::Point2F(x, y), D2D1_FIGURE_BEGIN_HOLLOW);
            //}
            //else
            //{
            //    pSink->AddLine(D2D1::Point2F(x, y));
            //}
        }
        //if (channel_count > 0 && sample_count > 0)
        //{
        //    pSink->EndFigure(D2D1_FIGURE_END_OPEN);
        //}
    }

    return hr;
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
            waves[0][ipos] = char(pAudioData[i] * 255.0f);
            waves[1][ipos] = char(pAudioData[i + 1] * 255.0f);
            ipos++;
            if (ipos >= 576)
                break;
        }
    }
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
