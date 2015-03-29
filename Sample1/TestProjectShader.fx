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
	float2 Warps : TEXCOORD2;
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
	output.Warps = warps;

    return output;
}


/*float4 PS( PS_INPUT input ) : SV_Target
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
}*/

#define PCF_NUM_SAMPLES 8 
#define PCF_NUM_SAMPLES_DIV 0.125

static float2 poissonDisk[16] =
{
	float2(-0.94201624, -0.39906216),
	float2(0.94558609, -0.76890725),
	float2(-0.094184101, -0.92938870),
	float2(0.34495938, 0.29387760),
	float2(-0.91588581, 0.45771432),
	float2(-0.81544232, -0.87912464),
	float2(-0.38277543, 0.27676845),
	float2(0.97484398, 0.75648379),
	float2(0.44323325, -0.97511554),
	float2(0.53742981, -0.47373420),
	float2(-0.26496911, -0.41893023),
	float2(0.79197514, 0.19090188),
	float2(-0.24188840, 0.99706507),
	float2(-0.81409955, 0.91437590),
	float2(0.19984126, 0.78641367),
	float2(0.14383161, -0.14100790)
};


float pcfFilter(float2 baseDepthCoord, float zReciever, float filterRadius, float2 warps)
{
	float sum = 0.0f;
	[unroll] for (int i = 0; i < PCF_NUM_SAMPLES; ++i)
	{
		float2 offset = poissonDisk[i] * filterRadius; //+warpsQ
		float depth = txShadowMap.Sample(samDepth, baseDepthCoord + offset).r;
		sum += zReciever > depth ? 0.5 : 1;
	}

	return sum * PCF_NUM_SAMPLES_DIV;
}


float4 PS(PS_INPUT input) : SV_Target
{
	float2 depthTexCoord = float2(input.LightPos.x, 1 - input.LightPos.y);
	float projDepth = saturate(input.LightPos.z);

	float shadow = 1;
	
	if (depthTexCoord.x < 0 || depthTexCoord.x >= 1.0f
		|| depthTexCoord.y < 0 || depthTexCoord.y >= 1.0f)
		shadow = 1;
	else
		shadow = pcfFilter(depthTexCoord, projDepth, 0.001, input.Warps);

	
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
