#include "BasicShaderHeader.hlsli"

Output BasicVS
(float4 pos : POSITION,
float4 norm : NORMAL,
float2 uv : TEXCOORD,
min16uint2 boneno : BONE_NO,
min16uint weight : WEIGHT,
uint instNo : SV_InstanceID
)
{
    Output output; // �s�N�Z���V�F�[�_�[�ɓn���l
    float w = weight / 100.0f;
    matrix bm = bones[boneno[0]] * w + bones[boneno[1]] * (1 - w);
    pos = mul(bm, pos);
    
    //if(instNo == 1)
    //{
    //    pos = mul(shadow, pos); // �e���v�Z
    //}
    
    // pixelshader�ւ̏o�͂�����Ă���
    output.svpos = mul(mul(mul(proj, view), world), pos)/*mul(lightCamera, pos)*/;
    norm.w = 0; // world�ɕ��s�ړ��������܂܂�Ă���ꍇ�A�@�������s�ړ�����B(���̎����f���͈Â��Ȃ�B�Ȃ��H�H)
    output.norm = mul(world, norm);
    output.vnormal = mul(view, output.norm);
    output.uv = uv;
    output.ray = normalize(pos.xyz - eye);
    output.instNo = instNo;
    output.tpos = mul(lightCamera, pos);
    
	return output;
}

float4 LightMapVS
(float4 pos : POSITION,
float4 norm : NORMAL,
float2 uv : TEXCOORD,
min16uint2 boneno : BONE_NO,
min16uint weight : WEIGHT) : SV_POSITION
{
    float w = float(weight) / 100.0f;
    matrix bm = bones[boneno[0]] * w + bones[boneno[1]] * (1 - w);
    pos = mul(bm, pos);
    
    //pos = mul(mul(mul(proj, view), world), pos) /*mul(lightCamera, pos)*/;
    //return pos;
    return mul(lightCamera, pos);
}