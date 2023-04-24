/*
  LICENSE
  -------
  Copyright 2005-2013 Nullsoft, Inc.
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

#include "pch.h"
#include "dxcontext.h"

#include <strsafe.h>
#include <DirectXColors.h>

#include "shell_defines.h"
#include "utility.h"

DXContext::DXContext(HWND hWndWinamp, DXCONTEXT_PARAMS* pParams, wchar_t* szIniFile) noexcept(false)
{
    m_hwnd = hWndWinamp;
    m_bpp = 32;
    m_frame_delay = 0;
    wcscpy_s(m_szIniFile, szIniFile);
    memcpy_s(&m_current_mode, sizeof(m_current_mode), pParams, sizeof(DXCONTEXT_PARAMS));

    // Clear the error register.
    m_lastErr = S_OK;

    // Clear the active flag.
    m_ready = FALSE;

    // Create the device.
    // Provide parameters for swap chain format, depth/stencil format, and back buffer count.
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_B8G8R8A8_UNORM, // backBufferFormat
        DXGI_FORMAT_D24_UNORM_S8_UINT, // depthBufferFormat
        m_current_mode.nbackbuf, // backBufferCount
        D3D_FEATURE_LEVEL_9_1, // minFeatureLevel
        DX::DeviceResources::c_FlipPresent | ((m_current_mode.allow_page_tearing << 1) & DX::DeviceResources::c_AllowTearing) |
            ((m_current_mode.enable_hdr << 2) & DX::DeviceResources::c_EnableHDR) // flags
    );
    m_deviceResources->RegisterDeviceNotify(this);
}

DXContext::~DXContext()
{
    Internal_CleanUp();
}

void DXContext::Internal_CleanUp()
{
    // Clear active flag.
    m_ready = FALSE;

    // Release 3D interfaces.
    if (m_lpDevice)
        m_lpDevice.reset();
}

// {0000000A-000C-0010-FF7B-01014263450C}
const GUID avs_guid = {
    10, 12, 16, {255, 123, 1, 1, 66, 99, 69, 12}
};

BOOL DXContext::Internal_Init(BOOL /* bFirstInit */)
{
    // Screen mode check.
    if (m_current_mode.screenmode != WINDOWED)
        m_current_mode.m_skin = 0;

    RECT r;
    GetClientRect(m_hwnd, &r);
    m_client_width = std::max(1l, r.right - r.left);
    m_client_height = std::max(1l, r.bottom - r.top);
    m_REAL_client_width = std::max(1l, r.right - r.left);
    m_REAL_client_height = std::max(1l, r.bottom - r.top);

    m_deviceResources->SetWindow(m_hwnd, m_client_width, m_client_height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    m_ready = TRUE;
    return TRUE;
}

// Display the swap chain contents to the screen.
void DXContext::Show()
{
    m_deviceResources->Present();
}

void DXContext::Clear()
{
    // Clear the views
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    context->ClearRenderTargetView(renderTarget, Colors::CornflowerBlue);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    // Set the viewport.
    auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);
}

void DXContext::RestoreTarget()
{
    //auto context = m_deviceResources->GetD3DDeviceContext();
    auto rt = m_deviceResources->GetRenderTarget();
    m_lpDevice->SetRenderTarget(rt);
}

// Call this to [re]initialize the DirectX environment with new parameters.
// Examples: startup; toggle windowed/fullscreen mode; change fullscreen resolution;
//           and so on.
// Clean up all the DirectX collateral first (textures, vertex buffers,
// D3DX allocations, etc...) and reallocate it afterwards!
// Note: for windowed mode, `pParams->disp_mode` (w/h/r/f) is ignored.
BOOL DXContext::StartOrRestartDevice()
{
    if (!m_ready)
    {
        // First time init: create a fresh new device.
        return Internal_Init(TRUE);
    }
    else
    {
        // Re-init: preserve the DX11 object (m_lpD3D),
        // but destroy and re-create the DX11 device (m_lpDevice).
        m_ready = FALSE;

        //SafeRelease(m_lpDevice);

        // But leave the D3D object!
        //RestoreWinamp();
        return Internal_Init(FALSE);
    }
}

// Call this function on `WM_EXITSIZEMOVE` when running windowed.
// Do not call when fullscreen. Clean up all the DirectX stuff
// first (textures, vertex buffers, etc...) and reallocate it
// afterwards!
BOOL DXContext::OnUserResizeWindow(RECT* new_client_rect)
{
    if (!m_ready)// || (m_current_mode.screenmode != WINDOWED))
        return FALSE;

    if ((m_client_width == new_client_rect->right - new_client_rect->left) &&
        (m_client_height == new_client_rect->bottom - new_client_rect->top))
    {
        return TRUE;
    }

    m_ready = FALSE;

    m_client_width = new_client_rect->right - new_client_rect->left;
    m_client_height = new_client_rect->bottom - new_client_rect->top;
    m_REAL_client_width = new_client_rect->right - new_client_rect->left;
    m_REAL_client_height = new_client_rect->bottom - new_client_rect->top;

    if (!m_deviceResources->WindowSizeChanged(m_client_width, m_client_height))
    {
        m_lastErr = DXC_ERR_RESIZEFAILED;
        return FALSE;
    }

    CreateWindowSizeDependentResources();

    m_ready = TRUE;
    return TRUE;
}

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void DXContext::CreateDeviceDependentResources()
{
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto device = m_deviceResources->GetD3DDevice();

    m_lpDevice = std::make_unique<D3D11Shim>(device, context);
    m_lpDevice->Initialize();
}

// Allocate all memory resources that change on a window SizeChanged event.
void DXContext::CreateWindowSizeDependentResources()
{
}

void DXContext::OnDeviceLost()
{
}

void DXContext::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion