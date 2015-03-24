

#define WIDTH 1024


SamplerState samLinear : register(s0);
SamplerState samBackbuffer : register(s1);
SamplerState samDepth : register(s2);


RWStructuredBuffer<float2> outBuffer : register(u0);

StructuredBuffer<float2> inBuffer : register(t0);


[numthreads(8, 1, 1)] // dispatch 128,1,1 - we have 1024 x calls
void CalcWarp(uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex)
{
    //formula from Paul Rosen's paper
    float2 totalSum = 0;
    float2 partialSum = 0;

    for (int i = 0; i < WIDTH; i++)
    {
        totalSum += inBuffer[i].xy;
        if (i == DTid.x)
            partialSum = totalSum;
    }
    
    outBuffer[DTid.x] = (partialSum / totalSum - float2(DTid.xx) / WIDTH);
}
