/*
============================================================

============================================================
*/


//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------

#define WIDTH 1024


Texture2D txDiffuse : register( t0 );
StructuredBuffer<float2> warpMaps : register( t1 );
Texture2D txShadowMap : register( t2 );

SamplerState samLinear : register( s0 );
SamplerState samBackbuffer : register( s1 );
SamplerState samDepth : register( s2 );


cbuffer cbChangesEveryFrame : register( b0 )
{
    matrix World : packoffset(c0);
    matrix View : packoffset(c4);
    matrix Projection : packoffset(c8);
	matrix mvp : packoffset(c12);
    matrix mvpInv : packoffset(c16);
    matrix mvpLight : packoffset(c20);

    float4 vMeshColor : packoffset(c24);
};


/*
==============================================================
Ordinary shader - render to G-Buffer


==============================================================
*/


struct VS_INPUT
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
    noperspective centroid float4 LightPos : TEXCOORD1;
};



PS_INPUT VS( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = mul(input.Pos, mvp);
    output.Tex = input.Tex;


    //some frontpass warping
    float4 rpos = mul(input.Pos, mul(World, mvpLight));
    rpos /= rpos.w;
    rpos.xy = mad(float2(rpos.x, rpos.y), 0.5, 0.5);

    float2 indexS = rpos.xy * WIDTH;
    float2 warps = float2(
        lerp(warpMaps[floor(indexS.x)].y, warpMaps[floor(indexS.x) + 1].y, frac(indexS.x)),
        lerp(warpMaps[floor(indexS.y)].x, warpMaps[floor(indexS.y) + 1].x, frac(indexS.y))
        );
    rpos.xy += warps * 0.75;
    
    output.LightPos = rpos;

    return output;
}


float4 PS( PS_INPUT input ) : SV_Target
{
    float2 depthTexCoord = float2(input.LightPos.x, 1- input.LightPos.y);
    float depth = txShadowMap.Sample(samDepth, depthTexCoord).r;
    float projDepth = saturate(input.LightPos.z);


    float shadow = projDepth > depth ? 0.5 : 1;

    if (depthTexCoord.x < 0 || depthTexCoord.x >= 1.0f
        || depthTexCoord.y < 0 || depthTexCoord.y >= 1.0f)
        shadow = 1;

    float4 res = float4(txDiffuse.Sample(samLinear, input.Tex).xyz, 1);

    return res * shadow;
}


/*
==============================================================
RTW shader

render to shadow map
==============================================================
*/



struct VS_RTW_INPUT
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
};

struct PS_RTW_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
    float2 Warp: TEXCOORD1;
};


PS_RTW_INPUT VS_RTW(VS_RTW_INPUT input)
{
    PS_RTW_INPUT output = (PS_RTW_INPUT)0;
    output.Pos = mul(input.Pos, mvp);
    output.Pos /= output.Pos.w;

    // warping
    float2 indexS = mad(output.Pos.xy, 0.5, 0.5) * WIDTH;
    float2 warps = float2(
        lerp(warpMaps[floor(indexS.x)].y, warpMaps[floor(indexS.x) + 1].y, frac(indexS.x)),
        lerp(warpMaps[floor(indexS.y)].x, warpMaps[floor(indexS.y) + 1].x, frac(indexS.y))
        );
    output.Pos.xy += warps * 1.5;
    output.Warp = warps;

    output.Tex = input.Tex;

    return output;
}

float4 PS_RTW(PS_RTW_INPUT input) : SV_Target
{
    return float4(input.Warp.xy, 1, 1);
    //return float4(txDiffuse.Sample(samLinear, input.Tex).xyz, 1) * vMeshColor;
}



/*
==============================================================
Quad shader


==============================================================
*/

PS_INPUT VS_QUAD(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;

    output.Pos = float4(input.Pos.xy, 0, 1);
    output.Tex = input.Tex;

    return output;
}

float4 PS_QUAD(PS_INPUT input) : SV_Target
{
    return float4(txDiffuse.SampleLevel(samLinear, input.Tex, 0).xyz, 1);
}
