

#include "VertexFormats.h"
#include "FUtil.h"


//statics
ID3D11InputLayout* VertexFormatMgr::mPTLayout(NULL);
ID3D11InputLayout* VertexFormatMgr::mPNTLayout(NULL);




//static
ID3D11InputLayout* VertexFormatMgr::getPTLayout()
{
    HRESULT hr;

    if(mPTLayout == NULL)
    {
        ID3DBlob* pVSBlob = getShaderBlob( "VS_PT" );

        // Define the input layout
        D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        UINT numElements = ARRAYSIZE( layout );

        // Create the input layout
        hr = GFXDEVICE->CreateInputLayout( layout, numElements, pVSBlob->GetBufferPointer(),
            pVSBlob->GetBufferSize(), &mPTLayout );

        pVSBlob->Release();
    }


    return mPTLayout;
}


//static
ID3D11InputLayout* VertexFormatMgr::getPNTLayout()
{
    HRESULT hr;

    if(mPNTLayout == NULL)
    {
        ID3DBlob* pVSBlob = getShaderBlob( "VS_PNT" );

        // Define the input layout
        D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },          
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        UINT numElements = ARRAYSIZE( layout );

        // Create the input layout
        hr = GFXDEVICE->CreateInputLayout( layout, numElements, pVSBlob->GetBufferPointer(),
            pVSBlob->GetBufferSize(), &mPNTLayout );

        pVSBlob->Release();
    }


    return mPNTLayout;
}


/*
do not forget to release blob after using it
*/
ID3DBlob* VertexFormatMgr::getShaderBlob(char* str)
{
    ID3DBlob* pVSBlob = NULL;
    HRESULT hr = FUtil::CompileShaderFromFile( "VertexFormatsInitializer.fx", str, "vs_4_0", &pVSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( NULL, "Error in VertexFormatsInitializer.fx, contact developer.", "Error", MB_OK );
        return NULL;
    }

    return pVSBlob;
}


//static
void VertexFormatMgr::release()
{
    SAFE_RELEASE(mPTLayout);
    SAFE_RELEASE(mPNTLayout);
}