#pragma once

// DirectX apps do not need GDI
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP

#define NOMCX
#define NOSERVICE

// WinHelp is deprecated
//#define NOHELP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <wrl/client.h>

#include <d3d11_1.h>
#include <dxgi1_6.h>

#include <algorithm>
#include <exception>
#include <memory>
#include <stdexcept>

#include <stdio.h>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

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
