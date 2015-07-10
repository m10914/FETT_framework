// Copyright (c) 2011 NVIDIA Corporation. All rights reserved.
//
// TO  THE MAXIMUM  EXTENT PERMITTED  BY APPLICABLE  LAW, THIS SOFTWARE  IS PROVIDED
// *AS IS*  AND NVIDIA AND  ITS SUPPLIERS DISCLAIM  ALL WARRANTIES,  EITHER  EXPRESS
// OR IMPLIED, INCLUDING, BUT NOT LIMITED  TO, NONINFRINGEMENT,IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL  NVIDIA 
// OR ITS SUPPLIERS BE  LIABLE  FOR  ANY  DIRECT, SPECIAL,  INCIDENTAL,  INDIRECT,  OR  
// CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION,  DAMAGES FOR LOSS 
// OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY 
// OTHER PECUNIARY LOSS) ARISING OUT OF THE  USE OF OR INABILITY  TO USE THIS SOFTWARE, 
// EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
//
// Please direct any bugs or questions to SDKFeedback@nvidia.com
// tampered with by Damon Wall

cbuffer cbDefault : register(b0)
{
    float2 FullResolution;
    float2 InvFullResolution;
    float2 AOResolution;
    float2 InvAOResolution;

    float2 FocalLen;
    float2 InvFocalLen;
    float2 UVToViewA;
    float2 UVToViewB;

    float R;
    float R2;
    float NegInvR2;
    float MaxRadiusPixels;

    float AngleBias;
    float TanAngleBias;
    float PowExponent;
    float Strength;

    float BlurDepthThreshold;
    float BlurFalloff;
    float LinA;
    float LinB;

    float2 nearFar;
    float2 garbage;
};


sampler PointClampSampler      : register(s0);
sampler LinearSamplerClamp     : register(s1);

// Use a static blur radius so the loops can be unrolled by fxc
#define BlurRadius KERNEL_RADIUS
#define HalfBlurRadius (BlurRadius/2)

#define KERNEL_RADIUS 12

//----------------------------------------------------------------------------------
float CrossBilateralWeight(float r, float d, float d0)
{
    // The exp2(-r*r*BlurFalloff) expression below is pre-computed by fxc.
    // On GF100, this ||d-d0||<threshold test is significantly faster than an exp weight.
    return exp2(-r*r*BlurFalloff) * (abs(d - d0) < BlurDepthThreshold);
}



// Must match the COL_TILE_W value from HBAO_DX11_LIB.cpp
#define COL_TILE_W 320

Texture2D<float2> tAoLinDepth       : register(t0);
RWTexture2D<float> uOutputBuffer    : register(u0);

#define SHARED_MEM_SIZE (KERNEL_RADIUS + COL_TILE_W + KERNEL_RADIUS)
groupshared float2 SharedMem[SHARED_MEM_SIZE];

//----------------------------------------------------------------------------------
// Load FP32x2 (AO,Z) sample from shared memory.
// On Fermi, such strided 64-bit accesses should not have any bank conflicts.
//----------------------------------------------------------------------------------
float2 GetSample(uint j) 
{
    return SharedMem[j];
}

//----------------------------------------------------------------------------------
float2 LinearSampleAOZ(float2 uv)
{
    return tAoLinDepth.SampleLevel(LinearSamplerClamp, uv, 0);
}

//----------------------------------------------------------------------------------
float2 PointSampleAOZ(float2 uv)
{
    return tAoLinDepth.SampleLevel(PointClampSampler, uv, 0);
}

//----------------------------------------------------------------------------------
// Approximate vertical cross-bilateral filter
//----------------------------------------------------------------------------------
void BlurY_CS(uint3 threadIdx, uint3 blockIdx)
{
    const float              col  = (float)blockIdx.y;
    const float         tileStart = (float)blockIdx.x * COL_TILE_W;
    const float           tileEnd = tileStart + COL_TILE_W;
    const float        apronStart = tileStart - KERNEL_RADIUS;
    const float          apronEnd = tileEnd   + KERNEL_RADIUS;

    // Fetch (ao,z) between adjacent pixels with linear interpolation
    const float x = col;
    const float y = apronStart + (float)threadIdx.x + 0.5f;
    const float2 uv = (float2(x,y) + 0.5f) * InvFullResolution;
    SharedMem[threadIdx.x] = LinearSampleAOZ(uv);
    GroupMemoryBarrierWithGroupSync();

    const float writePos = tileStart + (float)threadIdx.x;
    const float tileEndClamped = min(tileEnd, FullResolution.x);
    [branch]
    if (writePos < tileEndClamped)
    {
        // Fetch (ao,z) at the kernel center
        float2 uv = ((float2(x,writePos)) + 0.5f) * InvFullResolution;
        float2 AoDepth = PointSampleAOZ(uv);
        float ao_total = AoDepth.x;
        float center_d = AoDepth.y;
        float w_total = 1;
        float i;

        [unroll]
        for (i = 0; i < HalfBlurRadius; ++i)
        {
            // Sample the pre-filtered data with step size = 2 pixels
            float r = 2.0f*i + (-BlurRadius+0.5f);
            uint j = 2*(uint)i + threadIdx.x;
            float2 sample = GetSample(j);
            float w = CrossBilateralWeight(r, sample.y, center_d);
            ao_total += w * sample.x;
            w_total += w;
        }

        [unroll]
        for (i = 0; i < HalfBlurRadius; ++i)
        {
            // Sample the pre-filtered data with step size = 2 pixels
            float r = 2.0f*i + 1.5f;
            uint j = 2*(uint)i + threadIdx.x + KERNEL_RADIUS + 1;
            float2 sample = GetSample(j);
            float w = CrossBilateralWeight(r, sample.y, center_d);
            ao_total += w * sample.x;
            w_total += w;
        }

        float ao = ao_total / w_total;
        uOutputBuffer[uint2(blockIdx.y, writePos)] = ao;
    }
}

//----------------------------------------------------------------------------------
// Stubs
//----------------------------------------------------------------------------------
[numthreads(SHARED_MEM_SIZE, 1, 1)]
void CSMain (uint3 threadIdx : SV_GroupThreadID, uint3 blockIdx : SV_GroupID)
{
    BlurY_CS(threadIdx, blockIdx);
}

