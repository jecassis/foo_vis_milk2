#pragma once

#include <winsdkver.h>
#define _WIN32_WINNT 0x0600 // Windows Vista

// DirectX apps do not need GDI
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP

#define NOMINMAX // Use the C++ standard templated min/max
#define NOMCX
#define NOSERVICE

// WinHelp is deprecated
//#define NOHELP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <process.h>

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
#include <cassert>
#include <clocale>
#include <cmath>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>

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