Texture2D    g_Texture0 : register(t0);
SamplerState g_Sampler0 : register(s0);

struct PS_INPUT
{
    float4 Diffuse : COLOR;
    float4 Tex0    : TEXCOORD0;
    float2 Tex1    : TEXCOORD1;
};

float4 main(PS_INPUT In) : SV_TARGET
{
    return float4(g_Texture0.Sample(g_Sampler0, In.Tex0.xy).rgb, In.Diffuse.a);
}