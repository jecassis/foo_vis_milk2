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

DXContext::DXContext(HWND hWndWinamp /*, wchar_t* szIniFile*/) noexcept(false)
{
    m_hwnd = hWndWinamp;
    m_bpp = 0;
    m_frame_delay = 0;
    //wcscpy_s(m_szIniFile, szIniFile);

    // Clear the error register.
    m_lastErr = S_OK;

    // Clear the active flag.
    m_ready = FALSE;
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

BOOL DXContext::Internal_Init(DXCONTEXT_PARAMS* pParams, BOOL bFirstInit)
{
    memcpy_s(&m_current_mode, sizeof(m_current_mode), pParams, sizeof(DXCONTEXT_PARAMS));

    // Screen mode check.
    if (m_current_mode.screenmode != WINDOWED)
        m_current_mode.m_skin = 0;

    if (bFirstInit)
    {
        // Create the device.
        // Provide parameters for swap chain format, depth/stencil format, and back buffer count.
        m_deviceResources = std::make_unique<DX::DeviceResources>(
            DXGI_FORMAT_B8G8R8A8_UNORM, // backBufferFormat
            DXGI_FORMAT_D24_UNORM_S8_UINT, // depthBufferFormat
            m_current_mode.nbackbuf, // backBufferCount
            D3D_FEATURE_LEVEL_9_1, // minFeatureLevel
            DX::DeviceResources::c_FlipPresent |
                ((m_current_mode.allow_page_tearing << 1) & DX::DeviceResources::c_AllowTearing) |
                ((m_current_mode.enable_hdr << 2) & DX::DeviceResources::c_EnableHDR) // flags
        );
        m_deviceResources->RegisterDeviceNotify(this);
        m_bpp = 32;

        m_deviceResources->SetWindow(m_hwnd, m_current_mode.display_mode.Width, m_current_mode.display_mode.Height);

        //m_deviceResources->CreateDeviceIndependentResources();
        m_deviceResources->CreateDeviceResources();
        CreateDeviceDependentResources();

        //m_deviceResources->SetDpi(96.0f);
        m_deviceResources->CreateWindowSizeDependentResources();
        CreateWindowSizeDependentResources();
    }

    return m_ready;
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

// Call this to [re]initialize the DirectX environment with new parameters.
// Examples: startup; toggle windowed/fullscreen mode; change fullscreen resolution;
//           and so on.
// Clean up all the DirectX collateral first (textures, vertex buffers,
// D3DX allocations, etc...) and reallocate it afterwards!
// Note: for windowed mode, `pParams->disp_mode` (w/h/r/f) is ignored.
BOOL DXContext::StartOrRestartDevice(DXCONTEXT_PARAMS* pParams)
{
    if (!m_ready)
    {
        // First time init: create a fresh new device.
        return Internal_Init(pParams, TRUE);
    }
    else
    {
        // Re-init: preserve the DX11 object (m_lpD3D),
        // but destroy and re-create the DX11 device (m_lpDevice).
        m_ready = FALSE;

        //SafeRelease(m_lpDevice);

        // But leave the D3D object!
        //RestoreWinamp();
        return Internal_Init(pParams, FALSE);
    }
}

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void DXContext::CreateDeviceDependentResources()
{
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto device = m_deviceResources->GetD3DDevice();

    m_lpDevice = std::make_unique<D3D11Shim>(device, context);
    m_lpDevice->Initialize();

    //m_states = std::make_unique<CommonStates>(device);

    //m_fxFactory = std::make_unique<EffectFactory>(device);
    //m_fxFactory->SetDirectory(std::wstring(m_pwd.begin(), m_pwd.end()).c_str());

    //m_sprites = std::make_unique<SpriteBatch>(context);

    //m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(context);

    //m_batchEffect = std::make_unique<BasicEffect>(device);
    //m_batchEffect->SetVertexColorEnabled(true);

    //DX::ThrowIfFailed(CreateInputLayoutFromEffect<VertexPositionColor>(device, m_batchEffect.get(),
    //                                                                   m_batchInputLayout.ReleaseAndGetAddressOf()));

    //std::string spritefont = m_pwd + "SegoeUI_18.spritefont";
    //std::string sdkmesh = m_pwd + "tiny.sdkmesh";
    //std::string dds1 = m_pwd + "seafloor.dds";
    //std::string dds2 = m_pwd + "windowslogo.dds";

    //m_font = std::make_unique<SpriteFont>(device, std::wstring(spritefont.begin(), spritefont.end()).c_str());

    //m_shape = GeometricPrimitive::CreateTeapot(context, 4.f, 8);

    //// SDKMESH has to use clockwise winding with right-handed coordinates, so textures are flipped in U
    //m_model = Model::CreateFromSDKMESH(device, std::wstring(sdkmesh.begin(), sdkmesh.end()).c_str(), *m_fxFactory);

    //// Load textures
    //DX::ThrowIfFailed(CreateDDSTextureFromFile(device, std::wstring(dds1.begin(), dds1.end()).c_str(), nullptr,
    //                                           m_texture1.ReleaseAndGetAddressOf()));

    //DX::ThrowIfFailed(CreateDDSTextureFromFile(device, std::wstring(dds2.begin(), dds2.end()).c_str(), nullptr,
    //                                           m_texture2.ReleaseAndGetAddressOf()));
}

// Allocate all memory resources that change on a window SizeChanged event.
void DXContext::CreateWindowSizeDependentResources()
{
    //auto const size = m_deviceResources->GetOutputSize();
    //const float aspectRatio = float(size.right) / float(size.bottom);
    //float fovAngleY = 70.0f * XM_PI / 180.0f;

    //// This is a simple example of change that can be made when the app is in
    //// portrait or snapped view.
    //if (aspectRatio < 1.0f)
    //{
    //    fovAngleY *= 2.0f;
    //}

    //// This sample makes use of a right-handed coordinate system using row-major matrices.
    //m_projection = Matrix::CreatePerspectiveFieldOfView(fovAngleY, aspectRatio, 0.01f, 100.0f);

    //m_batchEffect->SetProjection(m_projection);
}

void DXContext::OnDeviceLost()
{
    //m_states.reset();
    //m_fxFactory.reset();
    //m_sprites.reset();
    //m_batch.reset();
    //m_batchEffect.reset();
    //m_font.reset();
    //m_shape.reset();
    //m_model.reset();
    //m_texture1.Reset();
    //m_texture2.Reset();
    //m_batchInputLayout.Reset();
}

void DXContext::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
