#include <AOShaderHeader.hlsli>

float SsaoPs(Output input) : SV_TARGET
{
    return depthtex.Sample(smp, input.uv);
}