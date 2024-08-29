/*
 * pch.h - Pre-compiled header file.
 *
 * Copyright (c) 2023-2024 Jimmy Cassis
 * SPDX-License-Identifier: MPL-2.0
 */

#pragma once

#include <winsdkver.h>
//#define _WIN32_WINNT 0x0600 // Windows Vista
#define _WIN32_WINNT 0x0601 // Windows 7
//#define _WIN32_WINNT 0x0602 // Windows 8
//#define _WIN32_WINNT 0x0603 // Windows 8.1
#if defined(_M_ARM64) || defined(_M_ARM64EC)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00 // Windows 10
#endif
#include <sdkddkver.h>

//#define NODRAWTEXT // `DrawText()` and `DT_*` definitions --> From "WinUser.h"
//#define NOGDI // All GDI defines and routines --> From "wingdi.h"
#define NOBITMAP // Extended bitmap info header definition --> From "mmreg.h

#define NOMINMAX // Macros `min(a,b)` and `max(a,b)` --> Use the C++ standard templated min/max
#define NOMCX // Modem Configuration Extensions --> From "mcx.h"
#define NOSERVICE // All Service Controller routines, `SERVICE_*` equates, etc... --> From "winsvc.h"
//#define NOHELP // Help engine interface (WinHelp) --> [deprecated]

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <d3d11_1.h> // Windows 8 and Platform Update for Windows 7 [desktop apps | UWP apps]
#include <dxgi1_6.h> // Windows 10, version 1803 [desktop apps only]
#include <d2d1_1.h> // Windows 8 and Platform Update for Windows 7 [desktop apps | UWP apps]
#include <dwrite_1.h> // Windows 8 and Platform Update for Windows 7 [desktop apps | UWP apps]

#include <wrl/client.h>

#include <array>

#include <helpers/foobar2000-lite+atl.h>
#include <sdk/componentversion.h>
#include <sdk/coreversion.h>
#include <sdk/preferences_page.h>
#include <sdk/cfg_var.h>
#include <sdk/advconfig_impl.h>
#include <sdk/album_art.h>
#include <sdk/audio_chunk.h>
#include <sdk/console.h>
#include <sdk/initquit.h>
#include <sdk/playback_control.h>
#include <sdk/play_callback.h>
#include <sdk/playlist.h>
#include <sdk/vis.h>
#include <sdk/ui_element.h>
#include <sdk/menu_helpers.h>
#include <helpers/atl-misc.h>
#include <helpers/advconfig_impl.h>
#include <helpers/BumpableElem.h>
#include <helpers/DarkMode.h>
#include <pfc/string-conv-lite.h>

#include <vis_milk2/defines.h>
#include <vis_milk2/md_defines.h>
#include <vis_milk2/shell_defines.h>
#define WASABI_API_ORIG_HINST core_api::get_my_instance()
#include <vis_milk2/api.h>
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

// Macros
template <class T>
inline void SafeReleaseT(T** ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

template <class T, class U>
inline void SafeReplace(T** ppD, U* pS)
{
    if (*ppD)
    {
        (*ppD)->Release();
    }
    *ppD = pS;
    if (pS)
    {
        (*ppD)->AddRef();
    }
}

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif