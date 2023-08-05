#include "BufferHeader.hlsli"

// entry point for bloom
float4 BloomPS(Output input) : SV_TARGET
{
    float w, h, miplevels;
    tex.GetDimensions(0, w, h, miplevels);
    return Get5x5GaussianBlur(tex, smp, input.uv, 1.0 / w, 1.0 / h);

}

float4 Get5x5GaussianBlur(Texture2D _texture, SamplerState _smp, float2 _uv, float dx, float dy)
{
    float4 ret = float4(0, 0, 0, 0);
    ret = model.Sample(smp, _uv);
    ret += bkweights[0] * ret;
    
    // ������
    for (int i = 1; i < 8; ++i)
    {
        ret += bkweights[i >> 2][i % 4] * _texture.Sample(smp, _uv + float2(i * dx, 0));
        //0000, 1
        //0000, 2
        //0000, 3
        //0001, 0
        //0001, 1
        //0001, 2
        //0001, 3�̕��сB���̍s��-i���-1�`-8�܂�
        ret += bkweights[i >> 2][i % 4] * _texture.Sample(smp, _uv + float2(-i * dx, 0));
    }
    
    // �c����
    for (int j = 1; j < 8; ++j)
    {
        ret += bkweights[j >> 2][j % 4] * _texture.Sample(smp, _uv + float2(0, 1 * dy));
        //0000, 1
        //0000, 2
        //0000, 3
        //0001, 0
        //0001, 1
        //0001, 2
        //0001, 3�̕��сB���̍s��-j���-1�`-8�܂�
        ret += bkweights[j >> 2][j % 4] * _texture.Sample(smp, _uv + float2(0, -j * dy));
    }
    
    return ret;
}