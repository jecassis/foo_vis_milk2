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

#define NOBITMAP
#define NOMINMAX
#define NOMCX
#define NOSERVICE

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <helpers/foobar2000-lite+atl.h>
//#include <sdk/component.h>

class foobar2000_component_globals
{
  public:
    foobar2000_component_globals()
    {
#if defined(_MSC_VER) && !defined(_DEBUG) && !defined(_DLL)
        ::OverrideCrtAbort();
#endif
    }
};

class NOVTABLE foobar2000_client
{
  public:
    typedef service_factory_base* pservice_factory_base;

    enum
    {
        FOOBAR2000_CLIENT_VERSION_COMPATIBLE = FOOBAR2000_TARGET_VERSION_COMPATIBLE,
        FOOBAR2000_CLIENT_VERSION = FOOBAR2000_TARGET_VERSION,
    };
    virtual t_uint32 get_version() = 0;
    virtual pservice_factory_base get_service_list() = 0;

    virtual void get_config(stream_writer* p_stream, abort_callback& p_abort) = 0;
    virtual void set_config(stream_reader* p_stream, abort_callback& p_abort) = 0;
    virtual void set_library_path(const char* path, const char* name) = 0;
    virtual void services_init(bool val) = 0;
    virtual bool is_debug() = 0;

  protected:
    foobar2000_client() {}
    ~foobar2000_client() {}
};

class NOVTABLE foobar2000_api
{
  public:
    virtual service_class_ref service_enum_find_class(const GUID& p_guid) = 0;
    virtual bool service_enum_create(service_ptr_t<service_base>& p_out, service_class_ref p_class, t_size p_index) = 0;
    virtual t_size service_enum_get_count(service_class_ref p_class) = 0;
    virtual fb2k::hwnd_t get_main_window() = 0;
    virtual bool assert_main_thread() = 0;
    virtual bool is_main_thread() = 0;
    virtual bool is_shutting_down() = 0;
    virtual const char* get_profile_path() = 0;
    virtual bool is_initializing() = 0;

    //New in 0.9.6
    virtual bool is_portable_mode_enabled() = 0;
    virtual bool is_quiet_mode_enabled() = 0;

  protected:
    foobar2000_api() {}
    ~foobar2000_api() {}
};

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
    service_class_ref service_enum_find_class(const GUID& p_guid) { return 0; }
    bool service_enum_create(service_ptr_t<service_base>& p_out, service_class_ref p_class, t_size p_index) { return false; }
    t_size service_enum_get_count(service_class_ref p_class) { return 0; }
    fb2k::hwnd_t get_main_window() { return NULL; };
    bool assert_main_thread() { return false; }
    bool is_main_thread() { return false; }
    bool is_shutting_down() { return false; }
    const char* get_profile_path() { return ""; }
    bool is_initializing() { return false; }
    bool is_portable_mode_enabled() { return false; }
    bool is_quiet_mode_enabled() { return false; }
};