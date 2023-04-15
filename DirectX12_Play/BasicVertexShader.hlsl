#include "BasicShaderHeader.hlsli"

Output BasicVS
(float4 pos : POSITION,
float4 norm : NORMAL,
float2 uv : TEXCOORD
//min16uint2 boneno : BONE_NO,
//min16uint weight : WEIGHT
)
{
    Output output; // �s�N�Z���V�F�[�_�[�ɓn���l
    output.svpos = mul(mul(viewproj, world), pos);
    norm.w = 0; // world�ɕ��s�ړ��������܂܂�Ă���ꍇ�A�@�������s�ړ�����B(���̎����f���͈Â��Ȃ�B�Ȃ��H�H)
    output.norm = mul(world, norm);
    output.uv = uv;
    
	return output;
}