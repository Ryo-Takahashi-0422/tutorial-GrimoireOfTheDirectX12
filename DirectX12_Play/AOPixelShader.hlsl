#include <AOShaderHeader.hlsli>

float SsaoPs(Output input) : SV_TARGET
{
    float w, h, levels;
    depthmap.GetDimensions(0, w, h, levels);
    float dx = 1.0f / w;
    float dy = 1.0f / h;
 
    float4 rpos = (0, 0, 0, 0);

    float dp = normalmap.Sample(smp, float2(input.svpos.x / w, input.svpos.y / h));
    //return dp;
    // SSAO

    float div = 0.0f;
    float ao = 0.0f;
    float3 norm = normalize(input.norm.xyz);
    const int trycnt = 32;
    const float radius = 12.0f;
    
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
            
            rpos.x = input.svpos.x + omega.x * radius;
            rpos.y = input.svpos.y + omega.y * radius; // omega.y�𑫂��ƂȂ���rpos.z�Ɉ��e�����o�Đ[�x�`�F�b�N����肭�����Ȃ�...�Ȃ��H�H�H
            rpos.z = input.svpos.z;
            rpos.w = input.svpos.w;
            
            // �v�Z���ʂ����݂̏ꏊ�̐[�x��艜�ɓ����Ă���Ȃ�Ւf����Ă���̂ŉ��Z����
            // x > y = 1, x < y = 0
            ao += step(depthmap.Sample(smp, float2(rpos.x / w, rpos.y / h)), rpos.z) * dt;
        }
        
        ao /= div;
    }
    
    return (1.0f - ao) * 3;
}

float random(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}