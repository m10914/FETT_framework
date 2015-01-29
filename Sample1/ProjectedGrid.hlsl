/*
-----------------------------------------------------------------

Projected Grid
compute shader


-----------------------------------------------------------------
*/


#define width 256
#define UV_SCALE 0.2
#define DISPLACE 0.003
#define PATCH_BLEND_BEGIN		5
#define PATCH_BLEND_END			45


SamplerState samLinear : register(s0);
SamplerState samBackbuffer : register(s1);
SamplerState samDepth : register(s2);

struct Pos
{
    float4 pos;
};


RWStructuredBuffer<Pos> gridBuffer : register(u0);

cbuffer cbFrame : register(b0)
{
    // ordinary data
    float4 dTexCoord : packoffset(c0.x);

    float4 vCorner0 : packoffset(c1.x);
    float4 vCorner1 : packoffset(c2.x);
    float4 vCorner2 : packoffset(c3.x);
    float4 vCorner3 : packoffset(c4.x);

    float4x4 World : packoffset(c5.x);
    float3 eyePosition : packoffset(c9.x);

    // Perlin noise for distant wave crest
    float		PerlinSize : packoffset(c10.x);
    float3		PerlinAmplitude : packoffset(c11.x);
    float3		PerlinOctave : packoffset(c12.x);
    float3		PerlinGradient : packoffset(c13.x);
    float2      PerlinMovement : packoffset(c14.x);
};


Texture2D txDisplacement : register(t0);
Texture2D txPerlin : register(t1);


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
    result.w = 1;

    // transmute
    result = mul(result, World);

    float3 vecEye = result.xyz - eyePosition;
    float distXZ = length(vecEye.xz);
    float blendFactor = saturate((PATCH_BLEND_END - distXZ) / (PATCH_BLEND_END - PATCH_BLEND_BEGIN));

    // intermediate texture coordinates for displacement map
    float2 tCoord = float2(result.xz * UV_SCALE);
    float3 displacement = txDisplacement.SampleLevel(samLinear, tCoord, 0).xzy;
    
    // Add perlin noise to distant patches
    float perlin = 0;
    if (blendFactor < 1)
    {
        float2 perlin_tc = tCoord * PerlinSize;
        float perlin_0 = txPerlin.SampleLevel(samLinear, perlin_tc * PerlinOctave.x - PerlinMovement, 0).w;
        float perlin_1 = txPerlin.SampleLevel(samLinear, perlin_tc * PerlinOctave.y - PerlinMovement, 0).w;
        float perlin_2 = txPerlin.SampleLevel(samLinear, perlin_tc * PerlinOctave.z - PerlinMovement, 0).w;

        perlin = perlin_0 * PerlinAmplitude.x + perlin_1 * PerlinAmplitude.y + perlin_2 * PerlinAmplitude.z;
    }

    displacement = lerp(float3(0, perlin*0.2, 0), displacement, blendFactor); // lerp perlin into displacement

    // apply displacement
    result.xyz += displacement * DISPLACE;

    gridBuffer[index].pos = float4(result.xyz, 1);
}