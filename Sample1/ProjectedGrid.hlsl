

#define width 128

struct Pos
{
    float4 pos;
};


RWStructuredBuffer<Pos> gridBuffer : register(u0);

cbuffer cbFrame : register(b0)
{
    float4 dTexCoord;

    float4 vCorner0;
    float4 vCorner1;
    float4 vCorner2;
    float4 vCorner3;
};


[numthreads(16, 16, 1)]
void CSMain(uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex)
{
    int index = DTid.x + DTid.y * width;

    float4 result;
    float v = DTid.y * dTexCoord.y;
    float u = DTid.x * dTexCoord.x;

    result.x = (1.0f - v)*((1.0f - u)*vCorner0.x + u * vCorner1.x) +
        v*((1.0f - u)*vCorner2.x + u*vCorner3.x);
    result.w = (1.0f - v)*((1.0f - u)*vCorner0.w + u * vCorner1.w) +
        v*((1.0f - u)*vCorner2.w + u*vCorner3.w);
    result.z = (1.0f - v)*((1.0f - u)*vCorner0.z + u * vCorner1.z) +
        v*((1.0f - u)*vCorner2.z + u*vCorner3.z);

    float divide = 1.0f / result.w;
    result.x *= divide;
    result.z *= divide;
    result.y = 0;

    gridBuffer[index].pos = float4(result.xyz, 1);
}