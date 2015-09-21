/*
============================================================

============================================================
*/


//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------


Texture2D txDiffuse : register( t0 );


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
};


struct HS_INPUT
{
    float3 vPosWS        : POSITION;
    float2 vTexCoord    : TEXCOORD0;
};

struct HS_OUTPUT
{
    float Edges[3] : SV_TessFactor;
    float Inside   : SV_InsideTessFactor;
};

struct HS_CONTROL_POINT_OUTPUT
{
    float3 vWorldPos    : WORLDPOS;
    float2 vTexCoord    : TEXCOORD0;
};




//-----------------------------------------------------------------------------
// V E R T E X

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;

    output.Pos = mul(input.Pos, World);
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    output.Tex = input.Tex;

    return output;
}

float4 PS( PS_INPUT input ) : SV_Target
{
    float4 res = float4(
        txDiffuse.Sample(samLinear, input.Tex).xyz,
    1);
    return res;
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

    output.Tex = input.Tex;

    return output;
}

float4 PS_RTW(PS_RTW_INPUT input) : SV_Target
{
    //return float4(input.Warp.xy, 1, 1);
    return float4(txDiffuse.Sample(samLinear, input.Tex).xyz, 1);
}


