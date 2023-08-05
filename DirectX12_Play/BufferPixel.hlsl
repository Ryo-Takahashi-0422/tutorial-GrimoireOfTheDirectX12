#include "BufferHeader.hlsli"

// entry point for BufferShaderCompile
float4 psBuffer(Output input) : SV_TARGET
{
    float w, h, levels;
    float dx;
    float dy;
    
    model.GetDimensions(0, w, h, levels);
    
    float4 col = model.Sample(smp, input.uv) + tex.Sample(smp, input.uv);
    // buffer[0] + buffer[1]による出力
    float dep = pow(depthmap.Sample(smp, input.uv), 20);
    float4 dep4 = float4(dep, dep, dep, 1);
    
    float lmap = pow(lightmap.Sample(smp, input.uv), 0.3);
    float4 lmap4 = float4(lmap, lmap, lmap, 1);
    
    //return bloommap.Sample(smp, input.uv);
    //return dep4;
    //return col;
    if (input.uv.x < 0.3 && input.uv.y < 0.3)
    {
        return multinormalmap.Sample(smp, (input.uv) * 3);
    }
    
    // deffered shading
    float4 nm = multinormalmap.Sample(smp, input.uv);
    nm = nm * 2.0f - 1.0f;
    float3 light = normalize(float3(1.0f, -1.0f, 1.0f));
    const float ambient = 0.25f;
    float diffB = max(saturate(dot(nm.xyz, -light)), ambient); // dot(nm.xyz, -light)) is cosθ between normalized reflection(-light) direction and normal direction of vertex
    
    //return model.Sample(smp, input.uv)/* * float4(diffB, diffB, diffB, 1)*/;
    
    return model.Sample(smp, input.uv) + Get5x5GaussianBlur(bloommap, smp, input.uv, 1.0f / w, 1.0f / h);   
}

float4 MakePAL(float4 col)
{
    float Y = dot(col.rgb, float3(0.299, 0.587, 0.114));
    return float4(Y, Y, Y, 1);
}

float4 RevrseColor(float4 col)
{
    return float4(float3(1.0f, 1.0f, 1.0f) - col.rgb, col.a);
}

float4 DownColorGradation(float4 col)
{
    return float4(col.rgb - fmod(col.rgb, 0.25f), col.a);
}

float4 Emboss(Texture2D _texture, SamplerState _smp, float2 _uv, int offset, float dx, float dy)
{
    float4 ret = float4(0, 0, 0, 0);
    
    // up-left, up, up-right
    ret += _texture.Sample(_smp, _uv + float2(-offset * dx, offset * dy)) * 2;
    ret += _texture.Sample(_smp, _uv + float2(0, offset * dy));
    ret += _texture.Sample(_smp, _uv) * 0;
    
    // left, center, right
    ret += _texture.Sample(_smp, _uv + float2(-offset * dx, 0));
    ret += _texture.Sample(_smp, _uv);
    ret += _texture.Sample(_smp, _uv + float2(offset * dx, 0)) * -1;
    
    //down-left, down, down-right
    ret += _texture.Sample(_smp, _uv + float2(-offset * dx, -offset * dy)) * 0;
    ret += _texture.Sample(_smp, _uv + float2(0, -offset * dy)) * -1;
    ret += _texture.Sample(_smp, _uv + float2(offset * dx, -offset * dy)) * -2;
    
    return ret;
}

float4 Sharpness(Texture2D _texture, SamplerState _smp, float2 _uv, int offset, float dx, float dy)
{
    float4 ret = float4(0, 0, 0, 0);
    
    // up-left, up, up-right
    ret += _texture.Sample(_smp, _uv + float2(-offset * dx, offset * dy)) * 0;
    ret += _texture.Sample(_smp, _uv + float2(0, offset * dy)) * -1;
    ret += _texture.Sample(_smp, _uv) * 0;
    
    // left, center, right
    ret += _texture.Sample(_smp, _uv + float2(-offset * dx, 0)) * -1;
    ret += _texture.Sample(_smp, _uv) * 4;
    ret += _texture.Sample(_smp, _uv + float2(offset * dx, 0)) * -1;
    
    //down-left, down, down-right
    ret += _texture.Sample(_smp, _uv + float2(-offset * dx, -offset * dy)) * 0;
    ret += _texture.Sample(_smp, _uv + float2(0, -offset * dy)) * -1;
    ret += _texture.Sample(_smp, _uv + float2(offset * dx, -offset * dy)) * 0;
    
    float Y = dot(ret.rgb, float3(0.299, 0.587, 0.114));
    Y = pow(1.0f - Y, 10.0f);
    Y = step(0.2, Y);
    return float4(Y, Y, Y, 1);
}

float4 Get5x5GaussianBlur(Texture2D _texture, SamplerState _smp, float2 _uv, float dx, float dy)
{
    float4 ret = float4(0, 0, 0, 0);
    ret = model.Sample(smp, _uv);
    ret += bkweights[0] * ret;
    
    // 横方向
    for (int i = 1; i < 8; ++i)
    {
        ret += bkweights[i >> 2][i % 4] * _texture.Sample(smp, _uv + float2(i * dx, 0));
        //0000, 1
        //0000, 2
        //0000, 3
        //0001, 0
        //0001, 1
        //0001, 2
        //0001, 3の並び。次の行は-iより-1～-8まで
        ret += bkweights[i >> 2][i % 4] * _texture.Sample(smp, _uv + float2(-i * dx, 0));
    }
    
    // 縦方向
    for (int j = 1; j < 8; ++j)
    {
        ret += bkweights[j >> 2][j % 4] * _texture.Sample(smp, _uv + float2(0, 1 * dy));
        //0000, 1
        //0000, 2
        //0000, 3
        //0001, 0
        //0001, 1
        //0001, 2
        //0001, 3の並び。次の行は-jより-1～-8まで
        ret += bkweights[j >> 2][j % 4] * _texture.Sample(smp, _uv + float2(0, -j * dy));
    }
    
    return ret;
}

float4 AverageBlur(Texture2D _texture, SamplerState _smp, float2 _uv, int offset, float dx, float dy)
{
    float4 ret = float4(0, 0, 0, 0);
    
    // up-left, up, up-right
    ret += _texture.Sample(_smp, _uv + float2(-offset * dx, offset * dy));
    ret += _texture.Sample(_smp, _uv + float2(0, offset * dy));
    ret += _texture.Sample(_smp, _uv + float2(offset * dx, offset * dy));
    
    // left, center, right
    ret += _texture.Sample(_smp, _uv + float2(-offset * dx, 0));
    ret += _texture.Sample(_smp, _uv);
    ret += _texture.Sample(_smp, _uv + float2(offset * dx, 0));
    
    //down-left, down, down-right
    ret += _texture.Sample(_smp, _uv + float2(-offset * dx, -offset * dy));
    ret += _texture.Sample(_smp, _uv + float2(0, -offset * dy));
    ret += _texture.Sample(_smp, _uv + float2(offset * dx, -offset * dy));
    
    return ret / 9.0f;
}

float4 SimpleGaussianBlur(Texture2D _texture, SamplerState _smp, float2 _uv, float dx, float dy)
{
    float4 ret = float4(0, 0, 0, 0);

    // highest
    ret += model.Sample(smp, _uv + float2(-2 * dx, 2 * dy)) * 1;
    ret += model.Sample(smp, _uv + float2(-1 * dx, 2 * dy)) * 4;
    ret += model.Sample(smp, _uv + float2(0 * dx, 2 * dy)) * 6;
    ret += model.Sample(smp, _uv + float2(1 * dx, 2 * dy)) * 4;
    ret += model.Sample(smp, _uv + float2(2 * dx, 2 * dy)) * 1;
    // high
    ret += model.Sample(smp, _uv + float2(-2 * dx, 1 * dy)) * 4;
    ret += model.Sample(smp, _uv + float2(-1 * dx, 1 * dy)) * 16;
    ret += model.Sample(smp, _uv + float2(0 * dx, 1 * dy)) * 24;
    ret += model.Sample(smp, _uv + float2(1 * dx, 1 * dy)) * 16;
    ret += model.Sample(smp, _uv + float2(2 * dx, 1 * dy)) * 4;
    // middle
    ret += model.Sample(smp, _uv + float2(-2 * dx, 0 * dy)) * 6;
    ret += model.Sample(smp, _uv + float2(-1 * dx, 0 * dy)) * 24;
    ret += model.Sample(smp, _uv + float2(0 * dx, 0 * dy)) * 36;
    ret += model.Sample(smp, _uv + float2(1 * dx, 0 * dy)) * 24;
    ret += model.Sample(smp, _uv + float2(2 * dx, 0 * dy)) * 6;
    // low
    ret += model.Sample(smp, _uv + float2(-2 * dx, -1 * dy)) * 4;
    ret += model.Sample(smp, _uv + float2(-1 * dx, -1 * dy)) * 16;
    ret += model.Sample(smp, _uv + float2(0 * dx, -1 * dy)) * 24;
    ret += model.Sample(smp, _uv + float2(1 * dx, -1 * dy)) * 16;
    ret += model.Sample(smp, _uv + float2(2 * dx, -1 * dy)) * 4;
    // lowest
    ret += model.Sample(smp, _uv + float2(-2 * dx, -2 * dy)) * 1;
    ret += model.Sample(smp, _uv + float2(-1 * dx, -2 * dy)) * 4;
    ret += model.Sample(smp, _uv + float2(0 * dx, -2 * dy)) * 6;
    ret += model.Sample(smp, _uv + float2(1 * dx, -2 * dy)) * 4;
    ret += model.Sample(smp, _uv + float2(2 * dx, -2 * dy)) * 1;
 
    return ret / 256;
}

float4 NormalmapEffect(float2 _uv)
{
    float4 ret = float4(0, 0, 0, 0);
    float2 normalTex = normalmap.Sample(smp, _uv);
    normalTex = normalTex * 2.0f - 1.0f; // 法線マップは法線方向Nx,Ny,Nzをそれぞれ(1+Nx,y,z)/2として画像データとしているので、法線方向データに戻す
    
    for (int i = 1; i < 8; ++i)
    {
        ret += bkweights[i >> 2][i % 4] * model.Sample(smp, _uv + normalTex * 0.1f);
        //0000, 1
        //0000, 2
        //0000, 3
        //0001, 0
        //0001, 1
        //0001, 2
        //0001, 3の並び。次の行は-iより-1～-8まで
        ret += bkweights[i >> 2][i % 4] * model.Sample(smp, _uv + normalTex * 0.1f);
    }
    
    //return float4(ret.rgb / 2, ret.a);
    
    ret = model.Sample(smp, _uv + normalTex * 0.1f) + tex.Sample(smp, _uv/* + normalTex * 0.5f*/);
    
    return ret;
}