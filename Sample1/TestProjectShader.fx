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

HS_INPUT VS(VS_INPUT input)
//PS_INPUT VS(VS_INPUT input)
{
    HS_INPUT output = (HS_INPUT)0;

    float4 wpos = input.Pos;
    wpos = mul(wpos, World);
    wpos = mul(wpos, View);
    output.vPosWS = wpos;

    output.vTexCoord = input.Tex;

    /*PS_INPUT output = (PS_INPUT)0;

    output.Pos = mul(input.Pos, World);
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    output.Tex = input.Tex;*/

    return output;
}


//-----------------------------------------------------------------------------
// H U L L

HS_OUTPUT HS_Ord(InputPatch<HS_INPUT, 3> p, uint PatchID : SV_PrimitiveID)
{
    HS_OUTPUT Out;

    Out.Edges[0] = 2;
    Out.Edges[1] = 2;
    Out.Edges[2] = 2;

    Out.Inside = 3;

    return Out;
}


[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("HS_Ord")]
[maxtessfactor(64.0)]
HS_CONTROL_POINT_OUTPUT HS(
    InputPatch<HS_INPUT, 3> inputPatch,
    uint uCPID : SV_OutputControlPointID)
{
    HS_CONTROL_POINT_OUTPUT Out;

    // Copy inputs to outputs
    Out.vWorldPos = inputPatch[uCPID].vPosWS.xyz;
    Out.vTexCoord = inputPatch[uCPID].vTexCoord;

    return Out;
}

//-----------------------------------------------------------------------------
// D O M A I N

[domain("tri")]
PS_INPUT DS(
    HS_OUTPUT input,
    float3 BarycentricCoordinates : SV_DomainLocation,
    const OutputPatch<HS_CONTROL_POINT_OUTPUT, 3> TrianglePatch)
{
    PS_INPUT Out;

    //interpolate stuff
    float3 vWorldPos = 
        BarycentricCoordinates.x * TrianglePatch[0].vWorldPos +
        BarycentricCoordinates.y * TrianglePatch[1].vWorldPos +
        BarycentricCoordinates.z * TrianglePatch[2].vWorldPos;
    float2 vTexCoord = 
        BarycentricCoordinates.x * TrianglePatch[0].vTexCoord +
        BarycentricCoordinates.y * TrianglePatch[1].vTexCoord +
        BarycentricCoordinates.z * TrianglePatch[2].vTexCoord;

    Out.Pos = mul(vWorldPos, Projection);
    Out.Tex = vTexCoord;

    return Out;
}

//-----------------------------------------------------------------------------


float4 PS( PS_INPUT input ) : SV_Target
{
    float4 res = float4(txDiffuse.Sample(samLinear, input.Tex).xyz, 1);
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
    //return float4(input.Warp.xy, 1, 1);
    return float4(txDiffuse.Sample(samLinear, input.Tex).xyz, 1);
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
