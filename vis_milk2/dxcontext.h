/*
  LICENSE
  -------
  Copyright 2005-2013 Nullsoft, Inc.
  Copyright 2021-2024 Jimmy Cassis
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the name of Nullsoft nor the names of its contributors may be used to
      endorse or promote products derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
  FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __NULLSOFT_DX_PLUGIN_SHELL_DXCONTEXT_H__
#define __NULLSOFT_DX_PLUGIN_SHELL_DXCONTEXT_H__

#include "shell_defines.h"
#include "deviceresources.h"
#include "d3d11shim.h"

#define DX_ERR_REGWIN -2
#define DX_ERR_CREATEWIN -3
#define DX_ERR_CREATE3D -4
#define DX_ERR_GETFORMAT -5
#define DX_ERR_FORMAT -6
#define DX_ERR_CREATEDEV_PROBABLY_OUTOFVIDEOMEMORY -7
#define DX_ERR_RESIZEFAILED -8
#define DX_ERR_CAPSFAIL -9
#define DX_ERR_BAD_FS_DISPLAYMODE -10
#define DX_ERR_USER_CANCELED -11
#define DX_ERR_CREATEDEV_NOT_AVAIL -12
#define DX_ERR_CREATEDDRAW -13
#define DX_ERR_SWAPFAIL -14

typedef struct _DXCONTEXT_PARAMS
{
    unsigned int allow_page_tearing;
    unsigned int enable_hdr;
    DXGI_FORMAT back_buffer_format;
    DXGI_FORMAT depth_buffer_format;
    UINT back_buffer_count;
    DXGI_SAMPLE_DESC msaa;
    D3D_FEATURE_LEVEL min_feature_level;
    LUID adapter_guid;
    wchar_t adapter_devicename[256];
    eScrMode screenmode; // WINDOWED, DESKTOP, FULLSCREEN, or FAKE FULLSCREEN
    int m_skin;
    HWND parent_window;
} DXCONTEXT_PARAMS;

class DXContext final
{
  public:
    DXContext(HWND hWndWinamp, DXCONTEXT_PARAMS* pParams) noexcept(false);
    ~DXContext();

    BOOL StartOrRestartDevice(DXCONTEXT_PARAMS* pParams); // also serves as Init() function
    BOOL OnWindowSizeChanged(int width, int height);
    BOOL OnWindowSwap(HWND window, int width, int height);
    void OnWindowMoved();
    void OnDisplayChange();
    inline HWND GetHwnd() const { return m_hwnd; };
    void Show();
    void Clear();
    void RestoreTarget();
    int GetBitDepth() const { return m_bpp; };

    void CreateDeviceIndependentResources();
    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    ID3D11Device1* GetD3DDevice() const noexcept { return m_deviceResources->GetD3DDevice(); }
    DXGI_FORMAT GetBackBufferFormat() const noexcept { return m_deviceResources->GetBackBufferFormat(); }
    ID2D1Factory1* GetD2DFactory() const noexcept { return m_deviceResources->GetD2DFactory(); }
    ID2D1Device* GetD2DDevice() const noexcept { return m_deviceResources->GetD2DDevice(); }
    ID2D1DeviceContext* GetD2DDeviceContext() const noexcept { return m_deviceResources->GetD2DDeviceContext(); }
    IDWriteFactory1* GetDWriteFactory() const noexcept { return m_deviceResources->GetDWriteFactory(); }
    DX::DeviceResources* GetDeviceResources() const noexcept { return m_deviceResources.get(); }

    // DO NOT WRITE TO THESE FROM OUTSIDE THE CLASS
    int m_ready;
    HRESULT m_lastErr;
    int m_client_width;
    int m_client_height;
    int m_frame_delay;
    DXCONTEXT_PARAMS m_current_mode;
    std::unique_ptr<D3D11Shim> m_lpDevice;

  protected:
    HWND m_hwnd;
    int m_bpp;

    BOOL Internal_Init(DXCONTEXT_PARAMS* pParams, BOOL bFirstInit);
    void Internal_CleanUp();

    std::unique_ptr<DX::DeviceResources> m_deviceResources;
};

#endif