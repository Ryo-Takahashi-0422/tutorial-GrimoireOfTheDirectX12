#include <AOShaderHeader.hlsli>

float SsaoPs(Output input) : SV_TARGET
{
    float w, h, levels;
    depthmap.GetDimensions(0, w, h, levels);
    float dx = 1.0f / w;
    float dy = 1.0f / h;
    
    //return input.svpos.z;
 
    // �����~�N�����߂Ă��āA���ꂪ���f����Ă���Ǝv����B
    //return pow(depthmap.Sample(smp, input.uv), 20);
    float4 respos = (0, 0, 0, 0);
    float4 rpos = (0, 0, 0, 0);

    float dp = depthmap.Sample(smp, float2(input.svpos.x / w, input.svpos.y / h)) - 0.02f; // ���f����UV����ׂ̐[�x�}�b�v�ł͍��W�ɐ��������Ȃ����ߏ�肭�����Ȃ��ƍl����B�ڂ͋��R�ǐ^�񒆂�
    //dp = pow(dp, 20);
    //return dp;
    

    
    // SSAO
    // restore original coordinate 
    respos = mul(invProj, input.svpos); //float4(input.uv * float2(2, -2) + float2(-1, 1), dp, 1));
    respos = mul(invView, respos);
    //respos = mul(view, respos); // invView��ł������ƌ��ʂ�ɂȂ��Ă���
    //respos = mul(proj, respos); // invProj��ł������ƌ��ʂ�ɂȂ��Ă���
    //return respos.z;
    //respos.xyz /= respos.w;

    float div = 0.0f;
    float ao = 0.0f;
    float3 norm = normalize(input.norm.xyz); //normalize(normalmap.Sample(smp, input.uv) * 2 - 1);
    //return float4(norm.x, norm.y, norm.z, 1);
    const int trycnt = 128;
    const float radius = 10.0f;
    
    if(dp < 1.0f)
    {
        for (int i = 0; i < trycnt; ++i)
        {
            float rnd1 = random(float2(i * dx, i * dy)) * 2 - 1;
            float rnd2 = random(float2(rnd1, i * dy)) * 2 - 1;
            float rnd3 = random(float2(rnd2, rnd1)) * 2 - 1;
            float3 omega = normalize(float3(rnd1, rnd2, rnd3));
            omega = normalize(omega);
            
            // �����̌��ʖ@���̔��Α��Ɍ����Ă����甽�]
            float dt = dot(norm, omega);
            float sgn = sign(dt);
            omega *= sign(dt);
            dt *= sgn; // ���̒l�ɂ���cos�Ƃ𓾂�            
            div += dt; // �Ւf���l���Ȃ����ʂ����Z���� 
            
            // ��invproj�̂ݓK�p�A���f���̉����ʂɉe�������p�^�[��
            //float4 rpos = mul(proj, float4(respos.xyz + omega * radius, 1));
            //ao += step(depthmap.Sample(smp, (rpos.xy + float2(1, -1)) * float2(0.5f, -0.5f)), rpos.z) * dt;
            // ��            
            
            rpos.x = input.svpos.x + omega.x * radius;
            rpos.y = input.svpos.y + omega.y * radius; // omega.y�𑫂��ƂȂ���rpos.z�Ɉ��e�����o�Đ[�x�`�F�b�N����肭�����Ȃ�...�Ȃ��H�H�H
            rpos.z = input.svpos.z/* + omega.z * radius*/;
            rpos.w = input.svpos.w;
            //rpos = mul(view, rpos);
            //rpos = mul(proj, rpos);
            //return rpos.z;
            
            //rpos = mul(proj, float4(respos.x + omega.x * radius, respos.y + omega.y * radius, respos.z + omega.z * radius, 1));
            //rpos.xyz /= rpos.w;  
            
            //return depthmap.Sample(smp, (rpos.xy + float2(1, -1)) * float2(0.5f, -0.5f));
            //return depthmap.Sample(smp, float2(rpos.x / w, rpos.y / h));
            
            // �v�Z���ʂ����݂̏ꏊ�̐[�x��艜�ɓ����Ă���Ȃ�Ւf����Ă���̂ŉ��Z����
            // x > y = 1, x < y = 0
            //ao += step(depthmap.Sample(smp, (rpos.xy + float2(1, -1)) * float2(0.5f, -0.5f)), rpos.z) * dt;
            ao += step(depthmap.Sample(smp, float2(rpos.x / w, rpos.y / h)) / 1.0001f, rpos.z) * dt;

        }
        
        ao /= div;
    }
    
    
    //return rpos.z;
    //return depthmap.Sample(smp, float2(rpos.x / w, rpos.y / h)) - 8.0f * (1 - input.svpos.z);
    return 1.0f - ao;
}

float random(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}