//
// pch.h: Pre-compiled header file.
//

#pragma once

#include <winsdkver.h>
#define _WIN32_WINNT 0x0601 // Windows 7

// DirectX apps do not need GDI
//#define NODRAWTEXT
//#define NOGDI
#define NOBITMAP

#define NOMINMAX // Use the C++ standard templated min/max
#define NOMCX
#define NOSERVICE

// WinHelp is deprecated
//#define NOHELP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <helpers/foobar2000+atl.h>
#include <helpers/BumpableElem.h>
#include <libppui/win32_op.h>

#include <wrl/client.h>

#include <d3d11_1.h>
#include <dxgi1_6.h>
#include <wrl/client.h>