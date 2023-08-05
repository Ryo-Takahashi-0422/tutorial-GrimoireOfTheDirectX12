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

struct PixelOutput
{    
    float4 col : SV_TARGET0; // model color rendering
    float4 mnormal : SV_TARGET1; // model normal rendering
    float4 highLum : SV_TARGET2; // model high luminance rendering
};

cbuffer SceneBuffer : register(b0) // �ϊ��s��
{
    matrix world; // world matrix
    matrix view; // view matrix
    matrix proj; // projection matrix
    matrix lightCamera; // view matrix from light * orthographic projection matrix
    matrix shadow; // shadow matrix
    float3 eye; // eye(camera) position
    matrix bones[256]; // bone matrix
};

cbuffer Material : register(b1)
{
    float4 diffuse; // r,g,b:diffuse , a:alpha
    float4 specular; // r,g,b:specular , a:specularity
    float3 ambient;
}

SamplerState smp : register(s0); // No.0 sampler
SamplerState smpToon : register(s1); // No.1 sampler(toon)
SamplerComparisonState smpBilinear : register(s2); // No.2 sampler

Texture2D<float> depthmap : register(t0); // No.0 depthmap
Texture2D<float> lightmap : register(t1); // No.1 lightmap(depth from light-view)
Texture2D<float4> tex : register(t2); // No.2 eye texture
Texture2D<float4> sph : register(t3); // No.3 .sph texture
Texture2D<float4> spa : register(t4); // No.4 .spa texture
Texture2D<float4> toon : register(t5); //No.5 toon texture