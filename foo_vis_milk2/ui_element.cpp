#include "pch.h"
#include "config.h"
#include "steptimer.h"
#include "resource.h"

#include <vis_milk2/plugin.h>

CPlugin g_plugin;

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

//std::unique_ptr<Vis> g_vis;

class milk2_ui_element_instance : public ui_element_instance, public CWindowImpl<milk2_ui_element_instance>
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
        MESSAGE_HANDLER(WM_MILK2, OnMilk2Message)
    END_MSG_MAP()
    // clang-format on

    milk2_ui_element_instance(ui_element_config::ptr config, ui_element_instance_callback_ptr p_callback);
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
    LRESULT OnMilk2Message(UINT message, WPARAM wparam, LPARAM lparam, BOOL& handled);

    milk2_config m_config;
    visualisation_stream_v3::ptr m_vis_stream;

    ULONGLONG m_last_refresh;
    DWORD m_refresh_interval = 33;
    double m_last_time = 0.0;
    bool m_IsInitialized = false;

    //Vis* vis;
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

    // Device status
    void OnDeviceLost();
    void OnDeviceRestored();

    void PrevPreset();
    void NextPreset();
    bool LoadPreset(int select);
    void RandomPreset();
    void LockPreset(bool lockUnlock);
    bool IsPresetLock();
    std::wstring GetCurrentPreset();
    bool GetPresets(std::vector<std::string>& presets);
    int GetActivePreset();
    char* WideToUTF8(const wchar_t* WFilename);

    // Window Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnDisplayChange();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;
    void SetPwd(std::string pwd) noexcept;
    void UpdateChannelMode();
    void ToggleFullScreen();

    void Update(DX::StepTimer const& timer);
    HRESULT Render();
    void BuildWaves();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // MilkDrop status
    bool m_milk2 = false;

    // Rendering loop timer
    DX::StepTimer m_timer;

    // Component paths
    std::string m_pwd;

    unsigned char waves[2][576];

  protected:
    const ui_element_instance_callback_ptr m_callback;
};

milk2_ui_element_instance::milk2_ui_element_instance(ui_element_config::ptr config, ui_element_instance_callback_ptr p_callback) :
    m_callback(p_callback)
{
    s_in_sizemove = false;
    s_in_suspend = false;
    s_minimized = false;

    set_configuration(config);
    m_pwd = ".\\";
}

ui_element_config::ptr milk2_ui_element_instance::g_get_default_configuration()
{
    ui_element_config_builder builder;
    milk2_config config;
    config.build(builder);
    return builder.finish(g_get_guid());
    //return ui_element_config::g_create_empty(g_get_guid());
}

void milk2_ui_element_instance::set_configuration(ui_element_config::ptr p_data)
{
    ui_element_config_parser parser(p_data);
    milk2_config config;
    config.parse(parser);
    m_config = config;

    UpdateChannelMode();
    //UpdateRefreshRateLimit();
}

ui_element_config::ptr milk2_ui_element_instance::get_configuration()
{
    ui_element_config_builder builder;
    m_config.build(builder);
    return builder.finish(g_get_guid());
}

void milk2_ui_element_instance::notify(const GUID& p_what, t_size p_param1, const void* p_param2, t_size p_param2size)
{
    if (p_what == ui_element_notify_colors_changed || p_what == ui_element_notify_font_changed)
    {
        Invalidate();
    }
}

int milk2_ui_element_instance::OnCreate(LPCREATESTRUCT cs)
{
    MILK2_CONSOLE_LOG("OnCreate ", cs->x, ", ", cs->y);
    std::string base_path = core_api::get_my_full_path();
    std::string::size_type t = base_path.rfind('\\');
    if (t != std::string::npos)
        base_path.erase(t + 1);
    SetPwd(base_path);

    //SetWindowLongPtr(GWLP_USERDATA, reinterpret_cast<LONG_PTR>(g_vis.get()));

    HRESULT hr = S_OK;
    int w, h;
    GetDefaultSize(w, h);

    //CRect rd = {0, 0, static_cast<LONG>(w), static_cast<LONG>(h)};
    CRect r{};
    WIN32_OP_D(GetClientRect(&r))
    if (r.right - r.left > 0 && r.bottom - r.top)
    {
        w = r.right - r.left;
        h = r.bottom - r.top;
    }
    if (!Initialize(get_wnd(), w, h))
    {
        MILK2_CONSOLE_LOG("Could not initialize MilkDrop");
    }
    MILK2_CONSOLE_LOG("OnCreate2 ", r.right, ", ", r.left, ", ", r.top, ", ", r.bottom);

    try
    {
        static_api_ptr_t<visualisation_manager> vis_manager;

        vis_manager->create_stream(m_vis_stream, 0);

        m_vis_stream->request_backlog(0.8);
        UpdateChannelMode();
    }
    catch (std::exception& exc)
    {
        MILK2_CONSOLE_LOG("Exception while creating visualization stream - ", exc);
    }

    return hr;
}

void milk2_ui_element_instance::OnDestroy()
{
    MILK2_CONSOLE_LOG("OnDestroy");
    m_vis_stream.release();

    if (m_IsInitialized)
    {
        OnDeviceLost();
        //g_vis.reset();
        m_IsInitialized = false;
    }
}

void milk2_ui_element_instance::OnTimer(UINT_PTR nIDEvent)
{
    //MILK2_CONSOLE_LOG("OnTimer");
    KillTimer(ID_REFRESH_TIMER);
    Invalidate();
}

void milk2_ui_element_instance::OnPaint(CDCHandle dc)
{
    //MILK2_CONSOLE_LOG("OnPaint");
    //auto vis = reinterpret_cast<Vis*>(GetWindowLongPtr(GWLP_USERDATA));
    if (m_milk2) //s_in_sizemove && vis
        Tick();
    else
    {
        PAINTSTRUCT ps;
        std::ignore = BeginPaint(&ps);
        EndPaint(&ps);
    }
    ValidateRect(nullptr);

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

void milk2_ui_element_instance::OnMove(CPoint ptPos)
{
    //auto vis = reinterpret_cast<Vis*>(GetWindowLongPtr(GWLP_USERDATA));
    OnWindowMoved();
}

void milk2_ui_element_instance::OnSize(UINT nType, CSize size)
{
    MILK2_CONSOLE_LOG("OnSize ", size.cx, ", ", size.cy);
    //auto vis = reinterpret_cast<Vis*>(GetWindowLongPtr(GWLP_USERDATA));
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
        OnWindowSizeChanged(size.cx, size.cy);
    }
}

BOOL milk2_ui_element_instance::OnEraseBkgnd(CDCHandle dc)
{
    MILK2_CONSOLE_LOG("OnEraseBkgnd");
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
    //auto vis = reinterpret_cast<Vis*>(GetWindowLongPtr(GWLP_USERDATA));
    if (m_milk2)
    {
        CRect r;
        WIN32_OP_D(GetClientRect(&r));
        OnWindowSizeChanged(r.right - r.left, r.bottom - r.top);
    }
}

void milk2_ui_element_instance::OnActivateApp(BOOL bActive, DWORD dwThreadID)
{
    //auto vis = reinterpret_cast<Vis*>(GetWindowLongPtr(GWLP_USERDATA));
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

void milk2_ui_element_instance::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    // Bit 29: The context code. The value is 1 if the ALT key is down while the
    //         key is pressed; it is 0 if the WM_SYSKEYDOWN message is posted to
    //         the active window because no window has the keyboard focus.
    // Bit 30: The previous key state. The value is 1 if the key is down before
    //         the message is sent, or it is 0 if the key is up.
    if (nChar == VK_RETURN && (nFlags & 0x6000) == 0x2000)
    {
        ToggleFullScreen();
    }
}

void milk2_ui_element_instance::OnContextMenu(CWindow wnd, CPoint point)
{
    MILK2_CONSOLE_LOG("OnContextMenu ", point.x, ", ", point.y);
    if (m_callback->is_edit_mode_enabled())
    {
        SetMsgHandled(FALSE);
        return;
    }

    // A (-1,-1) point is due to context menu key rather than right click.
    // `GetContextMenuPoint()` fixes that, returning a proper point at which the menu should be shown.
    //point = m_list.GetContextMenuPoint(point);
    CMenu menu;
    WIN32_OP_D(menu.CreatePopupMenu());
    menu.AppendMenu(MF_GRAYED, IDM_CURRENT_PRESET, GetCurrentPreset().c_str());
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING, IDM_NEXT_PRESET, TEXT("Next Preset"));
    menu.AppendMenu(MF_STRING, IDM_PREVIOUS_PRESET, TEXT("Previous Preset"));
    menu.AppendMenu(MF_STRING, IDM_SHUFFLE_PRESET, TEXT("Random Preset"));
    menu.AppendMenu(MF_STRING | (m_config.m_bPresetLockedByUser ? MF_CHECKED : 0), IDM_LOCK_PRESET, TEXT("Lock Preset"));
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING | (m_config.m_bEnableDownmix ? MF_CHECKED : 0), IDM_ENABLE_DOWNMIX, TEXT("Downmix Channels"));
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING, IDM_TOGGLE_FULLSCREEN, TEXT("Full Screen"));

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

    Invalidate();
}

LRESULT milk2_ui_element_instance::OnMilk2Message(UINT message, WPARAM wparam, LPARAM lparam, BOOL& handled)
{
    handled = TRUE;
    if (message != WM_MILK2)
        return 1;
    if (LOBYTE(wparam) == 0x21 && HIBYTE(wparam) == 0x09)
    {
        wchar_t buf[2048], title[64];
        LoadString(core_api::get_my_instance(), LOWORD(lparam), buf, 2048);
        LoadString(core_api::get_my_instance(), HIWORD(lparam), title, 64);
        MILK2_CONSOLE_LOG("milk2 -> title: ", title, ", message: ", buf);
    }

    return 0;
}

// Initialize the Direct3D resources required to run.
bool milk2_ui_element_instance::Initialize(HWND window, int width, int height)
{
    swprintf_s(g_plugin.m_szPluginsDirPath, L"%hs", const_cast<char*>(m_pwd.c_str()));
    swprintf_s(g_plugin.m_szConfigIniFile, L"%hs%ls", const_cast<char*>(m_pwd.c_str()), INIFILE);

    if (FALSE == g_plugin.PluginPreInitialize(window, core_api::get_my_instance()))
        return false;

    if (FALSE == g_plugin.PluginInitialize(0, 0, width, height, static_cast<float>(width) / static_cast<float>(height)))
        return false;

    m_milk2 = true;
    return true;
}

#pragma region Frame Update
// Executes the basic game loop.
void milk2_ui_element_instance::Tick()
{
    m_timer.Tick([&]() { Update(m_timer); });

    Render();
}

// Updates the world.
void milk2_ui_element_instance::Update(DX::StepTimer const& timer)
{
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
HRESULT milk2_ui_element_instance::Render()
{
    // Do not try to render anything before the first `Update()`.
    if (m_timer.GetFrameCount() == 0)
    {
        return S_OK;
    }

    Clear();

    return g_plugin.PluginRender(waves[0], waves[1]);
}

void milk2_ui_element_instance::BuildWaves()
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

// Helper method to clear the back buffers.
void milk2_ui_element_instance::Clear()
{
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void milk2_ui_element_instance::OnActivated()
{
}

void milk2_ui_element_instance::OnDeactivated()
{
}

void milk2_ui_element_instance::OnSuspending()
{
}

void milk2_ui_element_instance::OnResuming()
{
    m_timer.ResetElapsedTime();
}

void milk2_ui_element_instance::OnWindowMoved()
{
}

void milk2_ui_element_instance::OnDisplayChange()
{
}

void milk2_ui_element_instance::OnWindowSizeChanged(int width, int height)
{
    g_plugin.OnUserResizeWindow();

    CreateWindowSizeDependentResources();
}
#pragma endregion

// Properties
void milk2_ui_element_instance::GetDefaultSize(int& width, int& height) const noexcept
{
    width = 640;
    height = 480;
}

void milk2_ui_element_instance::SetPwd(std::string pwd) noexcept
{
    m_pwd.assign(pwd);
}

void milk2_ui_element_instance::ToggleFullScreen()
{
    static_api_ptr_t<ui_element_common_methods_v2>()->toggle_fullscreen(g_get_guid(), core_api::get_main_window());
}

void milk2_ui_element_instance::NextPreset()
{
    g_plugin.NextPreset(1.0f);
}

void milk2_ui_element_instance::PrevPreset()
{
    g_plugin.PrevPreset(1.0f);
}

bool milk2_ui_element_instance::LoadPreset(int select)
{
    //g_plugin.m_nCurrentPreset = select + g_plugin.m_nDirs;

    //wchar_t szFile[MAX_PATH] = {0};
    //wcscpy_s(szFile, g_plugin.m_szPresetDir); // Note: m_szPresetDir always ends with '\'
    //wcscat_s(szFile, g_plugin.m_presets[g_plugin.m_nCurrentPreset].szFilename.c_str());

    //g_plugin.LoadPreset(szFile, 1.0f);
    return true;
}

std::wstring milk2_ui_element_instance::GetCurrentPreset()
{
    return g_plugin.m_presets[g_plugin.m_nCurrentPreset].szFilename;
}

void milk2_ui_element_instance::LockPreset(bool lockUnlock)
{
    g_plugin.m_bPresetLockedByUser = lockUnlock;
}

bool milk2_ui_element_instance::IsPresetLock()
{
    return g_plugin.m_bPresetLockedByUser;
}

void milk2_ui_element_instance::RandomPreset()
{
    g_plugin.LoadRandomPreset(1.0f);
}

bool milk2_ui_element_instance::GetPresets(std::vector<std::string>& presets)
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

int milk2_ui_element_instance::GetActivePreset()
{
    //if (m_milk2)
    //{
    //    int CurrentPreset = g_plugin.m_nCurrentPreset;
    //    CurrentPreset -= g_plugin.m_nDirs;
    //    return CurrentPreset;
    //}

    return -1;
}

char* milk2_ui_element_instance::WideToUTF8(const wchar_t* WFilename)
{
    int SizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &WFilename[0], -1, NULL, 0, NULL, NULL);
    char* utf8Name = new char[SizeNeeded];
    WideCharToMultiByte(CP_UTF8, 0, &WFilename[0], -1, &utf8Name[0], SizeNeeded, NULL, NULL);
    return utf8Name;
}

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void milk2_ui_element_instance::CreateDeviceDependentResources()
{
}

// Allocate all memory resources that change on a window SizeChanged event.
void milk2_ui_element_instance::CreateWindowSizeDependentResources()
{
}

void milk2_ui_element_instance::OnDeviceLost()
{
    g_plugin.PluginQuit();
}

void milk2_ui_element_instance::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion

// clang-format off
void milk2_ui_element_instance::UpdateChannelMode()
{
    if (m_vis_stream.is_valid())
    {
        m_vis_stream->set_channel_mode(m_config.m_bEnableDownmix ? visualisation_stream_v3::channel_mode_mono : visualisation_stream_v3::channel_mode_default);
    }
}

class ui_element_milk2 : public ui_element_impl_visualisation<milk2_ui_element_instance> {};

// Service factory publishes the class.
static service_factory_single_t<ui_element_milk2> g_ui_element_milk2_factory;
} // namespace

void ExitVis() noexcept
{
    PostQuitMessage(0);
}
// clang-format on