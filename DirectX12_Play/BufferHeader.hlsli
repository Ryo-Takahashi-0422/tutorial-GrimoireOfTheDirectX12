Texture2D<float4> tex : register(t0); // multipass2 texture(background texture)
Texture2D<float4> model : register(t1); // multipass2 texture(pmd model rendering texture)
Texture2D<float4> normalmap : register(t2); // ノーマルマップ
Texture2D<float> depthmap : register(t3); // デプスマップ
Texture2D<float> lightmap : register(t4); // ライトマップ
Texture2D<float4> multinormalmap : register(t5); // マルチターゲットにレンダリングした法線マップ
Texture2D<float4> bloommap : register(t6); // マルチターゲットにレンダリングした法線マップ

SamplerState smp : register(s0); // sampler

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

struct Output
{
    float4 svpos : SV_POSITION;
    float4 norm : NORMAL0;
    float2 uv : TEXCOORD;
    float4 tpos : TPOS;
};

// return grayscale of PAL(phase alternating line、位相反転線) from input
float4 MakePAL(float4 col);

// return inversion color value
float4 RevrseColor(float4 col);

// down color gradation
float4 DownColorGradation(float4 col);

// emboss
float4 Emboss(Texture2D _texture, SamplerState _smp, float2 _uv, int offset, float dx, float dy);

// sharpness
float4 Sharpness(Texture2D _texture, SamplerState _smp, float2 _uv, int offset, float dx, float dy);

// return gaussian blur matrix
float4 Get5x5GaussianBlur(Texture2D _texture, SamplerState _smp, float2 _uv, float dx, float dy);

// blur by pixel averaging
float4 AverageBlur(Texture2D _texture, SamplerState _smp, float2 _uv, int offset, float dx, float dy);

// simple gaussian blur
float4 SimpleGaussianBlur(Texture2D _texture, SamplerState _smp, float2 _uv, float dx, float dy);

// normalize texture use
float4 NormalmapEffect(float2 _uv);