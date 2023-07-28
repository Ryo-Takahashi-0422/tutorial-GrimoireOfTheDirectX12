#include "BufferHeader.hlsli"

Output vsBuffer(float4 pos : POSITION,/* float4 norm : NORMAL, */float2 uv : TEXCOORD/*, min16uint2 boneno : BONE_NO, min16uint weight : WEIGHT*/)
{
    Output output;
    output.svpos = pos;
    output.uv = uv;
    output.tpos = mul(lightCamera, pos);
    //output.norm = norm;
    return output;
}
