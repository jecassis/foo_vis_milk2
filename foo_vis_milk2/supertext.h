/*
 * supertext.h - Interactive 3-D text header file.
 *
 * Copyright (c) Microsoft Corporation
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

#include <DirectXMath.h>
#include <wrl/client.h>
#include <vis_milk2/dxcontext.h>

struct ConstantBufferNeverChanges
{
    DirectX::XMFLOAT4X4 View;
    DirectX::XMFLOAT4 LightPosition[1];
    DirectX::XMFLOAT4 LightColor[1];
};

struct ConstantBufferChangeOnResize
{
    DirectX::XMFLOAT4X4 Projection;
};

struct ConstantBufferChangesEveryFrame
{
    DirectX::XMFLOAT4X4 World;
};

struct PNVertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT3 Norm;
};

class SuperText
{
  public:
    SuperText(DXContext* lpDX);
    ~SuperText();

    HRESULT CreateDeviceIndependentResources(ID2D1Factory1* pD2DFactory, IDWriteFactory1* pDWriteFactory);
    HRESULT CreateDeviceDependentResources(ID3D11Device1* pDevice, ID3D11DeviceContext1* pContext);
    HRESULT CreateWindowSizeDependentResources(int nWidth, int nHeight);
    void DiscardDeviceResources();
    void SetSwapChain(IDXGISwapChain1* pSwapChain) { m_pSwapChain = pSwapChain; }
    void SetDepthStencilView(ID3D11DepthStencilView* pDepthStencilView) { m_pDepthStencilView = pDepthStencilView; }
    void SetRenderTargetView(ID3D11RenderTargetView* pRenderTargetView) { m_pRenderTargetView = pRenderTargetView; }
    HRESULT SetTextFont(const std::wstring& str, const PCWSTR face = L"Gabriola", float size = 96.0f);
    HRESULT OnRender();

  private:
    HRESULT GenerateTextOutline(bool includeCursor, ID2D1Geometry** ppGeometry);
    HRESULT UpdateTextGeometry();
    //void OnChar(SHORT key);

    // Device-Dependent Resources
    Microsoft::WRL::ComPtr<ID3D11Device> m_pDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_pContext;
    Microsoft::WRL::ComPtr<IDXGISwapChain> m_pSwapChain;
    Microsoft::WRL::ComPtr<IDXGIFactory2> m_pDxgiFactory;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_pState;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_pDepthStencil;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_pDepthStencilView;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_pRenderTargetView;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_pVertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_pVertexLayout;

    // Device-Independent Resources
    Microsoft::WRL::ComPtr<ID2D1Factory> m_pD2DFactory;
    Microsoft::WRL::ComPtr<IDWriteFactory> m_pDWriteFactory;
    ID2D1Geometry* m_pTextGeometry;
    IDWriteTextLayout* m_pTextLayout;

    DirectX::XMFLOAT4X4 m_WorldMatrix;
    DirectX::XMFLOAT4X4 m_ViewMatrix;
    DirectX::XMFLOAT4X4 m_ProjectionMatrix;

    // Constant Buffers
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_constantBufferNeverChanges;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_constantBufferChangeOnResize;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_constantBufferChangesEveryFrame;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_samplerLinear;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;

    static const D3D11_INPUT_ELEMENT_DESC sc_PNVertexLayout[];
    static const UINT sc_vertexBufferCount;

    std::wstring m_characters;
    PCWSTR m_fontFace;
    float m_fontSize;
};