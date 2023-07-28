Texture2D<float4> tex : register(t0); // マルチパス1テクスチャ
Texture2D<float4> model : register(t1); // マルチパス2テクスチャ(モデル)
Texture2D<float4> normalmap : register(t2); // ノーマルマップ
Texture2D<float> depthmap : register(t3); // デプスマップ
Texture2D<float> lightmap : register(t4); // ライトマップ

cbuffer PostEffect : register(b0) // 画面エフェクト
{
    float4 bkweights[2];
};

cbuffer SceneBuffer : register(b1) // 変換行列
{
    matrix world; // ワールド行列
    matrix view; // ビュー行列
    matrix proj; // プロジェクション行列
    matrix lightCamera; // ライトから見たビュー
    matrix shadow; // 影
    float3 eye; // 視点
    matrix bones[256]; // ボーン行列
};

SamplerState smp : register(s0); // サンプラー

struct Output
{
    float4 svpos : SV_POSITION;
    float4 norm : NORMAL0; // 法線ベクトル
    float2 uv : TEXCOORD;
    float4 tpos : TPOS;
};
