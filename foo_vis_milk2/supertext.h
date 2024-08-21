/*
 * supertext.h - .
 *
 * Copyright (c) Microsoft Corporation
 * SPDX-License-Identifier: MIT
 */

#pragma once

// C RunTime Header Files
#include <algorithm>
#include <stdexcept>
#include <vector>

#include <DirectXMath.h>
#include <d3dx11effect.h>

#include "d3dmath.h"
#include "resource.h"

struct SimpleVertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT3 Norm;
};

class SuperText
{
  public:
    SuperText(
        ID2D1Factory1* pD2DFactory,
        IDWriteFactory1* pDWriteFactory,
        ID3D11Device1* pDevice,
        ID3D11DeviceContext1* pContext
    );
    ~SuperText();

    HRESULT CreateDeviceIndependentResources();
    HRESULT CreateDeviceDependentResources();
    void SetSwapChain(IDXGISwapChain1* pSwapChain) { m_pSwapChain = pSwapChain; }
    void SetDepthStencilView(ID3D11DepthStencilView* pDepthStencilView) { pDepthStencilView = pDepthStencilView; }
    void SetRenderTargetView(ID3D11RenderTargetView* pRenderTargetView) { m_pRenderTargetView = pRenderTargetView; }
    HRESULT CreateWindowSizeDependentResources(int nWidth, int nHeight);
    void DiscardDeviceResources();
    HRESULT OnRender();

  private:
    void OnChar(SHORT key);
    HRESULT GenerateTextOutline(bool includeCursor, ID2D1Geometry** ppGeometry);
    HRESULT LoadResourceShader(ID3D11Device* pDevice, PCWSTR pszResource, ID3DX11Effect** ppEffect);
    HRESULT UpdateTextGeometry();

    // Device-Independent Resources
    ID2D1Factory1* m_pD2DFactory;
    IDWriteFactory1* m_pDWriteFactory;
    ID2D1Geometry* m_pTextGeometry;
    IDWriteTextLayout* m_pTextLayout;

    // Device-Dependent Resources
    ID3D11Device1* m_pDevice;
    ID3D11DeviceContext1* m_pContext;
    IDXGISwapChain1* m_pSwapChain;
    ID3D11RasterizerState* m_pState;
    ID3D11Texture2D* m_pDepthStencil;
    ID3D11DepthStencilView* m_pDepthStencilView;
    ID3D11RenderTargetView* m_pRenderTargetView;
    ID3DX11Effect* m_pShader;
    ID3D11Buffer* m_pVertexBuffer;
    ID3D11InputLayout* m_pVertexLayout;

    ID3DX11EffectTechnique* m_pTechniqueNoRef;
    ID3DX11EffectMatrixVariable* m_pWorldVariableNoRef;
    ID3DX11EffectMatrixVariable* m_pViewVariableNoRef;
    ID3DX11EffectMatrixVariable* m_pProjectionVariableNoRef;

    ID3DX11EffectVectorVariable* m_pLightPosVariableNoRef;
    ID3DX11EffectVectorVariable* m_pLightColorVariableNoRef;

    DirectX::XMFLOAT4X4 m_WorldMatrix;
    DirectX::XMFLOAT4X4 m_ViewMatrix;
    DirectX::XMFLOAT4X4 m_ProjectionMatrix;

    static const D3D11_INPUT_ELEMENT_DESC s_InputLayout[];
    static const UINT sc_vertexBufferCount;

    std::vector<WCHAR> m_characters;
};