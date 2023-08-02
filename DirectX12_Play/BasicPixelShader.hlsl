#include "BasicShaderHeader.hlsli"

float4 BasicPS(Output input) : SV_TARGET
{    
    //if (input.instNo == 1)
    //{
    //    return float4(0, 0, 0, 1);
    //}
    
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
      
    float4 result = saturate(diffuse * texColor + toonSpecB * float4(specular.rgb, 1));
   
    //return result;
    
    //float4 result = max(saturate(
    //toonDif
    //* diffuse
    //* texColor
    //* sph.Sample(smp, sphereMapUV))
    //+ saturate(spa.Sample(smp, sphereMapUV) * texColor
    //+ toonSpecB * float4(specular.rgb, 1)), float4(texColor.xyz * ambient, 1));
    //return result;
    
    
    
    
    light = normalize(float3(1, -1, 1));
    float bright = dot(input.norm, -light);
    
    float shadowWeight = 1.0f;
    float3 posFromLightVP = input.tpos.xyz / input.tpos.w; // -1<=x<=1, -1<=y<=1, 0<=z<=1
    float2 shadowUV = (posFromLightVP.xy + float2(1, -1)) * float2(0.5, -0.5); // 0<=x<=1, -1<=y<=0
    //float depthFromLight = lightmap.Sample(smp, shadowUV);
    
    //if (depthFromLight < posFromLightVP.z - 0.001f)
    //{
    //    shadowWeight = 0.5f;
    //}
    //shadowWeight = lerp(0.5f, 1.0f, depthFromLight);
    float depthFromLight = lightmap.SampleCmp(smpBilinear, shadowUV, posFromLightVP.z - 0.005f);
    
    shadowWeight = lerp(0.5f, 1.0f, depthFromLight);
    float b = /*bright **/ shadowWeight;
        
    float lmap = pow(lightmap.Sample(smp, shadowUV), 0.3); // このuvはモデルのuvである。レンダリングされた画像のuvとは違うので上手く読めない？
    float4 lmap4 = float4(lmap, lmap, lmap, 1);
    //return lmap4; //!!!この結果がBufferPixelと異なっている。こちらがオカシイ。テクスチャが読み込めていない様子。
    return shadowWeight * result;
    
}