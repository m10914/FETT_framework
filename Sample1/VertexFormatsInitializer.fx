

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