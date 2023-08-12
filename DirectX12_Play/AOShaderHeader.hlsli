Texture2D<float4> depthtex : register(t0); // depthmap
Texture2D<float4> normtex : register(t1); // normalmap

SamplerState smp : register(s0); // sampler

struct Output
{
    float4 svpos : SV_POSITION;
    float4 norm : NORMAL0;
    float2 uv : TEXCOORD;
    float4 tpos : TPOS;
};