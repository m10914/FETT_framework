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


//-----------------------------------------------------------------
// TRAILS

struct TR_VS_INPUT
{
    float3 pos : POSITION;
    float3 pos2 : TEXCOORD0;
    float3 upvec : TEXCOORD1;
    float3 upvec2 : TEXCOORD2;
    float2 uv : TEXCOORD3;
    float2 uv2 : TEXCOORD4;
};

struct TR_GS_INPUT
{
    float4 pos1 : SV_Position;
    float4 pos2 : TEXCOORD0;
    float4 pos3 : TEXCOORD1;
    float4 pos4 : TEXCOORD2;
    float2 uv : TEXCOORD3;
    float2 uv2 : TEXCOORD4;
};

struct TR_PS_INPUT
{
    float4 hpos : SV_Position;
    float2 uv : TEXCOORD0;
};

float4 calcMVP(float3 inVec)
{
    float4 output;

    output = mul(float4(inVec, 1), Projection);
    output /= output.w;

    return output;
}

TR_GS_INPUT trail_vs(TR_VS_INPUT input)
{
    TR_GS_INPUT output = (TR_GS_INPUT)0;

    float4 vsPos1 = mul(float4(input.pos, 1), World);
    vsPos1 = mul(vsPos1, View);

    float4 vsPos2 = mul(float4(input.pos2, 1), World);
    vsPos2 = mul(vsPos2, View);

    float3 tang1 = normalize( cross(vsPos2.xyz - vsPos1.xyz, vsPos1.xyz) );
    float3 tang2 = normalize( cross(vsPos2.xyz - vsPos1.xyz, vsPos2.xyz) );

    output.pos1 = calcMVP(vsPos1 + tang1);
    output.pos2 = calcMVP(vsPos1 - tang1);
    output.pos3 = calcMVP(vsPos2 + tang2);
    output.pos4 = calcMVP(vsPos2 - tang2);

    //textures
    output.uv = input.uv;
    output.uv2 = input.uv2;

    return output;
}


[maxvertexcount(4)]
void trail_gs(point TR_GS_INPUT inStream[1], inout TriangleStream<TR_PS_INPUT> outStream)
{
    TR_PS_INPUT OUT;

    OUT.hpos = inStream[0].pos1;
    OUT.uv = float2(inStream[0].uv);
    outStream.Append(OUT);

    OUT.hpos = inStream[0].pos3;
    OUT.uv = float2(inStream[0].uv2);
    outStream.Append(OUT);

    OUT.hpos = inStream[0].pos2;
    OUT.uv = float2(inStream[0].uv + float2(0,1));
    outStream.Append(OUT);

    OUT.hpos = inStream[0].pos4;
    OUT.uv = float2(inStream[0].uv2 + float2(0,1));
    outStream.Append(OUT);

    outStream.RestartStrip();
}



float4 trail_ps(TR_PS_INPUT IN) : SV_Target
{
    float4 res = float4(
        txDiffuse.Sample(samLinear, IN.uv).xyz,
    1);
    return float4(res.xyz,1);
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


