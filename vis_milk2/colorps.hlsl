/*
 *  colorps.hlsl - Color pixel shader.
 *
 *  Copyright (c) 2021-2024 Jimmy Cassis
 *  SPDX-License-Identifier: BSD-3-Clause
 */

struct PS_INPUT
{
    float4 Diffuse : COLOR;
    float4 Tex0    : TEXCOORD0;
    float2 Tex1    : TEXCOORD1;
};

float4 main(PS_INPUT In) : SV_TARGET
{
    return In.Diffuse;
}