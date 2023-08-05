#include "BufferHeader.hlsli"

float4 psBuffer(Output input) : SV_TARGET
{
    float4 col = model.Sample(smp, input.uv) + tex.Sample(smp, input.uv);
    // buffer[0] + buffer[1]による出力
    float dep = pow(depthmap.Sample(smp, input.uv), 20);
    float4 dep4 = float4(dep, dep, dep, 1);
    
    float lmap = pow(lightmap.Sample(smp, input.uv), 0.3);
    float4 lmap4 = float4(lmap, lmap, lmap, 1);
    
    //return lmap4;
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
    float diffB = max(saturate(dot(nm.xyz, -light)), 0.6); // dot(nm.xyz, -light)) is cosθ between normalized reflection(-light) direction and normal direction of vertex
    
    return model.Sample(smp, input.uv) * float4(diffB, diffB, diffB, 1);
    
    

    
    
    
    //float3 light = normalize(float3(1, -1, 1));
    ////float bright = dot(input.norm, -light);
    
    //float3 posFromLightVP = input.tpos.xyz / input.tpos.w;
    //float2 shadowUV = (posFromLightVP + float2(1, -1)) * float2(0.5, -0.5);
    //float depthFromLight = lightmap.Sample(smp, shadowUV);
    //float shadowWeight = 1.0f;
    //if(depthFromLight < posFromLightVP.z)
    //{
    //    shadowWeight = 0.5f;
    //}
    ////shadowWeight = lerp(0.5f, 1.0f, depthFromLight);
    //float4 b = shadowWeight * model.Sample(smp, input.uv);
    
    //return b/*float4(b, b, b, 1)*/;
    
    
    
    
    
    // PAL(RGBからグレースケールYを得る企画)
    //float Y = dot(col.rgb, float3(0.299, 0.587, 0.114));
    //return float4(Y, Y, Y, 1);
    
    // 色の反転
    //return float4(float3(1.0f, 1.0f, 1.0f) - col.rgb, col.a);

    // 色の階調を落とす *ティザ併用などしないと出力結果がイマイチ
    //return float4(col.rgb - fmod(col.rgb, 0.25f), col.a);
    
    
    float w, h, levels;
    model.GetDimensions(0, w, h, levels);
    
    float dx = 1.0f / w;
    float dy = 1.0f / h;
    int offset = 2;
    float4 ret = float4(0, 0, 0, 0);
    
    // 画素の平均化によるぼかし処理
    //// 左上、上、右上
    //ret += model.Sample(smp, input.uv + float2(-offset * dx, offset* dy));
    //ret += model.Sample(smp, input.uv + float2(0, offset * dy));
    //ret += model.Sample(smp, input.uv + float2(offset * dx, offset * dy));
    
    //// 左、真ん中(自分)、右
    //ret += model.Sample(smp, input.uv + float2(-offset * dx, 0));
    //ret += model.Sample(smp, input.uv);
    //ret += model.Sample(smp, input.uv + float2(offset * dx, 0));
    
    ////左下、下、右下
    //ret += model.Sample(smp, input.uv + float2(-offset * dx, -offset * dy));
    //ret += model.Sample(smp, input.uv + float2(0, -offset * dy));
    //ret += model.Sample(smp, input.uv + float2(offset * dx, -offset * dy));
    
    //return ret / 9.0f;
    
    // エンボス
    // 左上、上、右上
    //ret += model.Sample(smp, input.uv + float2(-offset * dx, offset * dy)) * 2;
    //ret += model.Sample(smp, input.uv + float2(0, offset * dy));
    //ret += model.Sample(smp, input.uv) * 0;
    
    //// 左、真ん中(自分)、右
    //ret += model.Sample(smp, input.uv + float2(-offset * dx, 0));
    //ret += model.Sample(smp, input.uv);
    //ret += model.Sample(smp, input.uv + float2(offset * dx, 0)) * -1;
    
    ////左下、下、右下
    //ret += model.Sample(smp, input.uv + float2(-offset * dx, -offset * dy)) * 0;
    //ret += model.Sample(smp, input.uv + float2(0, -offset * dy)) * -1;
    //ret += model.Sample(smp, input.uv + float2(offset * dx, -offset * dy)) * -2;
    
    //return ret;
    
    // シャープネス
    //ret += model.Sample(smp, input.uv + float2(-offset * dx, offset * dy)) * 0;
    //ret += model.Sample(smp, input.uv + float2(0, offset * dy)) * -1;
    //ret += model.Sample(smp, input.uv) * 0;
    
    //// 左、真ん中(自分)、右
    //ret += model.Sample(smp, input.uv + float2(-offset * dx, 0)) * -1;
    //ret += model.Sample(smp, input.uv) * 4;
    //ret += model.Sample(smp, input.uv + float2(offset * dx, 0)) * -1;
    
    ////左下、下、右下
    //ret += model.Sample(smp, input.uv + float2(-offset * dx, -offset * dy)) * 0;
    //ret += model.Sample(smp, input.uv + float2(0, -offset * dy)) * -1;
    //ret += model.Sample(smp, input.uv + float2(offset * dx, -offset * dy)) * 0;
    
    //float Y = dot(ret.rgb, float3(0.299, 0.587, 0.114));
    //Y = pow(1.0f - Y, 10.0f);
    //Y = step(0.2, Y);
    //return float4(Y, Y, Y, col.a);
    
    // 簡易版ガウシアンぼかし
    // 最上段
    //ret += model.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) * 1;
    //ret += model.Sample(smp, input.uv + float2(-1 * dx, 2 * dy)) * 4;
    //ret += model.Sample(smp, input.uv + float2(0 * dx, 2 * dy)) * 6;
    //ret += model.Sample(smp, input.uv + float2(1 * dx, 2 * dy)) * 4;
    //ret += model.Sample(smp, input.uv + float2(2 * dx, 2 * dy)) * 1;
    //// 上段
    //ret += model.Sample(smp, input.uv + float2(-2 * dx, 1 * dy)) * 4;
    //ret += model.Sample(smp, input.uv + float2(-1 * dx, 1 * dy)) * 16;
    //ret += model.Sample(smp, input.uv + float2(0 * dx, 1 * dy)) * 24;
    //ret += model.Sample(smp, input.uv + float2(1 * dx, 1 * dy)) * 16;
    //ret += model.Sample(smp, input.uv + float2(2 * dx, 1 * dy)) * 4;
    //// 中段
    //ret += model.Sample(smp, input.uv + float2(-2 * dx, 0 * dy)) * 6;
    //ret += model.Sample(smp, input.uv + float2(-1 * dx, 0 * dy)) * 24;
    //ret += model.Sample(smp, input.uv + float2(0 * dx, 0 * dy)) * 36;
    //ret += model.Sample(smp, input.uv + float2(1 * dx, 0 * dy)) * 24;
    //ret += model.Sample(smp, input.uv + float2(2 * dx, 0 * dy)) * 6;
    //// 下段
    //ret += model.Sample(smp, input.uv + float2(-2 * dx, -1 * dy)) * 4;
    //ret += model.Sample(smp, input.uv + float2(-1 * dx, -1 * dy)) * 16;
    //ret += model.Sample(smp, input.uv + float2(0 * dx, -1 * dy)) * 24;
    //ret += model.Sample(smp, input.uv + float2(1 * dx, -1 * dy)) * 16;
    //ret += model.Sample(smp, input.uv + float2(2 * dx, -1 * dy)) * 4;
    //// 最下段
    //ret += model.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 1;
    //ret += model.Sample(smp, input.uv + float2(-1 * dx, -2 * dy)) * 4;
    //ret += model.Sample(smp, input.uv + float2(0 * dx, -2 * dy)) * 6;
    //ret += model.Sample(smp, input.uv + float2(1 * dx, -2 * dy)) * 4;
    //ret += model.Sample(smp, input.uv + float2(2 * dx, -2 * dy)) * 1;
 
    //return ret / 256;
    
    // ガウシアンぼかし
    col = model.Sample(smp, input.uv);
    ret += bkweights[0] * col; //0000, 0
    
    // 横方向
    //for (int i = 1; i < 8;++i)
    //{
    //    ret += bkweights[i >> 2][i % 4] * model.Sample(smp, input.uv + float2(i * dx, 0));        
    //    //0000, 1
    //    //0000, 2
    //    //0000, 3
    //    //0001, 0
    //    //0001, 1
    //    //0001, 2
    //    //0001, 3の並び。次の行は-iより-1～-8まで
    //    ret += bkweights[i >> 2][i % 4] * model.Sample(smp, input.uv + float2(-i * dx, 0));
    //}
    
    //// 縦方向
    //for (int i = 1; i < 8; ++i)
    //{
    //    ret += bkweights[i >> 2][i % 4] * model.Sample(smp, input.uv + float2(0, 1 * dy));
    //    //0000, 1
    //    //0000, 2
    //    //0000, 3
    //    //0001, 0
    //    //0001, 1
    //    //0001, 2
    //    //0001, 3の並び。次の行は-iより-1～-8まで
    //    ret += bkweights[i >> 2][i % 4] * model.Sample(smp, input.uv + float2(0, -i * dy));
    //}
   
    // 脳線マップ利用：画面にひび
    float2 normalTex = normalmap.Sample(smp, input.uv);
    normalTex = normalTex * 2.0f - 1.0f; // 法線マップは法線方向Nx,Ny,Nzをそれぞれ(1+Nx,y,z)/2として画像データとしているので、法線方向データに戻す
    
    for (int i = 1; i < 8; ++i)
    {
        ret += bkweights[i >> 2][i % 4] * model.Sample(smp, input.uv + normalTex * 0.1f);
        //0000, 1
        //0000, 2
        //0000, 3
        //0001, 0
        //0001, 1
        //0001, 2
        //0001, 3の並び。次の行は-iより-1～-8まで
        ret += bkweights[i >> 2][i % 4] * model.Sample(smp, input.uv + normalTex * 0.1f);
    }
    
    //return float4(ret.rgb / 2, ret.a);
    
    col = model.Sample(smp, input.uv + normalTex * 0.1f) + tex.Sample(smp, input.uv/* + normalTex * 0.5f*/);
    
    return col;

}
