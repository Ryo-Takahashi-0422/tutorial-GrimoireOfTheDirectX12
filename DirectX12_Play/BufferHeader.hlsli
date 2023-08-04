Texture2D<float4> tex : register(t0); // multipass2 texture(background texture)
Texture2D<float4> model : register(t1); // multipass2 texture(pmd model rendering texture)
Texture2D<float4> normalmap : register(t2); // �m�[�}���}�b�v
Texture2D<float> depthmap : register(t3); // �f�v�X�}�b�v
Texture2D<float> lightmap : register(t4); // ���C�g�}�b�v
Texture2D<float4> multinormalmap : register(t5); // �}���`�^�[�Q�b�g�Ƀ����_�����O�����@���}�b�v

cbuffer PostEffect : register(b0) // post-effect vector
{
    float4 bkweights[2];
};

cbuffer SceneBuffer : register(b1) // affine transformation matrix
{
    matrix world; // world matrix
    matrix view; // view matrix
    matrix proj; // projection matrix
    matrix lightCamera; // view matrix from light * orthographic projection matrix
    matrix shadow; // shadow matrix
    float3 eye; // eye(camera) position
    matrix bones[256]; // bone matrix
};

SamplerState smp : register(s0); // sampler

struct Output
{
    float4 svpos : SV_POSITION;
    float4 norm : NORMAL0;
    float2 uv : TEXCOORD;
    float4 tpos : TPOS;
};
