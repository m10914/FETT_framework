/*
DOF

*/


#include "DOF.h"

#include "dxapp.h"
#include "FUtil.h"
#include "d3d9.h"



void DOFEffect::doBlur(ID3D11ShaderResourceView* color, float blurQ)
{
    D3DPERF_BeginEvent(D3DCOLOR_RGBA(0, 255, 0, 1), L"Blur");

    constBuffer.blurQ = XMFLOAT4(blurQ, 0, 0, 0);
    
    GFXCONTEXT->UpdateSubresource(hwBuffer, 0, NULL, &constBuffer, 0, 0);
    GFXCONTEXT->PSSetConstantBuffers(0, 1, &hwBuffer);
    GFXCONTEXT->VSSetConstantBuffers(0, 1, &hwBuffer);

    ID3D11ShaderResourceView* srviews[] =
    {
        color,
    };
    GFXCONTEXT->PSSetShaderResources(0, 1, srviews);

    GFXCONTEXT->PSSetSamplers(0, 1, &mSamplerLinear);

    GFXCONTEXT->PSSetShader(mBlurPixelShader, NULL, 0);
    GFXCONTEXT->VSSetShader(mBlurVertexShader, NULL, 0);

    GFXCONTEXT->IASetVertexBuffers(0, 0, NULL, NULL, NULL);

    GFXCONTEXT->Draw(3, 0);

    D3DPERF_EndEvent();
}


void DOFEffect::doBokeh(
    ID3D11ShaderResourceView* color, 
    ID3D11ShaderResourceView* depth,
    int rtWidth, int rtHeight
    )
{
    D3DPERF_BeginEvent(D3DCOLOR_RGBA(0, 255, 0, 1), L"Bokeh");

    constBuffer.rtWidth = rtWidth;
    constBuffer.rtHeight = rtHeight;
    constBuffer.oneOverRT = XMFLOAT2(1.0 / rtWidth, 1.0 / rtHeight);

    GFXCONTEXT->UpdateSubresource(hwBuffer, 0, NULL, &constBuffer, 0, 0);
    GFXCONTEXT->PSSetConstantBuffers(0, 1, &hwBuffer);
    GFXCONTEXT->VSSetConstantBuffers(0, 1, &hwBuffer);

    ID3D11ShaderResourceView* srviews[] =
    {
        color,
        depth,
        mBokehShapeTexSRV
    };
    GFXCONTEXT->VSSetShaderResources(0, 3, srviews);
    GFXCONTEXT->PSSetShaderResources(0, 3, srviews);

    GFXCONTEXT->PSSetShader(mPixelShader, NULL, 0);
    GFXCONTEXT->VSSetShader(mVertexShader, NULL, 0);

    GFXCONTEXT->IASetVertexBuffers(0, 0, NULL, NULL, NULL);

    GFXCONTEXT->RSSetState(mRSCullNone);

    GFXCONTEXT->OMSetBlendState(mBlendState, NULL, 0xffffffff);

    int totalCount = rtWidth*rtHeight * 6;
    GFXCONTEXT->Draw(totalCount, 0);




    srviews[0] = NULL;
    srviews[1] = NULL;
    GFXCONTEXT->VSSetShaderResources(0, 2, srviews);

    GFXCONTEXT->OMSetBlendState(NULL, NULL, 0xffffffff);

    D3DPERF_EndEvent();
}


void DOFEffect::doMerge(ID3D11ShaderResourceView* blurredColor, ID3D11ShaderResourceView* bokehColor)
{
    D3DPERF_BeginEvent(D3DCOLOR_RGBA(0, 255, 0, 1), L"Merge");

    ID3D11ShaderResourceView* srviews[] =
    {
        blurredColor,
        bokehColor
    };
    GFXCONTEXT->PSSetShaderResources(0, 2, srviews);

    GFXCONTEXT->PSSetSamplers(0, 1, &mSamplerLinear);

    GFXCONTEXT->PSSetShader(mFinPixelShader, NULL, 0);
    GFXCONTEXT->VSSetShader(mFinVertexShader, NULL, 0);

    GFXCONTEXT->Draw(3, 0);

    D3DPERF_EndEvent();
}




DOFEffect::DOFEffect()
{
    HRESULT hr;

    // init shaders
    FUtil::InitPixelShader("DOF.hlsl", "ps_main", "ps_5_0", &mPixelShader);
    FUtil::InitVertexShader("DOF.hlsl", "vs_main", "vs_5_0", &mVertexShader);

    FUtil::InitPixelShader("DOF.hlsl", "ps_finalize", "ps_5_0", &mFinPixelShader);
    FUtil::InitVertexShader("DOF.hlsl", "vs_finalize", "vs_5_0", &mFinVertexShader);

    FUtil::InitPixelShader("DOF.hlsl", "ps_blur", "ps_5_0", &mBlurPixelShader);
    FUtil::InitVertexShader("DOF.hlsl", "vs_blur", "vs_5_0", &mBlurVertexShader);


    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.ByteWidth = sizeof(CBforDOF);
    hr = GFXDEVICE->CreateBuffer(&bd, NULL, &hwBuffer);
    if (FAILED(hr))
    {
        MessageBox(0, "Failed to create buffer", "Error.", 0);
    }


    D3D11_RASTERIZER_DESC desc;
    ZeroMemory(&desc, sizeof(D3D11_RASTERIZER_DESC));
    desc.CullMode = D3D11_CULL_NONE;
    desc.FillMode = D3D11_FILL_SOLID;
    hr = GFXDEVICE->CreateRasterizerState(&desc, &mRSCullNone);
    if (FAILED(hr))
    {
        MessageBox(0, "Error: cannot create rasterizer state", "Error.", 0);
    }


    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = GFXDEVICE->CreateSamplerState(&sampDesc, &mSamplerLinear);
    if (FAILED(hr))
    {
        MessageBox(0, "Error: cannot create sampler", "Error.", 0);
    }


    D3D11_BLEND_DESC bdesc;
    ZeroMemory(&bdesc, sizeof(bdesc));
    bdesc.IndependentBlendEnable = FALSE;
    bdesc.RenderTarget[0].BlendEnable = true;
    bdesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bdesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bdesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    bdesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    bdesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bdesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    bdesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    hr = GFXDEVICE->CreateBlendState(&bdesc, &mBlendState);
    if (FAILED(hr))
    {
        MessageBox(0, "Error: cannot create blend state", "Error.", 0);
    }


    // Load the Texture
    hr = D3DX11CreateShaderResourceViewFromFile(GFXDEVICE, "bokehShape.dds", NULL, NULL, &mBokehShapeTexSRV, NULL);
    if (FAILED(hr))
    {
        MessageBox(0, "Error: cannot create bokeh shape texture", "Error.", 0);
    }
}


DOFEffect::~DOFEffect()
{
    SAFE_RELEASE(mPixelShader);
    SAFE_RELEASE(mVertexShader);

    SAFE_RELEASE(mFinPixelShader);
    SAFE_RELEASE(mFinVertexShader);

    SAFE_RELEASE(hwBuffer);
    SAFE_RELEASE(mRSCullNone);
}