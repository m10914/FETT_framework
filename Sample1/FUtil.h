
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

};