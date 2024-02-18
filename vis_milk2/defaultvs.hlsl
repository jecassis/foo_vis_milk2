/*
 *  defaultvs.hlsl - Default vertex shader.
 *
 *  Copyright (c) 2021-2024 Jimmy Cassis
 *  SPDX-License-Identifier: BSD-3-Clause
 */

cbuffer cbTransforms : register(b0)
{
    float4x4 g_world;
    float4x4 g_view;
    float4x4 g_proj;
};

struct VS_INPUT
{
    float4 vPosition : POSITION;
    float4 Diffuse   : COLOR;
    float4 Tex0      : TEXCOORD0;
    float2 Tex1      : TEXCOORD1;
};

struct VS_OUTPUT
{
    float4 Diffuse   : COLOR;
    float4 Tex0      : TEXCOORD0;
    float2 Tex1      : TEXCOORD1;
    float4 vPosition : SV_POSITION;
};

VS_OUTPUT main(VS_INPUT In)
{
    VS_OUTPUT output;

    float4 worldPos = mul(In.vPosition, g_world);
    float4 cameraPos = mul(worldPos, g_view);
    output.vPosition = mul(cameraPos, g_proj);
    output.Diffuse = saturate(In.Diffuse);
    output.Tex0 = In.Tex0;
    output.Tex1 = 0.0;

    return output;
}