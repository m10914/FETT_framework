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

struct Pos
{
    float4 pos;
};
StructuredBuffer<Pos> gridBuffer : register( t3 );

SamplerState samLinear : register( s0 );
SamplerState samBackbuffer : register( s1 );
SamplerState samDepth : register( s2 );


cbuffer cbChangesEveryFrame : register( b0 )
{
    matrix World;
    matrix View;
    matrix Projection;

    float4 vScreenParams;
	float4 PerspectiveValues;

    float4 vMeshColor;
	float4 SSRParams; //x - numsteps, y - depth bias, z - pixelsize, w - reserved	
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


struct VS_INPUT_WAT
{
    uint id : SV_VERTEXID;
};


PS_INPUT VS_WAT(VS_INPUT_WAT input)
{
    PS_INPUT output = (PS_INPUT)0;

    output.Pos = mul(gridBuffer[input.id].pos, World);
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);

    return output;
}



float4 PS_WAT( PS_INPUT input ) : SV_Target
{
    return txDiffuse.Sample( samLinear, input.Tex ) * vMeshColor;
}
