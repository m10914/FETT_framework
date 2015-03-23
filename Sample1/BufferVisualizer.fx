

#define WIDTH 1024

StructuredBuffer<float2> inBuffer : register(t0);


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



PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;

	output.Pos = float4(input.Pos.xy, 0, 1);
	output.Tex = input.Tex;

	return output;
}

float4 PS(PS_INPUT input) : SV_Target
{
	int index = int(input.Tex.x * WIDTH);
	float2 val = inBuffer[index];

	return float4(val.x, 0, 0, 1);
}
