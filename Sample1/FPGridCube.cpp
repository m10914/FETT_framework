

#include "FPGridCube.h"



FPGridCube::FPGridCube(int width) : mGridWidth(width)
{

}


FPGridCube::~FPGridCube()
{

}




HRESULT FPGridCube::Init(LPD3D11Device device)
{
    HRESULT hr;

    // Create vertex buffer
    numOfVertices = mGridWidth*mGridWidth*6;
    VertexFormatPT* vertices = new VertexFormatPT[numOfVertices];

    //first
    int offset = mGridWidth*mGridWidth;
    for (int i = 0; i < mGridWidth; i++)
        for (int j = 0; j < mGridWidth; j++)
        {
            float fracx = (float)i / (mGridWidth-1) * 2, fracy = (float)j / (mGridWidth-1) * 2;
            vertices[j*mGridWidth + i] = { XMFLOAT3(fracx - 1.0f, 1.0f, fracy - 1.0f), XMFLOAT2(fracx, fracy) };
        }
    for (int i = 0; i < mGridWidth; i++)
        for (int j = 0; j < mGridWidth; j++)
        {
            float fracx = (float)i / (mGridWidth - 1) * 2, fracy = (float)j / (mGridWidth - 1) * 2;
            vertices[j*mGridWidth + i + offset] = { XMFLOAT3(1.0f - fracx, -1.0f, fracy - 1.0f), XMFLOAT2(fracx, fracy) };
        }
    for (int i = 0; i < mGridWidth; i++)
        for (int j = 0; j < mGridWidth; j++)
        {
            float fracx = (float)i / (mGridWidth - 1) * 2, fracy = (float)j / (mGridWidth - 1) * 2;
            vertices[j*mGridWidth + i + 2*offset] = { XMFLOAT3(fracx - 1.0f, 1.0f - fracy, 1.0), XMFLOAT2(fracx, fracy) };
        }
    for (int i = 0; i < mGridWidth; i++)
        for (int j = 0; j < mGridWidth; j++)
        {
            float fracx = (float)i / (mGridWidth - 1) * 2, fracy = (float)j / (mGridWidth - 1) * 2;
            vertices[j*mGridWidth + i + 3*offset] = { XMFLOAT3(fracx - 1.0f, fracy - 1.0f, -1.0), XMFLOAT2(fracx, fracy) };
        }
    for (int i = 0; i < mGridWidth; i++)
        for (int j = 0; j < mGridWidth; j++)
        {
            float fracx = (float)i / (mGridWidth - 1) * 2, fracy = (float)j / (mGridWidth - 1) * 2;
            vertices[j*mGridWidth + i + 4*offset] = { XMFLOAT3(1.0f, fracx - 1.0f, 1.0f - fracy), XMFLOAT2(fracx, fracy) };
        }
    for (int i = 0; i < mGridWidth; i++)
        for (int j = 0; j < mGridWidth; j++)
        {
            float fracx = (float)i / (mGridWidth - 1) * 2, fracy = (float)j / (mGridWidth - 1) * 2;
            vertices[j*mGridWidth + i + 5*offset] = { XMFLOAT3(-1.0f, fracx - 1.0f, fracy - 1.0f), XMFLOAT2(fracx, fracy) };
        }

    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(VertexFormatPT) * numOfVertices;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = vertices;
    hr = device->CreateBuffer(&bd, &InitData, &mVertexBuffer);
    V_RETURN(hr);


    // create index buffer
    // trickier
    numOfFaces = (mGridWidth - 1)*(mGridWidth - 1) * 2 * 6;
    numOfIndices = numOfFaces * 3;

    UINT* indices = new UINT[numOfIndices];
    int indexIndex = 0;

#define VERT(X,Y) (X) + mGridWidth*(Y) + gi*offset
    for (int gi = 0; gi < 6; gi++)
    {
        for (int j = 0; j < mGridWidth - 1; j++)
            for (int i = 0; i < mGridWidth - 1; i++)
            {
                //create two faces
                indices[indexIndex++] = VERT(i + 1, j);
                indices[indexIndex++] = VERT(i, j);
                indices[indexIndex++] = VERT(i, j + 1);

                indices[indexIndex++] = VERT(i + 1, j);
                indices[indexIndex++] = VERT(i, j + 1);
                indices[indexIndex++] = VERT(i + 1, j + 1);
            }
    }
#undef VERT

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(UINT) * numOfIndices;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;
    InitData.pSysMem = indices;
    hr = device->CreateBuffer(&bd, &InitData, &mIndexBuffer);
    V_RETURN(hr);

    delete[] indices;
    delete[] vertices;

    return hr;
}


HRESULT FPGridCube::Render(LPD3DDeviceContext context)
{
    // Set vertex buffer
    UINT stride = sizeof(VertexFormatPT);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &mVertexBuffer, &stride, &offset);

    // Set index buffer
    context->IASetIndexBuffer(mIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

    // Set primitive topology
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // draw dat shit
    context->DrawIndexed(numOfIndices, 0, 0);

    return S_OK;
}


HRESULT FPGridCube::Release()
{
    SAFE_RELEASE(mVertexBuffer);
    SAFE_RELEASE(mIndexBuffer);

    return S_OK;
}