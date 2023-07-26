Texture2D<float4> tex : register(t0); // �}���`�p�X1�e�N�X�`��
Texture2D<float4> model : register(t1); // �}���`�p�X2�e�N�X�`��(���f��)
Texture2D<float4> normalmap : register(t2); // �m�[�}���}�b�v

cbuffer PostEffect : register(b0) // ��ʃG�t�F�N�g
{
    float4 bkweights[2];
};

SamplerState smp : register(s0); // �T���v���[

struct Output
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};
