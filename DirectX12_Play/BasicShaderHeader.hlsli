struct Output
{
    float4 svpos : SV_POSITION; // システム用頂点座標
    float4 pos : POSITON; // 頂点座標
    float4 norm : NORMAL0; // 法線ベクトル
    float4 vnormal : NORMAL1; // ビュー変換後の法線ベクトル
    float2 uv : TEXCOORD; // uv値
    float3 ray : VECTOR; // 視点ベクトル
    uint instNo : SV_InstanceID; // DrawIndexedInstancedのinstance id
    float4 tpos : TPOS;
};

cbuffer SceneBuffer : register(b0) // 変換行列
{
    matrix world; // ワールド行列
    matrix view; // ビュー行列
    matrix proj; // プロジェクション行列
    matrix lightCamera; // ライトから見たビュー
    matrix shadow; // 影
    float3 eye; // 視点
    matrix bones[256]; // ボーン行列
};

cbuffer Material : register(b1)
{
    float4 diffuse; // r,g,b:diffuse , a:alpha
    float4 specular; // r,g,b:specular , a:specularity
    float3 ambient;
}

SamplerState smp : register(s0); // 0番スロットに設定されたサンプラー
SamplerState smpToon : register(s1); // 1番スロットに設定されたサンプラー(トゥーン)
SamplerComparisonState smpBilinear : register(s2); // 2番スロットに設定された、色情報そのものではなくその比較情報を得るサンプラー

Texture2D<float4> tex : register(t2); //0番スロットに設定されたテクスチャ
Texture2D<float4> sph : register(t3); // 1番スロットに設定されたテクスチャ
Texture2D<float4> spa : register(t4); // 2番スロットに設定されたテクスチャ
Texture2D<float4> toon : register(t5); // 3番スロットに設定されたトゥーンテクスチャ
Texture2D<float> depthmap : register(t0); // 4番スロットにデプスマップテクスチャ
Texture2D<float> lightmap : register(t1); // 4番スロットにライトマップテクスチャ