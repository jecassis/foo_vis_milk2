/*
 *  constanttable.h - Direct3D 11 constant table header.
 *
 *  Copyright (c) 2021-2024 Jimmy Cassis
 *  SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <string>
#include <vector>
#include <d3d11_1.h>
#include <d3d11shader.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include "utility.h"

using namespace DirectX;

struct ShaderVariable
{
    D3D11_SHADER_VARIABLE_DESC Description;
    D3D11_SHADER_TYPE_DESC Type;
    bool IsDirty;
    void* Value;

    ShaderVariable() : Value(nullptr), IsDirty(false), Description{}, Type{} {}
};

struct ShaderBinding
{
    D3D11_SHADER_INPUT_BIND_DESC Description;
};

struct ShaderConstantBuffer
{
    ID3D11Buffer* Data;
    D3D11_SHADER_BUFFER_DESC Description;
    std::vector<ShaderVariable> Variables;

    bool Create(ID3D11Device* pDevice);
    bool HasChanges();

    ShaderConstantBuffer() : Data(nullptr), Description{} {}
    ~ShaderConstantBuffer() { Variables.clear(); }
};

class CConstantTable
{
  public:
    CConstantTable(ID3D11ShaderReflection* pReflection);
    ~CConstantTable();

    void AddRef() { m_iRefCount++; }

    void Release()
    {
        if (--m_iRefCount == 0)
            delete this;
    }

    bool GrabShaderData(ID3D11Device* pDevice);
    size_t GetVariablesCount();
    size_t GetBuffersCount() { return m_ConstantBuffers.size(); }
    void GetBuffers(ID3D11Buffer** ppBuffers);

    int GetTextureSlot(std::string& strName);

    bool SetVector(LPCSTR handle, XMFLOAT4* vector);
    bool SetMatrix(LPCSTR handle, XMMATRIX* matrix);
    bool ApplyChanges(ID3D11DeviceContext* pContext);

    ShaderVariable* GetVariableByIndex(size_t index);
    ShaderVariable* GetVariableByName(std::string& strName);
    ShaderBinding* GetBindingByIndex(UINT index);

    D3D_FEATURE_LEVEL MinimumFeatureLevel;
    D3D11_SHADER_DESC ShaderDesc;

  private:
    int m_iRefCount;
    ID3D11ShaderReflection* m_pReflection = NULL;
    std::vector<ShaderConstantBuffer> m_ConstantBuffers;
    std::vector<ShaderBinding> m_Bindings;
};