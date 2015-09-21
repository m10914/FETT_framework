
/*
==============================================================
Quad shader


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


SamplerState samLinear : register(s0);

Texture2D txDiffuse : register(t0);


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


//-----------------------------------------------
// finalize