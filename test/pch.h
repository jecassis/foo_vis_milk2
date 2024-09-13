/*
 * pch.h - Pre-compiled header file.
 *
 * Copyright (c) 2023-2024 Jimmy Cassis
 * SPDX-License-Identifier: MPL-2.0
 */

#pragma once

#include <winsdkver.h>
#define _WIN32_WINNT 0x0601 // Windows 7
#if defined(_M_ARM64) || defined(_M_ARM64EC)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00 // Windows 10
#endif
#include <sdkddkver.h>

#define _USE_MATH_DEFINES
#define NOSYSMETRICS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define OEMRESOURCE
#define NOATOM
#define NOKERNEL
#define NOMEMMGR
#define NOMINMAX
#define NOOPENFILE
#define NOSERVICE
#define NOSOUND
#define NOWH
#define NOCOMM
#define NOKANJI
#define NOPROFILER
#define NOMCX
#define WIN32_LEAN_AND_MEAN
#include <helpers/foobar2000-lite+atl.h>
#include <sdk/component.h>

#ifdef _WIN32
#define FOOBAR2000_CC __cdecl

#ifdef FOOBAR2000_EXPORTS
#define FOOBAR2000_API __declspec(dllexport) FOOBAR2000_CC
#else
#define FOOBAR2000_API __declspec(dllimport) FOOBAR2000_CC
#endif

#ifdef __cplusplus
extern "C" {
#endif
//__declspec(dllexport) foobar2000_client * _cdecl foobar2000_get_interface(foobar2000_api * p_api,HINSTANCE hIns)
typedef foobar2000_client* p_foobar2000_client;
p_foobar2000_client FOOBAR2000_API foobar2000_get_interface(foobar2000_api* p_api, HINSTANCE hIns);
#ifdef __cplusplus
}
#endif
#endif

class NOVTABLE foobar2000_api_impl : public foobar2000_api
{
  public:
    service_class_ref service_enum_find_class(const GUID& p_guid) { return reinterpret_cast<void*>(static_cast<size_t>(rand())); }
    bool service_enum_create(service_ptr_t<service_base>& p_out, service_class_ref p_class, t_size p_index) { return false; }
    t_size service_enum_get_count(service_class_ref p_class) { return 5; }
    fb2k::hwnd_t get_main_window() { return NULL; };
    bool assert_main_thread() { return false; }
    bool is_main_thread() { return false; }
    bool is_shutting_down() { return false; }
    const char* get_profile_path() { return ""; }
    bool is_initializing() { return false; }
    bool is_portable_mode_enabled() { return false; }
    bool is_quiet_mode_enabled() { return false; }
};

namespace core_api
{
bool are_services_available()
{
    return true;
}

bool is_main_thread()
{
    return true;
}
}