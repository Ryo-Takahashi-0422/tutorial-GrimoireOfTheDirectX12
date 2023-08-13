Texture2D<float> depthmap : register(t0); // No.0 depthmap
Texture2D<float4> normalmap : register(t1); // No.1 lightmap(depth from light-view)


cbuffer SceneBuffer : register(b0) // affine transformation matrix
{
    matrix world; // world matrix
    matrix view; // view matrix
    matrix proj; // projection matrix
    matrix invProj; // inverse matrix of projection matrix
    matrix lightCamera; // view matrix from light * orthographic projection matrix
    matrix shadow; // shadow matrix
    float3 eye; // eye(camera) position
    matrix bones[256]; // bone matrix
};

SamplerState smp : register(s0); // sampler
SamplerState smp2 : register(s1); // sampler

struct AoOutput
{
    float ao : SV_TARGET0; // model color rendering
};

struct Output
{
    float4 svpos : SV_POSITION; // �V�X�e���p���_���W
    float4 pos : POSITON; // ���_���W
    float4 norm : NORMAL0; // �@���x�N�g��
    float4 vnormal : NORMAL1; // �r���[�ϊ���̖@���x�N�g��
    float2 uv : TEXCOORD; // uv�l
    float3 ray : VECTOR; // ���_�x�N�g��
    uint instNo : SV_InstanceID; // DrawIndexedInstanced��instance id
    float4 tpos : TPOS;
};

float random(float2 uv);