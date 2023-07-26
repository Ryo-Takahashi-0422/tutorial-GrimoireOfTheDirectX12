#include "BasicShaderHeader.hlsli"

float4 BasicPS(Output input) : SV_TARGET
{
    if (input.instNo == 1)
    {
        return float4(0, 0, 0, 1);
    }
    
        float3 light = normalize(float3(1, -1, 1));
        float3 lightColor = float3(1, 1, 1);
    
    // ディフューズ計算
        float diffuseB = saturate(dot(-light, input.norm.xyz));
        float4 toonDif = toon.Sample(smpToon, float2(0, diffuseB));
    
    //光の反射ベクトル
        float3 refLight = normalize(reflect(light, input.norm.xyz));
        float specularB = pow(saturate(dot(refLight, -input.ray)), specular.a);
        float4 toonSpecB = toon.Sample(smpToon, float2(0, specularB));
    
    //スフィアマップ用UV
        float2 sphereMapUV = (input.vnormal.xy + float2(1, -1)) * float2(0.5, -0.5);
    
    // テクスチャカラー
        float4 texColor = tex.Sample(smp, input.uv);
   
    float4 result = max(saturate(
    toonDif
    * diffuse
    * texColor
    * sph.Sample(smp, sphereMapUV))
    + saturate(spa.Sample(smp, sphereMapUV) * texColor
    + toonSpecB * float4(specular.rgb, 1)), float4(texColor.xyz * ambient, 1));
        
    return result;
    
    //return max(saturate(
    //toonDif
    //* diffuse
    //* texColor
    //* sph.Sample(smp, sphereMapUV))
    //+ saturate(spa.Sample(smp, sphereMapUV) * texColor
    //+ float4(texColor.xyz * ambient, 1)), float4(specularB * specular.rgb, 1));
    }