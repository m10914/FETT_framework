


#include "FUtil.h"
#include "FPrimitive.h"
#include "cstdio"

#pragma comment(lib, "legacy_stdio_definitions.lib")

void FUtil::Log(char* format, ...)
{
    va_list argList;
    char msg[2048];

    va_start(argList, format);
    vsnprintf(msg, 2048, format, argList);
    va_end(argList);

    OutputDebugString(msg);
}


void FUtil::RenderPrimitive(FPrimitive* primitive, LPD3DDeviceContext context, CBMatrixSet& cbset, LPD3D11Buffer buffer)
{
    primitive->writeTransform( cbset );
    context->UpdateSubresource( buffer, 0, NULL, &cbset, 0, 0 );
    primitive->Render( context );
}


XMVECTOR FUtil::FromFloat(XMFLOAT3& vec)
{
    return XMVectorSet(vec.x, vec.y, vec.z, 1);
}
XMVECTOR FUtil::FromFloat(XMFLOAT4& vec)
{
    return XMVectorSet(vec.x, vec.y, vec.z, vec.w);
}
XMFLOAT3 FUtil::FromVector3(XMVECTOR& vec)
{
    return XMFLOAT3(vec.m128_f32[0], vec.m128_f32[1], vec.m128_f32[2]);
}
XMFLOAT4 FUtil::FromVector4(XMVECTOR& vec)
{
    return XMFLOAT4(vec.m128_f32[0], vec.m128_f32[1], vec.m128_f32[2], vec.m128_f32[3]);
}




//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DX11
//--------------------------------------------------------------------------------------
HRESULT FUtil::CompileShaderFromFile( char* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut )
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;

#if defined( DEBUG ) || defined( _DEBUG )
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob;
    hr = D3DX11CompileFromFile( szFileName, NULL, NULL, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL );
    if( FAILED(hr) )
    {
        if( pErrorBlob != NULL )
            OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );
        if( pErrorBlob ) pErrorBlob->Release();
        return hr;
    }
    if( pErrorBlob ) pErrorBlob->Release();

    return S_OK;
}


HRESULT FUtil::InitPixelShader(LPD3D11Device device, char* fileName, LPCSTR entryPoint, LPCTSTR shaderModel, ID3DBlob** blob, ID3D11PixelShader** shaderPtr)
{
    HRESULT hr;
    *blob = NULL;
    hr = FUtil::CompileShaderFromFile( fileName, entryPoint, shaderModel, blob);
    if( FAILED( hr ) )
    {
        MessageBox( NULL,
            "The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", "Error", MB_OK );
        return hr;
    }

    // Create the pixel shader
    hr = device->CreatePixelShader( (*blob)->GetBufferPointer(), (*blob)->GetBufferSize(), NULL, shaderPtr );
    (*blob)->Release();
    if( FAILED( hr ) )
        return hr;
}

HRESULT FUtil::InitVertexShader(LPD3D11Device device, char* fileName, LPCSTR entryPoint, LPCTSTR shaderModel, ID3DBlob** blob, ID3D11VertexShader** shaderPtr)
{
    HRESULT hr;
    hr = FUtil::CompileShaderFromFile( fileName, entryPoint, shaderModel, blob );
    if( FAILED( hr ) )
    {
        MessageBox( NULL,
            "The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", "Error", MB_OK );
        return hr;
    }

    // Create the vertex shader
    hr = device->CreateVertexShader( (*blob)->GetBufferPointer(), (*blob)->GetBufferSize(), NULL, shaderPtr );
    if( FAILED( hr ) )
    {    
        (*blob)->Release();
        return hr;
    }
}