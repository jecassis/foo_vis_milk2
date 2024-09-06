/*
 * ui_element.cpp - Implements the MilkDrop 2 visualization component.
 *
 * Copyright (c) 2023-2024 Jimmy Cassis
 * SPDX-License-Identifier: MPL-2.0
 */

#include "pch.h"
#include "ui_element.h"

#pragma hdrstop

//#ifdef __clang__
//#pragma clang diagnostic ignored "-Wcovered-switch-default"
//#pragma clang diagnostic ignored "-Wswitch-enum"
//#endif

CPlugin g_plugin;
HWND g_hWindow;

// Indicates to hybrid graphics systems to prefer the discrete part by default.
//extern "C" {
//    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
//    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
//}

namespace
{
milk2_ui_element::milk2_ui_element(ui_element_config::ptr config, ui_element_instance_callback_ptr p_callback) :
    m_callback(p_callback),
    m_bMsgHandled(TRUE),
    play_callback_impl_base(flag_on_playback_starting | flag_on_playback_new_track | flag_on_playback_stop),
    playlist_callback_impl_base(flag_on_items_added | flag_on_items_reordered | flag_on_items_removed | flag_on_items_selection_change |
                                flag_on_item_focus_change | flag_on_items_modified | flag_on_playlist_activate | flag_on_playlists_reorder |
                                flag_on_playlists_removed | flag_on_playback_order_changed)
{
    m_milk2 = false;
    m_in_sizemove = false;
    m_in_suspend = false;
    m_minimized = false;
#if defined(TIMER_TP)
    m_tpTimer = nullptr;
#elif defined(TIMER_32)
    m_last_time = 0.0;
#endif
    m_refresh_interval = 33;

    m_pwd = L".\\";
    set_configuration(config);
}

ui_element_config::ptr milk2_ui_element::g_get_default_configuration()
{
    try
    {
        ui_element_config_builder builder;
        milk2_config config;
        config.init();
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
    //if (dataptr && p_data->get_data_size() >= 4 && static_cast<DWORD*>(dataptr)[0] == ('M' | 'I' << 8 | 'L' << 16 | 'K' << 24))
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

#ifdef TIMER_DX
bool milk2_ui_element::on_idle()
{
    Tick();
    return true;
}
#endif

void milk2_ui_element::notify(const GUID& p_what, size_t p_param1, const void* p_param2, size_t p_param2size)
{
    if (p_what == ui_element_notify_colors_changed || p_what == ui_element_notify_font_changed)
        Invalidate();
}

int milk2_ui_element::OnCreate(LPCREATESTRUCT cs)
{
    MILK2_CONSOLE_LOG("OnCreate ", cs->x, ", ", cs->y, ", ", GetWnd())
#if 0
    HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_PLUGIN_ICON));
    SetIcon(hIcon, FALSE); //SetClassLongPtr(get_wnd(), GCLP_HICON, (LONG_PTR)hIcon);
#endif
    if (!s_milk2)
    {
        ResolvePwd();
        s_config.init();
#ifdef TIMER_TP
        InitializeCriticalSection(&s_cs);
#endif
    }

    if (!m_milk2)
    {
        SetPwd(s_pwd);

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

#ifdef TIMER_TP
        m_tpTimer = CreateThreadpoolTimer(TimerCallback, this, NULL);
#endif
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
#ifdef TIMER_TP
    StopTimer();
    EnterCriticalSection(&s_cs);
#endif
    if (m_vis_stream.is_valid())
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
        s_was_topmost = false;
        s_milk2 = false;
#ifndef TIMER_TP
        KillTimer(ID_REFRESH_TIMER);
#else
        DeleteCriticalSection(&s_cs);
#endif
        wcscpy_s(s_config.settings.m_szPresetDir, g_plugin.GetPresetDir()); // save last "Load Preset" menu directory
        g_plugin.PluginQuit();
        //PostQuitMessage(0);
    }
    else
    {
        s_in_toggle = false;
    }
#ifdef TIMER_TP
    LeaveCriticalSection(&s_cs);
#endif
}

void milk2_ui_element::OnTimer(UINT_PTR nIDEvent)
{
    MILK2_CONSOLE_LOG_LIMIT("OnTimer ", GetWnd())
#ifndef TIMER_TP
    KillTimer(ID_REFRESH_TIMER);
    InvalidateRect(NULL, TRUE);
#endif
}

void milk2_ui_element::OnPaint(CDCHandle dc)
{
    MILK2_CONSOLE_LOG_LIMIT("OnPaint ", GetWnd())
    if (m_in_sizemove && m_milk2) // foobar2000 does not enter/exit size/move
    {
        Tick();
    }
    else
    {
        PAINTSTRUCT ps;
        std::ignore = BeginPaint(&ps);
        EndPaint(&ps);
    }
#ifdef TIMER_TP
    StartTimer();
#endif
    ValidateRect(NULL);
#ifndef TIMER_TP
    ULONGLONG now = GetTickCount64();
#endif
    if (m_vis_stream.is_valid())
    {
        BuildWaves();
#ifndef TIMER_TP
        ULONGLONG next_refresh = m_last_refresh + m_refresh_interval;
        // (next_refresh < now) would break when GetTickCount() overflows
        if (static_cast<LONGLONG>(next_refresh - now) < 0)
        {
            next_refresh = now;
        }
        SetTimer(ID_REFRESH_TIMER, static_cast<UINT>(next_refresh - now));
#endif
    }
#ifndef TIMER_TP
    m_last_refresh = now;
#endif
}

BOOL milk2_ui_element::OnEraseBkgnd(CDCHandle dc)
{
    MILK2_CONSOLE_LOG_LIMIT("OnEraseBkgnd ", GetWnd())
    ++s_count;

#if 0
    CRect r;
    WIN32_OP_D(GetClientRect(&r));
    CBrush brush;
    //const COLORREF rgbBlack = 0x00000000;
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
#ifndef TIMER_TP
    Tick();
#endif
#endif
#if 0
    FB2K_console_print(m_timer.GetFramesPerSecond(), "FPS");
#endif

    return TRUE;
}

void milk2_ui_element::OnMove(CPoint ptPos)
{
    MILK2_CONSOLE_LOG("OnMove ", GetWnd())
    if (m_milk2)
    {
#ifdef TIMER_TP
        if (TryEnterCriticalSection(&s_cs) == 0)
            return;
#endif
        g_plugin.OnWindowMoved();
#ifdef TIMER_TP
        LeaveCriticalSection(&s_cs);
#endif
    }
}

void milk2_ui_element::OnSize(UINT nType, CSize size)
{
    MILK2_CONSOLE_LOG("OnSize0 ", nType, ", ", size.cx, ", ", size.cy, ", ", GetWnd())
    if (nType == SIZE_MINIMIZED)
    {
        if (!m_minimized)
        {
            m_minimized = true;
            if (!m_in_suspend && m_milk2)
                OnSuspending();
            m_in_suspend = true;
        }
    }
    else if (m_minimized)
    {
        m_minimized = false;
        if (m_in_suspend && m_milk2)
            OnResuming();
        m_in_suspend = false;
    }
    else if (!m_in_sizemove && m_milk2)
    {
        int width = size.cx;
        int height = size.cy;

        if (!width || !height)
            return;
        if (width < 128)
            width = 128;
        if (height < 128)
            height = 128;
        MILK2_CONSOLE_LOG("OnSize1 ", nType, ", ", size.cx, ", ", size.cy, ", ", GetWnd())
#ifdef TIMER_TP
        if (TryEnterCriticalSection(&s_cs) == 0)
            return;
#endif
        g_plugin.OnWindowSizeChanged(size.cx, size.cy);
#ifdef TIMER_TP
        LeaveCriticalSection(&s_cs);
#endif
    }
}

void milk2_ui_element::OnEnterSizeMove()
{
    MILK2_CONSOLE_LOG("OnEnterSizeMove ", GetWnd())
    m_in_sizemove = true;
}

void milk2_ui_element::OnExitSizeMove()
{
    MILK2_CONSOLE_LOG("OnExitSizeMove ", GetWnd())
    m_in_sizemove = false;
    if (m_milk2)
    {
#ifdef TIMER_TP
        if (TryEnterCriticalSection(&s_cs) == 0)
            return;
#endif
        RECT rc;
        WIN32_OP_D(GetClientRect(&rc));
        g_plugin.OnWindowSizeChanged(rc.right - rc.left, rc.bottom - rc.top);
#ifdef TIMER_TP
        LeaveCriticalSection(&s_cs);
#endif
    }
}

// To avoid a 1-pixel-thick border of noise.
LRESULT milk2_ui_element::OnNcCalcSize(BOOL bCalcValidRects, LPARAM lParam)
{
    MILK2_CONSOLE_LOG("OnNcCalcSize ", GetWnd())
    if (bCalcValidRects == TRUE)
    {
        auto& params = *reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
        //adjust_fullscreen_client_rect(get_wnd(), params.rgrc[0]);
        return 0;
    }
    return 0;
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
            if (!m_in_suspend && m_milk2)
                OnSuspending();
            m_in_suspend = true;
            break;
        case PBT_APMRESUMESUSPEND:
            if (!m_minimized)
            {
                if (m_in_suspend && m_milk2)
                    OnResuming();
                m_in_suspend = false;
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

#pragma region Keyboard Controls
#include "keyboard.cpp"
#pragma endregion

void milk2_ui_element::OnContextMenu(CWindow wnd, CPoint point)
{
    MILK2_CONSOLE_LOG("OnContextMenu ", point.x, ", ", point.y, ", ", GetWnd())
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
    WIN32_OP_D(menu.CreatePopupMenu()); // ID_VIS_MENU
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
    menu.AppendMenu(MF_STRING, IDM_SHOW_PREFS, TEXT("Launch Preferences Page"));
    menu.AppendMenu(MF_STRING | (g_plugin.m_show_help ? MF_CHECKED : 0), IDM_SHOW_HELP, TEXT("Show Help"));
    menu.AppendMenu(MF_STRING | (g_plugin.m_show_playlist ? MF_CHECKED : 0), IDM_SHOW_PLAYLIST, TEXT("Show Playlist"));
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING | (s_fullscreen ? MF_CHECKED : 0), IDM_TOGGLE_FULLSCREEN, TEXT("Fullscreen"));

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
        case IDM_SHOW_PREFS:
            ShowPreferencesPage();
            break;
        case IDM_SHOW_HELP:
            ToggleHelp();
            break;
        case IDM_SHOW_PLAYLIST:
            TogglePlaylist();
            break;
        case IDM_QUIT:
            //g_plugin.m_exiting = 1;
            //PostMessage(WM_CLOSE, static_cast<WPARAM>(0), static_cast<LPARAM>(0));
            break;
    }

    Invalidate();
}

LRESULT milk2_ui_element::OnImeNotify(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    MILK2_CONSOLE_LOG("OnImeNotify ", GetWnd())
    if (uMsg != WM_IME_NOTIFY)
        return 1;
    if (wParam == IMN_CLOSESTATUSWINDOW)
    {
        if (s_fullscreen && !s_in_toggle)
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
        return HIWORD(lParam) == IDS_MILKDROP_ERROR || HIWORD(lParam) == IDS_MILKDROP_WARNING ? 0 : 1;
    }
    else if (lParam == IPC_GETVERSION)
    {
        MILK2_CONSOLE_LOG("IPC_GETVERSION")
        //const t_core_version_data v = core_version_info_v2::get_version();
        return 1;
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
        SetSelectionSingle(static_cast<size_t>(g_plugin.m_playlist_pos));
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
    else if (lParam == IPC_GETPLAYLISTTITLEW || lParam == IPC_GET_PLAYING_TITLE)
    {
        //MILK2_CONSOLE_LOG(IPC_GETPLAYLISTTITLEW ? "IPC_GETPLAYLISTTITLEW" : "IPC_GET_PLAYING_TITLE")
        titleformat_object::ptr title_format;
        if (lParam == IPC_GETPLAYLISTTITLEW && m_script.is_empty())
        {
            pfc::string8 pattern = pfc::utf8FromWide(s_config.settings.m_szTitleFormat);
            static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(m_script, pattern);
        }
        if (lParam == IPC_GET_PLAYING_TITLE && m_title.is_empty())
        {
            pfc::string8 pattern = default_szTitleFormat;
            static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(m_title, pattern);
        }
        pfc::string_formatter state;
        metadb_handle_list list;
        api->activeplaylist_get_all_items(list);
        if (wParam == -1 || !(list.get_item(static_cast<size_t>(wParam)))->format_title(NULL, state, IPC_GETPLAYLISTTITLEW ? m_script : m_title, NULL))
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
        //MILK2_CONSOLE_LOG("IPC_GETPLUGINDIRECTORYW")
        m_szBuffer = s_config.settings.m_szPluginsDirPath;
        return reinterpret_cast<LRESULT>(m_szBuffer.c_str());
    }
    else if (lParam == IPC_GETINIDIRECTORYW)
    {
        //MILK2_CONSOLE_LOG("IPC_GETINIDIRECTORYW")
        m_szBuffer = s_config.settings.m_szConfigIniFile;
        size_t p = m_szBuffer.find_last_of(L"\\");
        if (p != std::wstring::npos)
            m_szBuffer = m_szBuffer.substr(0, p + 1);
        return reinterpret_cast<LRESULT>(m_szBuffer.c_str());
    }
    else if (lParam == IPC_FETCH_ALBUMART)
    {
        MILK2_CONSOLE_LOG("IPC_FETCH_ALBUMART")
        if (m_art_file.empty())
        {
            m_art_data->imgData = m_raster.data();
            m_art_data->imgDataLen = static_cast<int>(m_raster.size());
            m_art_data->type[0] = L'j'; m_art_data->type[1] = L'p'; m_art_data->type[2] = L'g'; m_art_data->type[3] = L'\0';
            m_art_data->gracenoteFileId = nullptr;
        }
        else
        {
            m_art_data->imgData = nullptr;
            m_art_data->imgDataLen = 0;
            m_art_data->gracenoteFileId = m_art_file.data();
            std::wstring ext = GetExtension(m_art_file);
            std::copy_n(ext.begin(), ext.length(), &m_art_data->type[0]);
            std::fill_n(&m_art_data->type[0] + ext.length() + 1, 10 - ext.length(), L'\0');
        }
        s_config.settings.m_artData = m_art_data.get();

        return reinterpret_cast<LRESULT>(m_art_data.get());
    }
#if 0
    else if (lParam == IPC_SETVISWND)
    {
        //MILK2_CONSOLE_LOG("IPC_SETVISWND")
        //SendMessage(m_hwnd_winamp, WM_WA_IPC, (WPARAM)m_hwnd, IPC_SETVISWND);
    }
    else if (lParam == IPC_CB_VISRANDOM)
    {
        //MILK2_CONSOLE_LOG("IPC_CB_VISRANDOM")
        //SendMessage(GetWinampWindow(), WM_WA_IPC, (m_bPresetLockOnAtStartup ? 0 : 1) << 16, IPC_CB_VISRANDOM);
    }
    else if (lParam == IPC_IS_PLAYING_VIDEO)
    {
        //MILK2_CONSOLE_LOG("IPC_IS_PLAYING_VIDEO")
        //if (m_screenmode == FULLSCREEN && SendMessage(GetWinampWindow(), WM_WA_IPC, 0, IPC_IS_PLAYING_VIDEO) > 1)
    }
    else if (lParam == IPC_SET_VIS_FS_FLAG)
    {
        //MILK2_CONSOLE_LOG("IPC_SET_VIS_FS_FLAG")
        //SendMessage(GetWinampWindow(), WM_WA_IPC, 1, IPC_SET_VIS_FS_FLAG);
    }
    else if (lParam == IPC_GET_D3DX9)
    {
        //MILK2_CONSOLE_LOG("IPC_GET_D3DX9")
        //HMODULE d3dx9 = (HMODULE)SendMessage(winamp, WM_WA_IPC, 0, IPC_GET_D3DX9);
    }
    else if (lParam == IPC_GET_API_SERVICE)
    {
        //MILK2_CONSOLE_LOG("IPC_GET_API_SERVICE")
        //WASABI_API_SVC = (api_service*)SendMessage(hwndParent, WM_WA_IPC, 0, IPC_GET_API_SERVICE);
    }
    else if (lParam == IPC_GETDIALOGBOXPARENT)
    {
        //MILK2_CONSOLE_LOG("IPC_GETDIALOGBOXPARENT")
        //HWND parent = (HWND)SendMessage(winamp, WM_WA_IPC, 0, IPC_GETDIALOGBOXPARENT);
    }
    else if (lParam == IPC_GET_RANDFUNC)
    {
        //MILK2_CONSOLE_LOG("IPC_GET_RANDFUNC")
        //int warand();
        //warand = (int (*)(void))SendMessage(hwndParent, WM_WA_IPC, 0, IPC_GET_RANDFUNC);
    }
#endif

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
                m_script.reset();
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
        if (FALSE == g_plugin.PluginPreInitialize(window, core_api::get_my_instance()))
            return false;
        if (!g_plugin.PanelSettings(&s_config.settings))
            return false;

        //swprintf_s(g_plugin.m_szPluginsDirPath, L"%ls", s_config.settings.m_szPluginsDirPath);
        //swprintf_s(g_plugin.m_szConfigIniFile, L"%ls", s_config.settings.m_szConfigIniFile);
        swprintf_s(g_plugin.m_szComponentDirPath, L"%ls", const_cast<wchar_t*>(m_pwd.c_str()));

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
#ifdef TIMER_TP
        EnterCriticalSection(&s_cs);
#endif
        g_plugin.OnWindowSwap(window, width, height);
#ifdef TIMER_TP
        LeaveCriticalSection(&s_cs);
#endif
    }

    m_milk2 = true;

    return true;
}

#pragma region Frame Update
// Executes the render.
void milk2_ui_element::Tick()
{
#ifdef TIMER_TP
    if (TryEnterCriticalSection(&s_cs) == 0)
        return;
#endif

    m_timer.Tick([&]() { Update(m_timer); });

    Render();

#ifdef TIMER_TP
    LeaveCriticalSection(&s_cs);

    InvalidateRect(NULL, TRUE);
#endif
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

    return g_plugin.PluginRender(waves[0].data(), waves[1].data());
}

// Clears the back buffers and the window contents.
void milk2_ui_element::Clear()
{
#if 0
    HDC hdc = GetDC();
    RECT rect{};
    GetClientRect(&rect);
    FillRect(hdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));
    ReleaseDC(hdc);
#endif
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
        for (uint32_t i = 0; i < static_cast<uint32_t>(NUM_AUDIO_BUFFER_SAMPLES); ++i)
        {
            waves[0][i] = waves[1][i] = 0U;
        }
        return;
    }
    auto count = chunk.get_sample_count();
    auto sample_rate = chunk.get_srate();
    auto channels = chunk.get_channel_count();
    audio_sample* audio_data = chunk.get_data();

    size_t top = std::min(count / channels, static_cast<size_t>(NUM_AUDIO_BUFFER_SAMPLES));
    for (size_t i = 0; i < top; ++i)
    {
        waves[0][i] = static_cast<float>(audio_data[i * channels] * 128.0f);
        if (channels >= 2)
            waves[1][i] = static_cast<float>(audio_data[i * channels + 1] * 128.0f);
        else
            waves[1][i] = waves[0][i];
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

void milk2_ui_element::SetPwd(std::wstring pwd) noexcept
{
    m_pwd.assign(pwd);
}

void milk2_ui_element::ToggleFullScreen()
{
    MILK2_CONSOLE_LOG("ToggleFullScreen0 ", GetWnd())
    if (m_milk2)
    {
        if (!s_fullscreen)
        {
            g_hWindow = get_wnd();
            m_milk2 = false;
        }
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
        s_in_toggle = true;
        SetTopMost();
        static_api_ptr_t<ui_element_common_methods_v2>()->toggle_fullscreen(g_get_guid(), core_api::get_main_window());
        MILK2_CONSOLE_LOG("ToggleFullScreen1 ", GetWnd())
    }
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
#ifdef _DEBUG
    wchar_t buf[512]{};
    swprintf_s(buf, L"%s", (g_plugin.m_nLoadingPreset != 0) ? g_plugin.m_pNewState->m_szDesc : g_plugin.m_pState->m_szDesc);
    wcscat_s(buf, L".milk");
    if (buf == g_plugin.m_presets[g_plugin.m_nCurrentPreset].szFilename)
    {
        MILK2_CONSOLE_LOG("GetCurrentPreset Mismatch --> ", buf, " != ", g_plugin.m_presets[g_plugin.m_nCurrentPreset].szFilename.c_str())
    }
#endif
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

void milk2_ui_element::Seek(UINT nRepCnt, bool bShiftHeldDown, double seekDelta)
{
    int reps = (bShiftHeldDown) ? 6 * nRepCnt : 1 * nRepCnt;
    if (seekDelta > 0.0)
    {
        if (!m_playback_control->playback_can_seek())
        {
            m_playback_control->next();
            return;
        }
    }
    else
    {
        if (!m_playback_control->playback_can_seek())
        {
            m_playback_control->previous();
            return;
        }
        double p = m_playback_control->playback_get_position();
        if (p < 0.0)
            return;
        if (p < (seekDelta * -1.0))
        {
            if (p < (seekDelta * -1.0) / 3.0)
            {
                if (m_playback_control->is_playing() && p > (seekDelta * -1.0))
                    m_playback_control->playback_seek(0.0);
                else
                    m_playback_control->previous();
                return;
            }
            else
            {
                m_playback_control->playback_seek(0.0);
                return;
            }
        }
    }
    for (int i = 0; i < reps; ++i)
        m_playback_control->playback_seek_delta(seekDelta);
}

// clang-format off
void milk2_ui_element::UpdateChannelMode()
{
    if (m_vis_stream.is_valid())
    {
        m_vis_stream->set_channel_mode(s_config.settings.m_bEnableDownmix ? visualisation_stream_v3::channel_mode_mono : visualisation_stream_v3::channel_mode_default);
    }
}
// clang-format on

void milk2_ui_element::UpdateTrack()
{
    if (m_script.is_empty())
    {
        pfc::string8 pattern = pfc::utf8FromWide(s_config.settings.m_szTitleFormat);
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

#ifdef TIMER_TP
// Starts the timer.
void milk2_ui_element::StartTimer() noexcept
{
    MILK2_CONSOLE_LOG_LIMIT("StartTimer ", GetWnd())
    if (m_tpTimer == nullptr)
        return;

    FILETIME DueTime{};
    DWORD RefreshInterval = static_cast<DWORD>(lround(1000.0f / s_config.settings.m_max_fps_fs));
    SetThreadpoolTimer(m_tpTimer, &DueTime, RefreshInterval, 0);
}

// Stops the timer.
void milk2_ui_element::StopTimer() noexcept
{
    MILK2_CONSOLE_LOG_LIMIT("StopTimer ", GetWnd())
    if (m_tpTimer == nullptr)
        return;

    SetThreadpoolTimer(m_tpTimer, NULL, 0, 0);
    WaitForThreadpoolTimerCallbacks(m_tpTimer, TRUE);
    CloseThreadpoolTimer(m_tpTimer);
}

// Handles a timer tick.
VOID CALLBACK milk2_ui_element::TimerCallback(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_TIMER Timer) noexcept
{
    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Timer);

    ((milk2_ui_element*)Context)->Tick();
}
#endif

#ifdef TIMER_DX
bool milk2_ui_element::on_idle()
{
    Tick();
    //MILK2_CONSOLE_LOG("on_idle ", GetWnd())
    return true;
}
#endif

// Sets and unsets foobar2000's "Always on Top" setting (if main window is
// `TOPMOST`) so that visualization window becomes `TOPMOST` on entering
// fullscreen.
// Note: Using `SetWindowPos` does not work because foobar2000 appears to
//       reset itself to `TOPMOST` forcefully when this setting is enabled.
//       So unsetting and resetting the program's setting is a workaround.
bool milk2_ui_element::SetTopMost() noexcept
{
    LONG_PTR ptr = ::GetWindowLongPtr(core_api::get_main_window(), GWL_EXSTYLE);
    bool topmost = static_cast<bool>(ptr & WS_EX_TOPMOST);
    if (topmost)
    {
        if (s_fullscreen)
        {
            MILK2_CONSOLE_LOG("SetTopMost unsetting main window ", GetWnd())
            standard_commands::main_always_on_top(); //::SetWindowPos(get_wnd(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW /*| SWP_NOZORDER | SWP_FRAMECHANGED*/);
            s_was_topmost = true;
            return true;
        }
    }
    else
    {
        if (s_was_topmost)
        {
            MILK2_CONSOLE_LOG("SetTopMost resetting main window ", GetWnd())
            s_was_topmost = false;
            standard_commands::main_always_on_top(); //::SetWindowPos(get_wnd(), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW /*| SWP_NOZORDER | SWP_FRAMECHANGED*/);
            return true;
        }
    }
    return false;
}

void milk2_ui_element::ShowPreferencesPage()
{
    ui_control::get()->show_preferences(guid_milk2_preferences);
}

void milk2_ui_element::SetSelectionSingle(size_t idx)
{
    SetSelectionSingle(idx, false, true, true);
}

void milk2_ui_element::SetSelectionSingle(size_t idx, bool toggle, bool focus, bool single_only)
{
    auto api = playlist_manager::get();
    const size_t total = api->activeplaylist_get_item_count();
    const size_t idx_focus = api->activeplaylist_get_focus_item();
    //if (idx_focus == pfc::infinite_size)
    //    return;

    bit_array_bittable mask(total);
    mask.set(idx, toggle ? !api->activeplaylist_is_item_selected(idx) : true);

    if (single_only || toggle || !api->activeplaylist_is_item_selected(idx))
        api->activeplaylist_set_selection(single_only ? (pfc::bit_array&)pfc::bit_array_true() : (pfc::bit_array&)pfc::bit_array_one(idx), mask);
    if (focus && idx_focus != idx)
        api->activeplaylist_set_focus_item(idx);
}

// Resolves PWD, taking care of the case where the path contains non-ASCII
// characters.
void milk2_ui_element::ResolvePwd()
{
    // Get profile directory path through foobar2000 API.
    pfc::string8 full_path = filesystem::g_get_native_path(core_api::get_my_full_path());
    size_t t = full_path.lastIndexOf(L'\\');
    if (t != SIZE_MAX)
        full_path = full_path.subString(0, t + 1);
    size_t path_length = full_path.get_length();
    std::wstring base_path(path_length, L'\0');
    path_length = pfc::stringcvt::convert_utf8_to_wide(const_cast<wchar_t*>(base_path.c_str()), path_length, full_path.get_ptr(), path_length);
    base_path = base_path.erase(base_path.find(L'\0'));

#if 0
    // Get PWD path through Win32 API.
    wchar_t path[MAX_PATH];
    HMODULE hm = NULL;
    if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          (LPCWSTR)&TEXT(APPLICATION_FILE_NAME),
                          &hm) == 0)
    {
        DWORD ret = GetLastError();
        MILK2_CONSOLE_LOG("GetModuleHandleEx failed, error = %d\n", ret);
    }
    if (GetModuleFileName(hm, path, MAX_PATH) == 0)
    {
        DWORD ret = GetLastError();
        MILK2_CONSOLE_LOG("GetModuleFileName failed, error = %d\n", ret);
    }
    std::wstring paths(path);
    size_t p = paths.rfind(L'\\');
    if (p != std::wstring::npos)
        paths.erase(p + 1);

    // Use Win32 string it mismatches with the foobar2000 string.
    if (paths != base_path)
    {
        s_pwd = paths;
    }
    else
    {
        s_pwd = base_path;
    }
#else
    s_pwd = base_path;
#endif
}

// Retrieves image raster data and clears the file path.
void milk2_ui_element::ExtractRasterData(const uint8_t* data, size_t size) noexcept
{
    m_art_file.clear();
    std::vector<uint8_t> empty;
    m_raster.swap(empty);
    if ((data != nullptr) && (size != 0))
    {
        m_raster.assign(data, data + size);
    }
}

// Registers with the album art notifier.
void milk2_ui_element::RegisterForArtwork()
{
#if 0
    // Register with the album art notification manager.
    auto AlbumArtNotificationManager = now_playing_album_art_notify_manager_v2::tryGet();

    if (AlbumArtNotificationManager.is_valid())
        AlbumArtNotificationManager->add(this);

    // Get the artwork data from the album art.
    if (wcsnlen_s(s_config.settings.m_szArtworkFormat, 256) == 0)
    {
        auto aanm = now_playing_album_art_notify_manager_v2::get();

        if (aanm != nullptr)
        {
            album_art_data_ptr aad = aanm->current();

            if (aad.is_valid())
            {
                ExtractRasterData(static_cast<const uint8_t*>(aad->data()), aad->size());
            }
        }
    }
#endif
}

// Loads embedded album art.
void milk2_ui_element::LoadAlbumArt(const metadb_handle_ptr& track, abort_callback& abort)
{
    static_api_ptr_t<album_art_manager_v2> aam;

    auto extractor = aam->open(pfc::list_single_ref_t(track), pfc::list_single_ref_t(album_art_ids::cover_front), abort);

    try
    {
        auto aad = extractor->query(album_art_ids::cover_front, abort);

        if (aad.is_valid())
        {
            ExtractRasterData(static_cast<const uint8_t*>(aad->data()), aad->size());
        }
    }
    catch (const exception_album_art_not_found&)
    {
        return;
    }
    catch (const exception_aborted&)
    {
        throw;
    }
    catch (...)
    {
        return;
    }
}

// Service factory publishes the class.
static service_factory_single_t<ui_element_milk2> g_ui_element_milk2_factory;
#pragma endregion

#pragma region Initialize/Quit
class milk2_initquit : public initquit
{
  public:
    void on_init()
    {
        MILK2_CONSOLE_LOG("on_init")
        if (core_api::is_quiet_mode_enabled())
            return;
    }
    void on_quit() { MILK2_CONSOLE_LOG("on_quit") }
};

FB2K_SERVICE_FACTORY(milk2_initquit);
#pragma endregion
} // namespace