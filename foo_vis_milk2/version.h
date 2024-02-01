//
// version.h: Defines common version information for the DLL's version resource
//            and the component version shown in foobar2000.
//            Note: Newline at EOF required for resource compiler (RC1004).
//

#pragma once

//#include <winver.h>
#include <vis_milk2/defines.h>

#define APPLICATION_FILE_NAME "foo_vis_milk2"
#define APPLICATION_PRODUCT_NAME SHORTNAME
#define APPLICATION_COMPANY_NAME "KernelOops"
#define APPLICATION_FILE_IS_DLL 1
#define APPLICATION_DESCRIPTION "MilkDrop 2 Visualization Library"
#define APPLICATION_SOURCE_URL "https: //github.com/jecassis/foo_vis_milk2"

#define APPLICATION_VERSION_MAJOR 0
#define APPLICATION_VERSION_MINOR 0
#define APPLICATION_VERSION_BUILD INT_VERSION
#define APPLICATION_VERSION_REVISION -2 // -2: alpha, -1: beta, >= 0: not development version

#define LIBRARY_VERSION_MAJOR 2 // INT_VERSION / 100
#define LIBRARY_VERSION_MINOR 25 // INT_VERSION % 100
#define LIBRARY_VERSION_REVISION INT_SUBVERSION
#define LIBRARY_VERSION_BUILD 0

#define APPLICATION_COPYRIGHT "2024 " APPLICATION_COMPANY_NAME
#define LIBRARY_COPYRIGHT COPYRIGHT

#if !defined(APPLICATION_VERSION_MAJOR) || APPLICATION_VERSION_MAJOR < 0 || APPLICATION_VERSION_MAJOR > 32768
#error Invalid APPLICATION_VERSION_MAJOR
#endif

#if !defined(APPLICATION_VERSION_MINOR) || APPLICATION_VERSION_MINOR < 0 || APPLICATION_VERSION_MINOR > 32768
#error Invalid APPLICATION_VERSION_MINOR
#endif

#if !defined(APPLICATION_VERSION_BUILD) || APPLICATION_VERSION_BUILD < 0 || APPLICATION_VERSION_BUILD > 32768
#error Invalid APPLICATION_VERSION_BUILD
#endif

#if !defined(APPLICATION_VERSION_REVISION) || APPLICATION_VERSION_REVISION < -2 || APPLICATION_VERSION_REVISION > 32768
#error Invalid APPLICATION_VERSION_REVISION
#endif

#ifndef APPLICATION_FILE_NAME
#error Expected APPLICATION_FILE_NAME
#endif

#if !(defined(APPLICATION_FILE_IS_DLL) ^ defined(APPLICATION_FILE_IS_EXE))
#error Expected either APPLICATION_FILE_IS_DLL or APPLICATION_FILE_IS_EXE to be defined
#endif

#ifndef APPLICATION_PRODUCT_NAME
#error Expected APPLICATION_PRODUCT_NAME
#endif

#ifndef APPLICATION_COMPANY_NAME
#error Expected APPLICATION_COMPANY_NAME
#endif

#ifndef APPLICATION_COPYRIGHT
#error Expected APPLICATION_COPYRIGHT
#endif

#ifdef APPLICATION_FILE_IS_DLL
#define _APPLICATION_VERSION_EXTENSION ".dll"
#define _APPLICATION_VERSION_FILETYPE VFT_DLL // 0x2L
#endif

#ifdef APPLICATION_FILE_IS_EXE
#define _APPLICATION_VERSION_EXTENSION ".exe"
#define _APPLICATION_VERSION_FILETYPE VFT_APP // 0x1L
#endif

#if APPLICATION_VERSION_REVISION < 0
#ifdef _DEBUG
#define _APPLICATION_VERSION_FILEFLAGS VS_FF_DEBUG | VS_FF_PRERELEASE // 0x3L
#else
#define _APPLICATION_VERSION_FILEFLAGS VS_FF_PRERELEASE // 0x2L
#endif
#else
#ifdef _DEBUG
#define _APPLICATION_VERSION_FILEFLAGS VS_FF_DEBUG // 0x1L
#else
#define _APPLICATION_VERSION_FILEFLAGS 0x0L
#endif
#endif

#ifndef STR
#define STR_QUOTE(x) #x
#define STR(x) STR_QUOTE(x)
#endif

#if APPLICATION_VERSION_REVISION == -2
#undef APPLICATION_VERSION_REVISION
#define APPLICATION_VERSION_REVISION -1
#define APPLICATION_VERSION STR(APPLICATION_VERSION_MAJOR) "." STR(APPLICATION_VERSION_MINOR) "." STR(APPLICATION_VERSION_BUILD) "-alpha"
#elif APPLICATION_VERSION_REVISION == -1
#undef APPLICATION_VERSION_REVISION
#define APPLICATION_VERSION_REVISION -2
#define APPLICATION_VERSION STR(APPLICATION_VERSION_MAJOR) "." STR(APPLICATION_VERSION_MINOR) "." STR(APPLICATION_VERSION_BUILD) "-beta"
#else
#define APPLICATION_VERSION STR(APPLICATION_VERSION_MAJOR) "." STR(APPLICATION_VERSION_MINOR) "." STR(APPLICATION_VERSION_BUILD) "." STR(APPLICATION_VERSION_REVISION)
#endif
#define LIBRARY_VERSION STR(LIBRARY_VERSION_MAJOR) "." STR(LIBRARY_VERSION_MINOR) "." STR(LIBRARY_VERSION_REVISION) "." STR(LIBRARY_VERSION_BUILD)
