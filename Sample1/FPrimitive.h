/*
=====================================================================


=====================================================================
*/

#pragma warning(disable:4005)
#pragma once

#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>
#include <xnamath.h>

#include <xmmintrin.h>

#include "dxapp.h"


class FPrimitive
{
protected:

public:

    FPrimitive();

    XMFLOAT3 position;
    XMFLOAT3 rotationEuler;
    XMFLOAT3 scale;
   
    XMMATRIX getTransform();
    void writeTransform(CBMatrixSet& cbset);


    virtual	HRESULT Init(LPD3D11Device device) = 0;
    virtual HRESULT Render(LPD3DDeviceContext context) = 0;
    virtual HRESULT Release() = 0;
};
