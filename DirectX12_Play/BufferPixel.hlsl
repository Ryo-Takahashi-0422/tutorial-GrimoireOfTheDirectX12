#include "BufferHeader.hlsli"

float4 psBuffer(Output input) : SV_TARGET
{
    //return float4(input.uv, 1.0f, 1.0f);
    return model.Sample(smp, input.uv) + tex.Sample(smp, input.uv);
}