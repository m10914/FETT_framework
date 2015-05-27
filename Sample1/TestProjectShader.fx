/*
============================================================

============================================================
*/


//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------

struct Pos
{
    float4 pos;
};



Texture2D txDiffuse : register( t0 );
Texture2D txBackbuffer : register( t1 );
Texture2D txDepth : register( t2 );

StructuredBuffer<Pos> gridBuffer : register(t3);

Texture2D txDisplacement : register(t4);
Texture2D txRipple : register(t5);
Texture1D txFresnel : register(t6);
Texture2D txPerlin : register(t7);


SamplerState samLinear : register( s0 );
SamplerState samBackbuffer : register( s1 );
SamplerState samDepth : register( s2 );


cbuffer cbChangesEveryFrame : register( b0 )
{
    matrix World : packoffset(c0.x);
    matrix View : packoffset(c4.x);
    matrix Projection : packoffset(c8.x);

    float4 vScreenParams : packoffset(c12.x);
    float4 PerspectiveValues : packoffset(c13.x);

    float4 vMeshColor : packoffset(c14.x);
    float4 SSRParams : packoffset(c15.x); //x - numsteps, y - depth bias, z - pixelsize, w - reserved	

    float4 eyeVector : packoffset(c16.x);
    float4 sunDirection : packoffset(c17.x);
    float4 waterColor : packoffset(c18.x);
    float4 skyColor : packoffset(c19.x);

    //Perlin stuff
    float		PerlinSize : packoffset(c20.x);
    float3		PerlinAmplitude : packoffset(c21.x);
    float3		PerlinOctave : packoffset(c22.x);
    float3		PerlinGradient : packoffset(c23.x);
    float2      PerlinMovement : packoffset(c24.x);
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
    output.Pos = mul( input.Pos, World );
    output.Pos = mul( output.Pos, View );
    output.Pos = mul( output.Pos, Projection );
    
	output.Tex = input.Tex;
    
    return output;
}


float4 PS( PS_INPUT input) : SV_Target
{
    return float4(txDiffuse.Sample( samLinear, input.Tex ).xyz, 1) * vMeshColor;
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



/*
==============================================================
Reflection shader


==============================================================
*/


struct PS_REF_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
	float4 PosVS : TEXCOORD1;
	float3 NormalVS : TEXCOORD2;
	noperspective float3 PosCS : TEXCOORD3;
};




float ConvertZToLinearDepth(float depth)
{
	float linearDepth = PerspectiveValues.z / (depth + PerspectiveValues.w);
	return linearDepth;
}

float3 CalcViewPos(float2 csPos, float depth)
{
	float3 position;

	position.xy = csPos.xy * PerspectiveValues.xy * depth;
	position.z = depth;

	return position;
}

PS_REF_INPUT VS_Reflection( VS_INPUT input )
{
    PS_REF_INPUT output = (PS_REF_INPUT)0;
    output.Pos = mul( float4(input.Pos.xyz,1), World );
    output.Pos = mul( output.Pos, View );
	output.PosVS = output.Pos;
    output.Pos = mul( output.Pos, Projection );

	output.Tex = input.Tex;

	output.PosCS = output.Pos.xyz / output.Pos.w;

	output.NormalVS = mul( float3(0,1,0), (float3x3)View );

    return output;
}


float4 PS_Reflection( PS_REF_INPUT input ) : SV_Target
{
	int nNumSteps = SSRParams.x;
	float DepthBias = SSRParams.y;
	float PixelSize = SSRParams.z;

	// Pixel position and normal in view space
	float3 vsPos = input.PosVS.xyz;
	float3 vsNorm = normalize(input.NormalVS);

	// Calculate the camera to pixel direction
	float3 eyeToPixel = normalize(vsPos);

	// Calculate the reflected view direction
	float3 vsReflect = reflect(eyeToPixel,  vsNorm);

	// The initial reflection color for the pixel
	float4 reflectColor = float4(0.0, 0.0, 0.0, 0.0);

	// Transform the View Space Reflection to clip-space
	float3 vsPosReflect = vsPos + vsReflect;
	float3 csPosReflect = mul(float4(vsPosReflect, 1.0), Projection).xyz / vsPosReflect.z;
	float3 csReflect = csPosReflect - input.PosCS;

	// Resize Screen Space Reflection to an appropriate length.
	float reflectScale = PixelSize / length(csReflect.xy);
	csReflect *= reflectScale;

	// Calculate the first sampling position in screen-space
	float2 ssSampPos = (input.PosCS + csReflect).xy;
	ssSampPos = ssSampPos * float2(0.5, -0.5) + 0.5;

	// Find each iteration step in screen-space
	float2 ssStep = csReflect.xy * float2(0.5, -0.5);

	// Build a plane laying on the reflection vector
	// Use the eye to pixel direction to build the tangent vector
	float4 rayPlane;
	float3 vRight = cross(eyeToPixel, vsReflect);
	rayPlane.xyz = normalize(cross(vsReflect, vRight));
	rayPlane.w = dot(rayPlane, vsPos);

	//if(input.Pos.x/vScreenParams.x < 0.5)
		//return float4(input.PosCS, 1);

	// Iterate over the HDR texture searching for intersection
	for (int nCurStep = 0; nCurStep < nNumSteps; nCurStep++)
	{
		// Sample from depth buffer
		float curDepth = txDepth.SampleLevel(samDepth, ssSampPos, 0.0).x;

		float curDepthLin = ConvertZToLinearDepth(curDepth);
		float3 curPos = CalcViewPos(input.PosCS.xy + csReflect.xy * ((float)nCurStep + 1.0), curDepthLin);

		// Find the intersection between the ray and the scene
		// The intersection happens between two positions on the oposite sides of the plane
		if(rayPlane.w >= dot(rayPlane.xyz, curPos) + DepthBias)
		{
			// Calculate the actual position on the ray for the given depth value
			float3 vsFinalPos = vsPos + (vsReflect / abs(vsReflect.z)) * abs(curDepthLin - vsPos.z + DepthBias);
			float2 csFinalPos = vsFinalPos.xy / PerspectiveValues.xy / vsFinalPos.z;
			ssSampPos = csFinalPos.xy * float2(0.5, -0.5) + 0.5;

			// Get the HDR value at the current screen space location
			reflectColor.xyz = txBackbuffer.SampleLevel(samBackbuffer, ssSampPos, 0.0).xyz;

			// Advance past the final iteration to break the loop
			nCurStep = nNumSteps;
		}

		// Advance to the next sample
		ssSampPos += ssStep;	
	}


	return float4(0.1, 0.7, 0.1, 1) * 0.6 + 0.4* reflectColor;
}



/*
-----------------------------------------------------
Water shader

-----------------------------------------------------
*/

#define PATCH_BLEND_BEGIN		5
#define PATCH_BLEND_END			45
#define UV_SCALE 0.2



struct VS_INPUT_WAT
{
    uint id : SV_VERTEXID;
};

struct PS_INPUT_WAT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
    float4 PosWS : TEXCOORD1;
};


PS_INPUT_WAT VS_WAT(VS_INPUT_WAT input)
{
    PS_INPUT_WAT output = (PS_INPUT_WAT)0;

    output.Pos = gridBuffer[input.id].pos;

    // intermediate texture coordinates for displacement map
    output.PosWS = output.Pos;

    output.Tex = float2(output.Pos.xz * UV_SCALE);

    output.Pos = mul(output.Pos, View);  
    output.Pos = mul(output.Pos, Projection);

    return output;
}


float4 PS_WAT(PS_INPUT_WAT input) : SV_Target
{
    // constants - move to constant buffer
    // TODO
    float3 vEyeRay = normalize(eyeVector - input.PosWS);
    float3 watColor = float3(0.07f, 0.15f, 0.2f);
    float3 reflectionColor = float3(1, 1, 1);
    float3 sunDir = float3(0.936016f, 0.0780013f, -0.343206f);
    float3 sunColor = float3(1, 1, 0.6);

    // calc normal & reflect
    float2 ripple = txRipple.Sample(samLinear, input.Tex).xy;
   
    
    float dist2d = length(eyeVector.xz - input.PosWS.xz);
    float blendFactor = (PATCH_BLEND_END - dist2d) / (PATCH_BLEND_END - PATCH_BLEND_BEGIN);
    blendFactor = saturate(blendFactor * blendFactor * blendFactor);

    blendFactor = 1;

    // get perlin noise    
    float2 perlin_tc = input.Tex * PerlinSize;
    float2 perlin_tc0 = (blendFactor < 1) ? perlin_tc * PerlinOctave.x - PerlinMovement : 0;
    float2 perlin_tc1 = (blendFactor < 1) ? perlin_tc * PerlinOctave.y - PerlinMovement : 0;
    float2 perlin_tc2 = (blendFactor < 1) ? perlin_tc * PerlinOctave.z - PerlinMovement : 0;

    float2 perlin_0 = txPerlin.Sample(samLinear, perlin_tc0).xy;
    float2 perlin_1 = txPerlin.Sample(samLinear, perlin_tc1).xy;
    float2 perlin_2 = txPerlin.Sample(samLinear, perlin_tc2).xy;
    float2 perlin = (perlin_0 * PerlinGradient.x + perlin_1 * PerlinGradient.y + perlin_2 * PerlinGradient.z);

    // blend perlin and riplle
    ripple = lerp(perlin, ripple, blendFactor);
    float3 normal = normalize(float3(ripple.x, 6.812, ripple.y));

    return float4(ripple.xy, 0, 1);

    //procedural
    //normal = normalize(cross(ddx(input.PosWS), ddy(input.PosWS)));

    float3 vReflect = reflect(-vEyeRay, normal);

    float dotNV = saturate(dot(normal, vEyeRay));

    return float4(dotNV.xxx, 1);

    float4 ramp = txFresnel.Sample(samLinear, dotNV).xyzw; // ramp.x for fresnel term. ramp.y for sky blending

    // Combine waterbody color and reflected color
    float3 surfaceColor = lerp(watColor, reflectionColor, ramp.x);

    // sun blicks
    float specCos = saturate(dot(vReflect, sunDir));
    float sunSpot = pow(specCos, 400);
    surfaceColor += sunColor * sunSpot;


    float4 res = float4(surfaceColor.xyz, 1);
    return res;
}
