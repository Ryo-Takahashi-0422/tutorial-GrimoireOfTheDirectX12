#include "BasicShaderHeader.hlsli"

Output BasicVS
(float4 pos : POSITION,
float4 norm : NORMAL,
float2 uv : TEXCOORD,
min16uint2 boneno : BONE_NO,
min16uint weight : WEIGHT)
{
    Output output; // �s�N�Z���V�F�[�_�[�ɓn���l
    float w = weight / 100.0f;
    matrix bm = bones[boneno[0]] * w + bones[boneno[1]] * (1 - w);
    pos = mul(bm, pos);
    output.svpos = mul(mul(mul(proj, view), world), pos);
    norm.w = 0; // world�ɕ��s�ړ��������܂܂�Ă���ꍇ�A�@�������s�ړ�����B(���̎����f���͈Â��Ȃ�B�Ȃ��H�H)
    output.norm = mul(world, norm);
    output.vnormal = mul(view, output.norm);
    output.uv = uv;
    output.ray = normalize(pos.xyz - eye);
    
	return output;
}