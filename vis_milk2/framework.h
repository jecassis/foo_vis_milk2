#pragma once

#include <winsdkver.h>
#ifndef _DEBUG
//#define _WIN32_WINNT 0x0600 // Windows Vista
#define _WIN32_WINNT 0x0601 // Windows 7
//#define _WIN32_WINNT 0x0602 // Windows 8
#else
#define _WIN32_WINNT 0x0603 // Windows 8.1
#endif
#if defined(_M_ARM64) || defined(_M_ARM64EC)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00 // Windows 10
#endif

// DirectX apps do not need GDI
#define NODRAWTEXT // `DrawText()` and `DT_*` definitions --> From "WinUser.h"
#define NOGDI // All GDI defines and routines --> From "wingdi.h"
#define NOBITMAP // Extended bitmap info header definition --> From "mmreg.h

#define NOMINMAX // Macros `min(a,b)` and `max(a,b)` --> Use the C++ standard templated min/max
#define NOMCX // Modem Configuration Extensions --> From "mcx.h"
#define NOSERVICE // All Service Controller routines, `SERVICE_*` equates, etc... --> From "winsvc.h"
#define NOHELP // Help engine interface (WinHelp) --> [deprecated]

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <process.h>
//#include <shellapi.h>

#include <wrl/client.h>

#include <d3d11_1.h>
#include <d3d11shader.h>
#include <dxgi1_6.h>
#ifdef _DEBUG
#include <dxgidebug.h>
#endif
#include <d3dcompiler.h>

#include <DirectXColors.h>
#include <DirectXMath.h>

#include <algorithm>
//#include <atomic>
#include <cassert>
#include <clocale>
#include <cmath>
//#include <cstddef>
//#include <cstdint>
//#include <cstdio>
//#include <cstring>
//#include <cwchar>
#include <exception>
//#include <iterator>
#include <list>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
//#include <tuple>
#include <vector>

#include <eel2/ns-eel.h>
#include <foo_vis_milk2/resource.h>

#include <CommonStates.h>
#include <DDSTextureLoader.h>
#include <DirectXHelpers.h>
#include <WICTextureLoader.h>

namespace DX
{
// Helper class for COM exceptions.
class com_exception : public std::exception
{
  public:
    com_exception(HRESULT hr) noexcept : result(hr) {}

    const char* what() const override
    {
        static char s_str[64] = {};
        sprintf_s(s_str, "Failure with HRESULT of %08X", static_cast<unsigned int>(result));
        return s_str;
    }

  private:
    HRESULT result;
};

// Helper utility converts D3D API failures into exceptions.
inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw com_exception(hr);
    }
}
} // namespace DX