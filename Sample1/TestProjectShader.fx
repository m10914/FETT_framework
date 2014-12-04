/*
============================================================

============================================================
*/

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
Texture2D txDiffuse : register( t0 );
Texture2D txBackbuffer : register( t1 );
Texture2D txDepth : register( t2 );

SamplerState samLinear : register( s0 );
SamplerState samBackbuffer : register( s1 );
SamplerState samDepth : register( s2 );


cbuffer cbNeverChanges : register( b0 )
{ 
};

cbuffer cbChangeOnResize : register( b1 )
{
    matrix Projection;
	float4 vScreenParams;
};

cbuffer cbChangesEveryFrame : register( b2 )
{
    matrix World;
    float4 vMeshColor;
	matrix View;
};



static const float stepSize = 0.01;
static const float maxDelta = 0.01;   // Delta depth test value
static const float fade = 5.0;



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
    output.Pos = mul( input.Pos, World );
    output.Pos = mul( output.Pos, View );
    output.Pos = mul( output.Pos, Projection );
    
	output.Tex = input.Tex;
    
    return output;
}


float4 PS( PS_INPUT input) : SV_Target
{
    return txDiffuse.Sample( samLinear, input.Tex ) * vMeshColor;
}



/*
==============================================================
Reflection shader


==============================================================
*/

struct PS_REF_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
	float4 PosVS : POSITION; //view-space position
};




//math
float3 fromVStoNDC(float3 vec) //after that we get coordinates from 0 to 1
{
	float4 res = mul( float4(vec,1), Projection );
	return mad( res.xyz / res.w, 0.5, 0.5);
}
float linearizeDepth(float depth)
{
    return (2.0 * vScreenParams.z) / (vScreenParams.w + vScreenParams.z - depth * (vScreenParams.w - vScreenParams.z));
}

float4 raytrace(in float3 startPos, in float3 endPos)
{
	// Convert start and end positions of reflect vector from the
	// camera space to the screen space

    float3 startPosSS = fromVStoNDC( startPos );
    float3 endPosSS = fromVStoNDC( endPos );

    // Reflection vector in the screen space
    float3 vectorSS = normalize(endPosSS.xyz - startPosSS.xyz)*stepSize;
    
    // cycle
    float2 samplePos = 0;   // texcoord for the depth and color
    float sampleDepth = 0;  // depth from texture
    float currentDepth = 0; // current depth calculated with reflection vector
    float deltaD = 0;
    float4 color = float4(0,0,0,1);
    for (int i = 1; i < 64; i++)
    {
        samplePos = (startPosSS.xy + vectorSS.xy*i);
        currentDepth = linearizeDepth(startPosSS.z + vectorSS.z*i);        
        sampleDepth = linearizeDepth( txDepth.Sample( samDepth, samplePos ).r );
        deltaD = currentDepth - sampleDepth;
        if ( deltaD > -maxDelta && deltaD < maxDelta )
        {
            color = txBackbuffer.Sample( samBackbuffer, samplePos );
            color.a *= fade / i;
            break;
        }
    }
    return color;
}


PS_REF_INPUT VS_Reflection( VS_INPUT input )
{
    PS_REF_INPUT output = (PS_REF_INPUT)0;
    output.Pos = mul( float4(input.Pos.xyz,1), World );
    output.Pos = mul( output.Pos, View );
	output.PosVS = output.Pos;

    output.Pos = mul( output.Pos, Projection );   

	output.Tex = input.Tex;

    return output;
}


float4 PS_Reflection( PS_REF_INPUT input ) : SV_Target
{
	float2 sTexCoords = input.Pos.xy / vScreenParams.xy;

	// now we have spos - texture coordinates
	float3 NormalVS = normalize( mul( float4(0,1,0,0), View).xyz );
	float3 VecVS = normalize( reflect( input.PosVS.xyz, NormalVS ) ) / 2;

	//return float4( mad(NormalVS,0.5,0.5).xyz, 1);

	return raytrace(input.PosVS, input.PosVS + VecVS);
}