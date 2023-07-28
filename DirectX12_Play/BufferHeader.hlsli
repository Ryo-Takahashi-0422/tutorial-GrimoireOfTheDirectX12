Texture2D<float4> tex : register(t0); // �}���`�p�X1�e�N�X�`��
Texture2D<float4> model : register(t1); // �}���`�p�X2�e�N�X�`��(���f��)
Texture2D<float4> normalmap : register(t2); // �m�[�}���}�b�v
Texture2D<float> depthmap : register(t3); // �f�v�X�}�b�v
Texture2D<float> lightmap : register(t4); // ���C�g�}�b�v

cbuffer PostEffect : register(b0) // ��ʃG�t�F�N�g
{
    float4 bkweights[2];
};

cbuffer SceneBuffer : register(b1) // �ϊ��s��
{
    matrix world; // ���[���h�s��
    matrix view; // �r���[�s��
    matrix proj; // �v���W�F�N�V�����s��
    matrix lightCamera; // ���C�g���猩���r���[
    matrix shadow; // �e
    float3 eye; // ���_
    matrix bones[256]; // �{�[���s��
};

SamplerState smp : register(s0); // �T���v���[

struct Output
{
    float4 svpos : SV_POSITION;
    float4 norm : NORMAL0; // �@���x�N�g��
    float2 uv : TEXCOORD;
    float4 tpos : TPOS;
};
