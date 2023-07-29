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

cbuffer SceneBuffer : register(b0) // �ϊ��s��
{
    matrix world; // ���[���h�s��
    matrix view; // �r���[�s��
    matrix proj; // �v���W�F�N�V�����s��
    matrix lightCamera; // ���C�g���猩���r���[
    matrix shadow; // �e
    float3 eye; // ���_
    matrix bones[256]; // �{�[���s��
};

cbuffer Material : register(b1)
{
    float4 diffuse; // r,g,b:diffuse , a:alpha
    float4 specular; // r,g,b:specular , a:specularity
    float3 ambient;
}

SamplerState smp : register(s0); // 0�ԃX���b�g�ɐݒ肳�ꂽ�T���v���[
SamplerState smpToon : register(s1); // 1�ԃX���b�g�ɐݒ肳�ꂽ�T���v���[(�g�D�[��)

Texture2D<float4> tex : register(t2); //0�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> sph : register(t3); // 1�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> spa : register(t4); // 2�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> toon : register(t5); // 3�ԃX���b�g�ɐݒ肳�ꂽ�g�D�[���e�N�X�`��
Texture2D<float> depthmap : register(t0); // 4�ԃX���b�g�Ƀf�v�X�}�b�v�e�N�X�`��
Texture2D<float> lightmap : register(t1); // 4�ԃX���b�g�Ƀ��C�g�}�b�v�e�N�X�`��