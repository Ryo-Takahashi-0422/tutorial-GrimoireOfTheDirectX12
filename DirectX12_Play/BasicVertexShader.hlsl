#include "BasicShaderHeader.hlsli"

Output BasicVS
(float4 pos : POSITION,
float4 norm : NORMAL,
float2 uv : TEXCOORD
//min16uint2 boneno : BONE_NO,
//min16uint weight : WEIGHT
)
{
    Output output; // ピクセルシェーダーに渡す値
    output.svpos = mul(mul(viewproj, world), pos);
    norm.w = 0; // worldに平行移動成分が含まれている場合、法線が並行移動する。(この時モデルは暗くなる。なぜ？？)
    output.norm = mul(world, norm);
    output.uv = uv;
    
	return output;
}