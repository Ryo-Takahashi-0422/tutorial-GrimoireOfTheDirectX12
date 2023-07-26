#include "BasicShaderHeader.hlsli"

float4 BasicPS(Output input) : SV_TARGET
{
    if (input.instNo == 1)
    {
        return float4(0, 0, 0, 1);
    }
    
        float3 light = normalize(float3(1, -1, 1));
        float3 lightColor = float3(1, 1, 1);
    
    // �f�B�t���[�Y�v�Z
        float diffuseB = saturate(dot(-light, input.norm.xyz));
        float4 toonDif = toon.Sample(smpToon, float2(0, diffuseB));
    
    //���̔��˃x�N�g��
        float3 refLight = normalize(reflect(light, input.norm.xyz));
        float specularB = pow(saturate(dot(refLight, -input.ray)), specular.a);
        float4 toonSpecB = toon.Sample(smpToon, float2(0, specularB));
    
    //�X�t�B�A�}�b�v�pUV
        float2 sphereMapUV = (input.vnormal.xy + float2(1, -1)) * float2(0.5, -0.5);
    
    // �e�N�X�`���J���[
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