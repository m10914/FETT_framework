


#define WIDTH 1024


SamplerState samLinear : register(s0);
SamplerState samBackbuffer : register(s1);
SamplerState samDepth : register(s2);

RWStructuredBuffer<float2> outBuffer : register(u0); //as soon as x == y, we can do this trick - unite two buffers into one


StructuredBuffer<int> inBuffer : register(t0);


[numthreads(8, 1, 1)] // dispatch 128,1,1 - we have 1024 x calls
void CalcImportance(uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex)
{
	// importance was calculated in Reprojection
	// now all we need is to gather elements
	float2 res = 0;
	for (int i = 0; i < WIDTH; i++)
	{
		res.x = max(res.x, float(inBuffer[int(DTid.x*WIDTH) + i]) / 100000.0);
		res.y = max(res.y, float(inBuffer[DTid.x + int(i*WIDTH)]) / 100000.0);
	}
	outBuffer[DTid.x] = res.xy;
}
