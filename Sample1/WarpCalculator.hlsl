

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


[numthreads(8, 1, 1)]
void BlurWarp(uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex)
{
    float coeffs[7] = { 0.127325, 0.107778, 0.081638, 0.055335, 0.033562, 0.018216, 0.008847 };
    float2 sum = inBuffer[DTid.x] *0.134598;

    // not very 
    // go along arrays and blur
    [unroll]
    for (int i = 0; i < 7; i++)
    {
        int ind_pos = min(DTid.x + i + 1, WIDTH);
        int ind_neg = max(0, DTid.x - i - 1);

        sum += (inBuffer[ind_pos] + inBuffer[ind_neg])*coeffs[i];
    }
    
    outBuffer[DTid.x] = sum;
}

