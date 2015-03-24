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
    float depth = txDepth.SampleLevel(samDepth, input.Tex, 0).r;
    float2 cspos = float2(input.Tex.x * 2 - 1, (1 - input.Tex.y) * 2 - 1);
    float4 depthCoord = float4(cspos.xy, depth, 1);
    depthCoord = mul(depthCoord, matMVPInv);
    depthCoord /= depthCoord.w;
    depthCoord.w = 1;

    // reprojection
    float4 ncoord = mul(depthCoord, matMVPLight);
    ncoord.xyz /= ncoord.w;
    ncoord.xy = mad(ncoord.xy, 0.5, 0.5);

    // warping
    int2 index = ncoord.xy * WIDTH;
    float2 warps = float2(warpMaps[index.x].y, warpMaps[index.y].x);
    ncoord.xy += warps*0.5;

    float shadowDepth = txShadowMap.SampleLevel(samDepth, float2(ncoord.xy), 0).r;
    //float shadowDepth = txShadowMap.SampleLevel(samDepth, float2(input.Tex.xy), 0).r;

    //return float4(ncoord.zzz, 1);

    float shadow = ncoord.z > shadowDepth + 0.0002 ? 0.5 : 1;

    return float4(txDiffuse.Sample( samLinear, input.Tex ).xyz, 1) * shadow;
}