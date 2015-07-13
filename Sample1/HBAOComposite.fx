
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


SamplerState samPointClamp : register(s0);

Texture2D txAO : register(t0);
Texture2D txColor : register(t1);


float4 PS (PS_INPUT input) : SV_Target
{
    float ao = txAO.SampleLevel(samPointClamp, input.Tex, 0).r;
    float4 color = txColor.SampleLevel(samPointClamp, input.Tex, 0).rgba;

    return color * ao;
}
