/*
 * extrusionvs.hlsl - .
 *
 * Copyright (c) Microsoft Corporation
 * SPDX-License-Identifier: MIT
 */

#include "extrusion.hlsli"

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;
    float4 worldPos = mul(float4(input.Pos, 1), World);
    float4 cameraPos = mul(worldPos, View);

    output.WorldPos = worldPos.xyz;
    output.WorldNorm = normalize(mul(input.Norm, (float3x3)World));
    output.CameraPos = cameraPos.xyz;
    output.Pos = mul(cameraPos, Projection);

    return output;
}
