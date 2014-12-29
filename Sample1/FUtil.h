
#pragma once
#pragma warning(disable:4005 4324)

#include <windows.h>

#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>

#include <DxErr.h>
#include <xnamath.h>

#include "dxapp.h"

class FPrimitive;


class FUtil
{
public:
    static void RenderPrimitive(FPrimitive* primitive, LPD3DDeviceContext context, CBMatrixSet& cbset, LPD3D11Buffer buffer);


    static HRESULT CompileShaderFromFile( char* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut );
    static HRESULT InitVertexShader( LPD3D11Device device, char* fileName, LPCSTR entryPoint, LPCTSTR shaderModel, ID3DBlob** blob, ID3D11VertexShader** shaderPtr);
    static HRESULT InitPixelShader( LPD3D11Device device, char* fileName, LPCSTR entryPoint, LPCTSTR shaderModel, ID3DBlob** blob, ID3D11PixelShader** shaderPtr);


    static void Log(char* format, ...);
};