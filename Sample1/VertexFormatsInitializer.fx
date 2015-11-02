


//-----------------------------------
// PT

struct VS_INPUT_PT
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
};

float4 VS_PT( VS_INPUT_PT input ) : SV_POSITION
{
    return input.Pos;
}




//----------------------------------
// PNT

struct VS_INPUT_PNT
{
    float4 Pos : POSITION;
    float4 Normal: NORMAL;
    float2 Tex : TEXCOORD0;
};

float4 VS_PNT( VS_INPUT_PNT input ) : SV_POSITION
{
    return input.Pos;
}



//----------------------------------
// PPTT

struct VS_INPUT_PPTT
{
    float3 Pos: POSITION;
    float3 Pos2 : TEXCOORD0;
    float3 upvec : TEXCOORD1;
    float3 upvec2 : TEXCOORD2;
    float2 uv : TEXCOORD3;
    float2 uv2 : TEXCOORD4;
};

float4 VS_PPTT(VS_INPUT_PPTT input) : SV_POSITION
{
    return float4(input.Pos, 1);
}
