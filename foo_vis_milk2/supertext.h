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

#include <DirectXMath.h>
#include <d3dx11effect.h>
#include <atlsimpcoll.h>

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
    SuperText();
    ~SuperText();

    //HRESULT Initialize();

    //void RunMessageLoop();

  private:
    HRESULT CreateDeviceIndependentResources();
    HRESULT CreateDeviceResources();
    void DiscardDeviceResources();

    HRESULT OnRender();

    //void OnTimer();

    //void OnChar(SHORT vkey);

    void GetHardwareAdapter(IDXGIAdapter1** ppAdapter);
    HRESULT CreateD3DDevice(IDXGIAdapter1* pAdapter, UINT Flags, ID3D11Device** ppDevice, ID3D11DeviceContext** ppContext);

    HRESULT GenerateTextOutline(bool includeCursor, ID2D1Geometry** ppGeometry);

    //static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    HRESULT LoadResourceShader(ID3D11Device* pDevice, PCWSTR pszResource, ID3DX11Effect** ppEffect);

    HRESULT UpdateTextGeometry();

  private:
    HWND m_hwnd;
    ID2D1Factory* m_pD2DFactory;
    IDWriteFactory* m_pDWriteFactory;

    // Device-Dependent Resources
    Microsoft::WRL::ComPtr<IDXGIFactory2> m_dxgiFactory;
    ID3D11Device* m_pDevice;
    ID3D11DeviceContext* m_pContext;
    IDXGISwapChain* m_pSwapChain;
    IDXGIFactory6* m_pFactory;
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

    // Device-Independent Resources
    ID2D1Geometry* m_pTextGeometry;
    IDWriteTextLayout* m_pTextLayout;

    DirectX::XMFLOAT4X4 m_WorldMatrix;
    DirectX::XMFLOAT4X4 m_ViewMatrix;
    DirectX::XMFLOAT4X4 m_ProjectionMatrix;

    static const D3D11_INPUT_ELEMENT_DESC s_InputLayout[];
    static const UINT sc_vertexBufferCount;

    ATL::CSimpleArray<WCHAR> m_characters;
};