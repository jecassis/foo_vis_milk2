/*
 * extrusionps.hlsl - Interactive 3-D text pixel shader.
 *
 * Copyright (c) Microsoft Corporation
 * SPDX-License-Identifier: MIT
 */

#include "extrusion.fxh"

struct ColorsOutput
{
    float4 Diffuse;
    float4 Specular;
};

ColorsOutput CalcLighting(float3 worldNormal, float3 worldPos, float3 cameraPos)
{
    ColorsOutput output = (ColorsOutput)0.0;
    
    float specularBrightness = 0.8;
    float shininess = 60;
    float4 specular = float4(specularBrightness * float3(1, 1, 1), 1);

    float ambient = 0.5f;

    float3 lightDir = normalize(vLightPos[0].xyz - worldPos);

    output.Diffuse += min(max(0, dot(lightDir, worldNormal)) + ambient, 1) * vLightColor[0];

    float3 halfAngle = normalize(normalize(cameraPos) + lightDir);
    output.Specular += max(0, pow(abs(max(0, dot(halfAngle, worldNormal))), shininess) * specular);
    
    return output;
}

float4 main(PS_INPUT input) : SV_Target
{
    //float4 finalColor = {1.0f, 0.0f, 0.0f, 1.0f};
    float4 finalColor = 0;
    
    ColorsOutput cOut = CalcLighting(input.WorldNorm, input.WorldPos, input.CameraPos);

    finalColor += cOut.Diffuse + cOut.Specular;

    finalColor.a = 1;
    return finalColor;
}
