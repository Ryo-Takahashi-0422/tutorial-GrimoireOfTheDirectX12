#include "BasicShaderHeader.hlsli"

float4 BasicPS(Output input) : SV_TARGET
{    
    //if (input.instNo == 1)
    //{
    //    return float4(0, 0, 0, 1);
    //}
    
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
    
    float3 posFromLightVP = input.tpos.xyz / input.tpos.w;
    float2 shadowUV = (posFromLightVP.xy  + float2(1, -1)) * float2(0.5, -0.5);
    float depthFromLight = lightmap.Sample(smp, shadowUV);
    float shadowWeight = 1.0f;
    if (depthFromLight < posFromLightVP.z - 0.001f)
    {
        shadowWeight = 0.5f;
    }
    //shadowWeight = lerp(0.5f, 1.0f, depthFromLight);
    float b = bright * shadowWeight;
    
    float lmap = pow(lightmap.Sample(smp, input.uv), 0.3);
    float4 lmap4 = float4(lmap, lmap, lmap, 1);
    return lmap4; //!!!���̌��ʂ�BufferPixel�ƈقȂ��Ă���B�����炪�I�J�V�C�B�e�N�X�`�����ǂݍ��߂Ă��Ȃ��l�q�B
    return float4(b, b, b, 1)/* * result*/;
    
}