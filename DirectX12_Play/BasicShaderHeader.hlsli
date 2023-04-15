struct Output
{
    float4 svpos : SV_POSITION; // システム用頂点座標
    float4 norm : NORMAL;
    float2 uv : TEXCOORD; // uv値
};

cbuffer cbuff0 : register(b0) // 変換行列
{
    matrix world;
    matrix viewproj;
};

cbuffer Material : register(b1)
{
    float4 diffuse; // r,g,b:diffuse , a:alpha
    float4 specular; // r,g,b:specular , a:specularity
    float3 ambient;
}
