


#define WIDTH 1024
#define SQUARE 1048576


SamplerState samLinear : register(s0);
SamplerState samBackbuffer : register(s1);
SamplerState samDepth : register(s2);

Texture2D txDepthBuffer : register(t0);

RWStructuredBuffer<float> outBuffer : register(u0);


cbuffer cbDefault : register(b0)
{
	matrix matMVPInv;
	matrix matMVPLight;
};

[numthreads(16, 16, 1)]
void CSMain(uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex)
{
	float2 texCoord = float2(float2(DTid.xy) / WIDTH);
	float depth = txDepthBuffer.SampleLevel(samLinear, texCoord, 0).r;
	//int debugIndex = DTid.x + DTid.y * WIDTH;

	// reconstruction
	float2 cspos = float2(texCoord.x * 2 - 1, (1 - texCoord.y) * 2 - 1);
	float4 depthCoord = float4(cspos.xy, depth, 1);
	depthCoord = mul(depthCoord, matMVPInv);
	depthCoord /= depthCoord.w;
	depthCoord.w = 1;


	// reprojection
	float4 ncoord = mul(depthCoord, matMVPLight);
	ncoord /= ncoord.w;
	ncoord.xy = mad(ncoord.xy, 0.5, 0.5);
	
	int index = int(ncoord.x * WIDTH) + int(ncoord.y * WIDTH) * WIDTH;

	//outBuffer[debugIndex] = float(index) / SQUARE;

	if (index < SQUARE && index >= 0)
		outBuffer[index] = max(outBuffer[index], 1 - depth);
}