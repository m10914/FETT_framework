/*
============================================================

============================================================
*/


//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------

#define WIDTH 1024

Texture2D txDiffuse : register( t0 );
Texture2D txDepth : register( t1 );
Texture2D txShadowMap : register(t2);
StructuredBuffer<float2> warpMaps : register( t3 );


SamplerState samLinear : register( s0 );
SamplerState samBackbuffer : register( s1 );
SamplerState samDepth : register( s2 );


cbuffer cbDefault : register( b0 )
{
    matrix matMVPLight : packoffset(c0);
    matrix matMVPInv : packoffset(c4);
};


/*
==============================================================
Ordinary shader

==============================================================
*/


struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
};



float4 PS( PS_INPUT input ) : SV_Target
{
    // reconstruction
    float depth = txDepth.Sample(samDepth, input.Tex).r;
    float2 cspos = float2(input.Tex.x * 2 - 1, (1 - input.Tex.y) * 2 - 1);
    float4 depthCoord = float4(cspos.xy, depth, 1);
    depthCoord = mul(depthCoord, matMVPInv);
    depthCoord /= depthCoord.w;

    // reprojection
    float4 ncoord = mul(depthCoord, matMVPLight);
    ncoord.xyz /= ncoord.w;
    ncoord.xy = mad(ncoord.xy, 0.5, 0.5);

    // warping
    float2 indexS = ncoord.xy * WIDTH;
        float2 warps = float2(
        lerp(warpMaps[ceil(indexS.x)].y, warpMaps[ceil(indexS.x) + 1].y, frac(indexS.x)),
        lerp(warpMaps[ceil(indexS.y)].x, warpMaps[ceil(indexS.y) + 1].x, frac(indexS.y))
        );
    ncoord.xy += warps;


    float shadowDepth = txShadowMap.Sample(samDepth, float2(ncoord.x, 1-ncoord.y)).r;

    float shadow = ncoord.z > shadowDepth ? 0.5 : 1;
    if (ncoord.x > 1 || ncoord.y > 1 || ncoord.x < 0 || ncoord.y < 0)
        shadow = 1;

    return float4(txDiffuse.Sample( samLinear, input.Tex ).xyz, 1) * shadow;
}