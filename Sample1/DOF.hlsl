










struct VertToPix
{
    float4 pos : SV_Position;
    float4 col : COLOR;
    float2 uv : TEXCOORD0;
};


cbuffer cbDefault : register(b0)
{
    uint rtWidth;
    uint rtHeight;
    float2 oneOverRT;
    float4 blurQ;
};


Texture2D colorTex: register(t0);
Texture2D depthTex: register(t1);

SamplerState samplerDefault : register(s0);

#define LUM_WEIGHTS float3(0.2126, 0.7152, 0.0722)


float GatherLumKernel(uint x, uint y)
{
    float res = 0;

    [unroll]
    for (uint i = 0; i < 4; i++)
        for (uint j = 0; j < 4; j++)
        {
            uint2 ind = uint2(
                x - 2 + i + float(i == 2) * 1,
                y - 2 + j + float(j == 2) * 1
                );
            res += dot( colorTex.Load(int3(ind.xy, 0)).rgb, LUM_WEIGHTS );
        }

    return res / 16.0;
}

VertToPix vs_main(uint id : SV_VertexID)
{
    VertToPix OUT;

    //get row and col
    uint x = id / 6;
    uint y = x / rtWidth;
    x = x % rtWidth;

    //get color and luminocity
    // (in real effect - we need CoC and luminocity)
    float lumKernel = GatherLumKernel(x, y);
    OUT.col = colorTex.Load(int3(x, y, 0));
    float locLum = dot(OUT.col.rgb, LUM_WEIGHTS);
    float lum = locLum - lumKernel;
    float size = lerp(0.5, 1.0, saturate((lum - 0.6) / 0.4));


    // calculate coordinates on quad
    uint ind = id % 6;
    uint smtri = ind / 3; //1 or 2
    ind = ind % 3;

    float2 tcoord = float2((ind >> 1) & 1, ind & 1);
    tcoord += (ind ^ 1 && (ind >> 1) ^ 1) * smtri; //1 only for ind == 0
    OUT.uv = tcoord;

    //-----------------------------------------------
    // blur

    float2 depth = depthTex.Load(int3(x, y, 0));
    float2 blur = float2(8, 8) * size * blurQ.x;

    tcoord = mad(tcoord, blur, 0.5 - blur*0.5);


    //------------------------------------
    // output

    // offset on a screen
    float2 res = tcoord * oneOverRT + float2(x, y)*oneOverRT;
    OUT.pos = float4(mad(res, 2, -1), 0, 1);
    
    OUT.col.a = 1;

    // discard
    if (lum < 0.4 && locLum < 0.6)
        OUT.pos += 1000;

    return OUT;
}


Texture2D bokehShape : register(t2);

float4 ps_main(VertToPix IN) : SV_TARGET
{
    float bokehTex = dot(bokehShape.Sample(samplerDefault, IN.uv).rgb, LUM_WEIGHTS);

    return float4(IN.col.rgb*bokehTex, 1.0);
}


//----------------------------------------------------
// FINALIZE

struct FinVertToPix
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
};

Texture2D blurredColor : register(t0);
Texture2D bokehShapes : register(t1);


FinVertToPix vs_finalize(uint id : SV_VertexID)
{
   FinVertToPix OUT;
          
   float2 uv;
   uv = float2((id << 1) & 2, id & 2);
   OUT.pos = float4(mad(uv, float2(2, -2), float2(-1, 1)), 0, 1);

   OUT.uv = uv;

   return OUT;
}


float4 ps_finalize(FinVertToPix IN) : SV_Target
{
    float4 bokeh = bokehShapes.Sample(samplerDefault, float2(IN.uv.x, 1 - IN.uv.y));
    if (bokeh.a != 0) bokeh.rgb /= bokeh.a;
    float4 scene = blurredColor.Sample(samplerDefault, IN.uv);

    return float4(scene.rgb + bokeh.rgb, 1);
}



//----------------------------------------------------
// Fullscreen BLUR

static float2 poissonDisk[16] = {
    float2(0.0, 0.0),
    float2(0.170019, -0.040254),
    float2(-0.299417, 0.791925),
    float2(0.645680, 0.493210),
    float2(-0.651784, 0.717887),
    float2(0.421003, 0.027070),
    float2(-0.817194, -0.271096),
    float2(-0.705374, -0.668203),
    float2(0.977050, -0.108615),
    float2(0.063326, 0.142369),
    float2(0.203528, 0.214331),
    float2(-0.667531, 0.326090),
    float2(-0.098422, -0.295755),
    float2(-0.885922, 0.215369),
    float2(0.566637, 0.605213),
    float2(0.039766, -0.396100)
};

FinVertToPix vs_blur(uint id : SV_VertexID)
{
    FinVertToPix OUT;

    float2 uv;
    uv = float2((id << 1) & 2, id & 2);
    OUT.pos = float4(mad(uv, float2(2, -2), float2(-1, 1)), 0, 1);

    OUT.uv = uv;

    return OUT;
}

Texture2D colTex : register(t0);

float4 ps_blur(FinVertToPix IN) : SV_Target
{
    float4 res = float4(0, 0, 0, 0);
    
    [unroll]
    for (int i = 0; i < 16; i++)
    {
        res += colTex.Sample(samplerDefault, IN.uv + poissonDisk[i]*0.01*blurQ.x);
    }

    return float4(res.rgb / 16, 1);
}