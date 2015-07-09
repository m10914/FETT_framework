

#include "PostEffect.h"

#include "dxapp.h"
#include "FUtil.h"
#include <d3d9.h>


ID3D11VertexShader* PostEffect::mVertexShaderQuad = NULL;
ID3D11InputLayout* PostEffect::mLayoutPT = NULL;


ID3D11VertexShader* PostEffect::getQuadVertexShader()
{
    ID3DBlob* pVSBlob = NULL;

    if (!mVertexShaderQuad)
        FUtil::InitVertexShader(GFXDEVICE, "FettEssential.fx", "VS_QUAD", "vs_5_0", &pVSBlob, &mVertexShaderQuad);

    return mVertexShaderQuad;
}



PostEffect::PostEffect(
    LPCSTR pixShader, LPCSTR entryPoint,
    LPCSTR vertShader, LPCSTR vertEntryPoint)
{
    strcpy(pixShaderName, pixShader);
    strcpy(this->entryPoint, entryPoint);

    ID3DBlob* pBlob = NULL;
    FUtil::InitPixelShader(GFXDEVICE, pixShaderName, entryPoint, "ps_5_0", &pBlob, &mPixelShader);

    if (vertShader != NULL)
    {
        FUtil::InitVertexShader(GFXDEVICE, (char*)vertShader, vertEntryPoint, "vs_5_0", &pBlob, &mVertexShader);
    }
    else
        mVertexShader = getQuadVertexShader();

    mLayoutPT = VertexFormatMgr::getPTLayout();
};

PostEffect::~PostEffect()
{
    SAFE_RELEASE(mPixelShader);
}


void PostEffect::render()
{
    D3DPERF_BeginEvent(D3DCOLOR_RGBA(0, 255, 0, 1), L"PostEffect");

    updateConstants();

    GFXCONTEXT->IASetInputLayout(mLayoutPT);

    GFXCONTEXT->PSSetShader(mPixelShader, NULL, 0);
    GFXCONTEXT->VSSetShader(mVertexShader, NULL, 0);

    renderQuad();

    D3DPERF_EndEvent();
}



void PostEffect::renderQuad(XMFLOAT2 offset, XMFLOAT2 relSize)
{
    //assemble points
    ID3D11Buffer* mVertexBuffer = NULL;

    //create buffers
    HRESULT hr;

    VertexFormatPT vertices[] =
    {
        { XMFLOAT3(2.0*offset.x - 1.0, 2.0*(offset.y + relSize.y) - 1.0, 0), XMFLOAT2(0, 0) },
        { XMFLOAT3(2.0*(offset.x + relSize.x) - 1.0, 2.0*(offset.y + relSize.y) - 1.0, 0), XMFLOAT2(1, 0) },
        { XMFLOAT3(2.0*offset.x - 1.0, 2.0*offset.y - 1.0, 0), XMFLOAT2(0, 1) },
        { XMFLOAT3(2.0*(offset.x + relSize.x) - 1.0, 2.0*offset.y - 1.0, 0), XMFLOAT2(1, 1) },
    };

    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));

    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(vertices);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    InitData.pSysMem = vertices;
    hr = GFXDEVICE->CreateBuffer(&bd, &InitData, &mVertexBuffer);
    if (FAILED(hr))
        goto render_quad_end;

    // Set vertex buffer
    UINT stride = sizeof(VertexFormatPT);
    UINT loffset = 0;
    GFXCONTEXT->IASetVertexBuffers(0, 1, &mVertexBuffer, &stride, &loffset);

    // Set primitive topology
    GFXCONTEXT->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // draw dat shit
    GFXCONTEXT->Draw(4, 0);


render_quad_end:
    SAFE_RELEASE(mVertexBuffer);
    return;
}