Texture2D<float4> tex : register(t0); // マルチパス1テクスチャ
Texture2D<float4> model : register(t1); // マルチパス2テクスチャ(モデル)
SamplerState smp : register(s0); // サンプラー

struct Output
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};
