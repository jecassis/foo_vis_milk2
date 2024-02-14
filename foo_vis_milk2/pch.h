/*
 *  pch.h - Pre-compiled header file.
 */

#pragma once

#include <winsdkver.h>
#ifndef _DEBUG
//#define _WIN32_WINNT 0x0600 // Windows Vista
#define _WIN32_WINNT 0x0601 // Windows 7
//#define _WIN32_WINNT 0x0602 // Windows 8
#else
#define _WIN32_WINNT 0x0603 // Windows 8.1
#endif

//#define NODRAWTEXT // `DrawText()` and `DT_*` definitions --> From "WinUser.h"
//#define NOGDI // All GDI defines and routines --> From "wingdi.h"
#define NOBITMAP // Extended bitmap info header definition --> From "mmreg.h

#define NOMINMAX // Macros `min(a,b)` and `max(a,b)` --> Use the C++ standard templated min/max
#define NOMCX // Modem Configuration Extensions --> From "mcx.h"
#define NOSERVICE // All Service Controller routines, `SERVICE_*` equates, etc... --> From "winsvc.h"
//#define NOHELP // Help engine interface (WinHelp) --> [deprecated]

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <helpers/foobar2000-lite+atl.h>
#include <sdk/componentversion.h>
#include <sdk/coreversion.h>
#include <sdk/preferences_page.h>
#include <sdk/cfg_var.h>
#include <sdk/advconfig_impl.h>
#include <sdk/audio_chunk.h>
#include <sdk/console.h>
#include <sdk/initquit.h>
#include <sdk/playback_control.h>
#include <sdk/play_callback.h>
#include <sdk/playlist.h>
#include <sdk/vis.h>
#include <sdk/ui_element.h>
#include <helpers/atl-misc.h>
#include <helpers/advconfig_impl.h>
#include <helpers/BumpableElem.h>
#include <helpers/DarkMode.h>
#include <pfc/string-conv-lite.h>
#include <libppui/win32_op.h>

#include <wrl/client.h>

#include <d3d11_1.h>
#include <dxgi1_6.h>
#include <d2d1_1.h>
#include <dwrite_1.h>
//#endif

#include <vis_milk2/defines.h>
#include <vis_milk2/md_defines.h>
#include <vis_milk2/shell_defines.h>
#include <vis_milk2/pluginshell.h>
#include <vis_milk2/plugin.h>
#include <vis_milk2/dxcontext.h>
#ifdef _DEBUG
#include <vis_milk2/utility.h>
#endif
#include <winamp/wa_ipc.h>

#define MAX_PROPERTY_PAGES 8
#define MAX_DISPLAY_ADAPTERS 16
#define MAX_MAX_FPS 144
#define MAX_DISPLAY_MODES 1024

#define WM_MILK2 (WM_USER + 0)
#define WM_CONFIG_CHANGE (WM_USER + 1)

#ifdef _DEBUG
#define MILK2_CONSOLE_LOG(...) FB2K_console_print(core_api::get_my_file_name(), ": ", __VA_ARGS__);
#define MILK2_CONSOLE_LOG_LIMIT(...) \
    if (s_count <= s_debug_limit) \
    { \
        FB2K_console_print(core_api::get_my_file_name(), ": ", __VA_ARGS__); \
    }
#else
#define MILK2_CONSOLE_LOG(...)
#define MILK2_CONSOLE_LOG_LIMIT(...)
#endif