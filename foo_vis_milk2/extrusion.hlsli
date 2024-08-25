/*
 * extrusion.hlsi - .
 *
 * Copyright (c) Microsoft Corporation
 * SPDX-License-Identifier: MIT
 */

SamplerState samLinear : register(s0);

cbuffer cbNeverChanges : register(b0)
{
    matrix View;
    float4 vLightPos[1];
    float4 vLightColor[1];
}

cbuffer cbChangeOnResize : register(b1)
{
    matrix Projection;
};

cbuffer cbChangesEveryFrame : register(b2)
{
    matrix World;
};

struct VS_INPUT
{
    float3 Pos : POSITION;
    float3 Norm : NORMAL;
};

struct PS_INPUT
{
    float4 Pos : SV_Position;
    float3 WorldNorm : TEXCOORD0;
    float3 CameraPos : TEXCOORD1;
    float3 WorldPos : TEXCOORD2;
};
