

#define NUM_DIRECTIONS 8
#define NUM_STEPS 6
#define M_PI 3.14159265f


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


sampler PointClampSampler : register(s0);
sampler PointWrapSampler : register(s1);

Texture2D txDepth : register(t0);
Texture2D tRandom : register(t1);

//----------------------------------------------------------------------------------
float linearizeDepth(float depth)
{
    return (2 * nearFar.x) / (nearFar.y + nearFar.x - depth * (nearFar.y - nearFar.x));
}
//----------------------------------------------------------------------------------
float InvLength(float2 v)
{
    return rsqrt(dot(v, v));
}
//----------------------------------------------------------------------------------
float Tangent(float3 P, float3 S)
{
    return (P.z - S.z) * InvLength(S.xy - P.xy);
}

//----------------------------------------------------------------------------------
float3 UVToEye(float2 uv, float eye_z)
{
    uv = UVToViewA * uv + UVToViewB;
    return float3(uv * eye_z, eye_z);
}
//----------------------------------------------------------------------------------
float3 FetchEyePos(float2 uv)
{
    float z = linearizeDepth(txDepth.SampleLevel(PointClampSampler, uv, 0));
    return UVToEye(uv, z);
}
//----------------------------------------------------------------------------------
float Length2(float3 v)
{
    return dot(v, v);
}
//----------------------------------------------------------------------------------
float3 MinDiff(float3 P, float3 Pr, float3 Pl)
{
    float3 V1 = Pr - P;
        float3 V2 = P - Pl;
        return (Length2(V1) < Length2(V2)) ? V1 : V2;
}
//----------------------------------------------------------------------------------
float Falloff(float d2)
{
    // 1 scalar mad instruction
    return d2 * NegInvR2 + 1.0f;
}

//----------------------------------------------------------------------------------
float2 SnapUVOffset(float2 uv)
{
    return round(uv * AOResolution) * InvAOResolution;
}
//----------------------------------------------------------------------------------
float TanToSin(float x)
{
    return x * rsqrt(x*x + 1.0f);
}

//----------------------------------------------------------------------------------
float3 TangentVector(float2 deltaUV, float3 dPdu, float3 dPdv)
{
    return deltaUV.x * dPdu + deltaUV.y * dPdv;
}
//----------------------------------------------------------------------------------
float Tangent(float3 T)
{
    return -T.z * InvLength(T.xy);
}
//----------------------------------------------------------------------------------
float BiasedTangent(float3 T)
{
    // Do not use atan() because it gets expanded by fxc to many math instructions
    return Tangent(T) + TanAngleBias;
}

//----------------------------------------------------------------------------------
void ComputeSteps(inout float2 step_size_uv, inout float numSteps, float ray_radius_pix, float rand)
{
    // Avoid oversampling if NUM_STEPS is greater than the kernel radius in pixels
    numSteps = min(NUM_STEPS, ray_radius_pix);

    // Divide by Ns+1 so that the farthest samples are not fully attenuated
    float step_size_pix = ray_radius_pix / (numSteps + 1);

    // Clamp numSteps if it is greater than the max kernel footprint
    float maxNumSteps = MaxRadiusPixels / step_size_pix;
    if (maxNumSteps < numSteps)
    {
        // Use dithering to avoid AO discontinuities
        numSteps = floor(maxNumSteps + rand);
        numSteps = max(numSteps, 1);
        step_size_pix = MaxRadiusPixels / numSteps;
    }

    // Step size in uv space
    step_size_uv = step_size_pix * InvAOResolution;
}
//----------------------------------------------------------------------------------
float2 RotateDirections(float2 Dir, float2 CosSin)
{
    return float2(Dir.x*CosSin.x - Dir.y*CosSin.y,
        Dir.x*CosSin.y + Dir.y*CosSin.x);
}
//----------------------------------------------------------------------------------



/*
float IntegerateOcclusion(
    float2 uv0,
    float2 snapped_duv,
    float3 P,
    float3 dPdu,
    float3 dPdv,
    inout float tanH)
{
    float ao = 0;

    // Compute a tangent vector for snapped_duv
    float3 T1 = TangentVector(snapped_duv, dPdu, dPdv);
        float tanT = BiasedTangent(T1);
    float sinT = TanToSin(tanT);

    float3 S = FetchEyePos(uv0 + snapped_duv);
        float tanS = Tangent(P, S);

    float sinS = TanToSin(tanS);
    float d2 = Length2(S - P);

    if ((d2 < R2) && (tanS > tanT))
    {
        // Compute AO between the tangent plane and the sample
        ao = Falloff(d2) * (sinS - sinT);

        // Update the horizon angle
        tanH = max(tanH, tanS);
    }

    return ao;
}

//----------------------------------------------------------------------------------
float horizon_occlusion(float2 deltaUV,
    float2 texelDeltaUV,
    float2 uv0,
    float3 P,
    float numSteps,
    float randstep,
    float3 dPdu,
    float3 dPdv)
{
    float ao = 0;

    // Randomize starting point within the first sample distance
    float2 uv = uv0 + SnapUVOffset(randstep * deltaUV);

    // Snap increments to pixels to avoid disparities between xy
    // and z sample locations and sample along a line
    deltaUV = SnapUVOffset(deltaUV);

    // Compute tangent vector using the tangent plane
    float3 T = deltaUV.x * dPdu + deltaUV.y * dPdv;

    float tanH = BiasedTangent(T);

    // Take a first sample between uv0 and uv0 + deltaUV
    float2 snapped_duv = SnapUVOffset(randstep * deltaUV + texelDeltaUV);
        ao = IntegerateOcclusion(uv0, snapped_duv, P, dPdu, dPdv, tanH);
    --numSteps;

    float sinH = tanH / sqrt(1.0f + tanH*tanH);

    for (float j = 1; j <= numSteps; ++j)
    {
        uv += deltaUV;
        float3 S = FetchEyePos(uv);
        float tanS = Tangent(P, S);
        float d2 = Length2(S - P);

        // Use a merged dynamic branch
        [branch]
        if ((d2 < R2) && (tanS > tanH))
        {
            // Accumulate AO between the horizon and the sample
            float sinS = tanS / sqrt(1.0f + tanS*tanS);
            ao += Falloff(d2) * (sinS - sinH);

            // Update the current horizon angle
            tanH = tanS;
            sinH = sinS;
        }
    }

    return ao;
}
*/

//----------------------------------------------------------------------------------
void IntegrateDirection(inout float ao, float3 P, float2 uv, float2 deltaUV,
    float numSteps, float tanH, float sinH)
{
    for (float j = 1; j <= numSteps; ++j)
    {
        uv += deltaUV;
        float3 S = FetchEyePos(uv);
        float tanS = Tangent(P, S);
        float d2 = Length2(S - P);

        [branch]
        if ((d2 < R2) && (tanS > tanH))
        {
            // Accumulate AO between the horizon and the sample
            float sinS = TanToSin(tanS);
            ao += Falloff(d2) * (sinS - sinH);

            // Update the current horizon angle
            tanH = tanS;
            sinH = sinS;
        }
    }
}

//----------------------------------------------------------------------------------
float NormalFreeHorizonOcclusion(float2 deltaUV,
    float2 texelDeltaUV,
    float2 uv0,
    float3 P,
    float numSteps,
    float randstep)
{
    float tanT = tan(-M_PI*0.5 + AngleBias);
    float sinT = TanToSin(tanT);

    float ao1 = 0;

    // Take a first sample between uv0 and uv0 + deltaUV
    float2 deltaUV1 = SnapUVOffset(randstep * deltaUV + texelDeltaUV);
    IntegrateDirection(ao1, P, uv0, deltaUV1, 1, tanT, sinT);
    IntegrateDirection(ao1, P, uv0, -deltaUV1, 1, tanT, sinT);

    ao1 = max(ao1 * 0.5 - 1.0, 0.0);
    --numSteps;

    float ao = 0;
    float2 uv = uv0 + SnapUVOffset(randstep * deltaUV);
    deltaUV = SnapUVOffset(deltaUV);
    IntegrateDirection(ao, P, uv, deltaUV, numSteps, tanT, sinT);

    // Integrate opposite directions together
    deltaUV = -deltaUV;
    uv = uv0 + SnapUVOffset(randstep * deltaUV);
    IntegrateDirection(ao, P, uv, deltaUV, numSteps, tanT, sinT);

    // Divide by 2 because we have integrated 2 directions together
    // Subtract 1 and clamp to remove the part below the surface
    return max(ao * 0.5 - 1.0, ao1);

}



/*
===========================================================================================
Entry point for HBAO effect.


===========================================================================================
*/
float4 HBAO_PS(PS_INPUT IN) : SV_Target
{
    float3 P = FetchEyePos(IN.Tex);

    // (cos(alpha),sin(alpha),jitter)
    float3 rand = tRandom.Sample(PointWrapSampler, IN.Pos.xy / 4);

    // Compute projection of disk of radius R into uv space
    // Multiply by 0.5 to scale from [-1,1]^2 to [0,1]^2
    float2 ray_radius_uv = 0.5 * R * FocalLen / P.z;
    float ray_radius_pix = ray_radius_uv.x * AOResolution.x;
    if (ray_radius_pix < 1) 
        return float4(1,1,1,1);

    float numSteps;
    float2 step_size;
    ComputeSteps(step_size, numSteps, ray_radius_pix, rand.z);

    // Nearest neighbor pixels on the tangent plane
    float3 Pr, Pl, Pt, Pb;
    Pr = FetchEyePos(IN.Tex + float2(InvAOResolution.x, 0));
    Pl = FetchEyePos(IN.Tex + float2(-InvAOResolution.x, 0));
    Pt = FetchEyePos(IN.Tex + float2(0, InvAOResolution.y));
    Pb = FetchEyePos(IN.Tex + float2(0, -InvAOResolution.y));

    // Screen-aligned basis for the tangent plane
    float3 dPdu = MinDiff(P, Pr, Pl);
    float3 dPdv = MinDiff(P, Pt, Pb) * (AOResolution.y * InvAOResolution.x);

    float ao = 0;
    float d;
    float alpha = 2.0f * M_PI / NUM_DIRECTIONS;

    /*for (d = 0; d < NUM_DIRECTIONS; ++d)
    {
        float angle = alpha * d;
        float2 dir = RotateDirections(float2(cos(angle), sin(angle)), rand.xy);
        float2 deltaUV = dir * step_size.xy;
        float2 texelDeltaUV = dir * InvAOResolution;
        ao += horizon_occlusion(deltaUV, texelDeltaUV, IN.Tex, P, numSteps, rand.z, dPdu, dPdv);
    }*/
    for (d = 0; d < NUM_DIRECTIONS*0.5; ++d)
    {
        float angle = alpha * d;
        float2 dir = RotateDirections(float2(cos(angle), sin(angle)), rand.xy);
        float2 deltaUV = dir * step_size.xy;
        float2 texelDeltaUV = dir * InvAOResolution;
        ao += NormalFreeHorizonOcclusion(deltaUV, texelDeltaUV, IN.Tex, P, numSteps, rand.z);
    }
    ao *= 2.0;

    ao = 1.0 - ao / NUM_DIRECTIONS * Strength;
    return float4(ao, P.z, 1, 1);
}
