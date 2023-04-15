struct Output
{
    float4 svpos : SV_POSITION; // �V�X�e���p���_���W
    float4 norm : NORMAL;
    float2 uv : TEXCOORD; // uv�l
};

cbuffer cbuff0 : register(b0) // �ϊ��s��
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
