/*
============================================================

============================================================
*/


//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------



Texture2D txDiffuse : register( t0 );
StructuredBuffer<float2> warpMaps : register( t1 );


SamplerState samLinear : register( s0 );
SamplerState samBackbuffer : register( s1 );
SamplerState samDepth : register( s2 );


cbuffer cbChangesEveryFrame : register( b0 )
{
    matrix World : packoffset(c0);
    matrix View : packoffset(c4);
    matrix Projection : packoffset(c8);
	matrix mvp : packoffset(c12);

    float4 vMeshColor : packoffset(c16);
};


/*
==============================================================
Ordinary shader


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



PS_INPUT VS( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;
	output.Pos = mul(input.Pos, mvp);
  
	output.Tex = input.Tex;
    
    return output;
}


float4 PS( PS_INPUT input) : SV_Target
{
    return float4(txDiffuse.Sample( samLinear, input.Tex ).xyz, 1) * vMeshColor;
}


/*
==============================================================
RTW shader
vertex only (use ordinary PS with this one)
==============================================================
*/

#define WIDTH 1024

PS_INPUT VS_RTW(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = mul(input.Pos, mvp);
    output.Pos /= output.Pos.w;
    output.Pos.w = 1;

    // warping
    int2 index = mad(output.Pos.xy, 0.5, 0.5) * WIDTH;
    float2 warps = float2(warpMaps[index.x].y, warpMaps[index.y].x);
    output.Pos.xy += warps;

    output.Tex = input.Tex;

    return output;
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
