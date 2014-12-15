

#pragma warning(disable:4005)
#pragma once

#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>
#include <xnamath.h>

#include <xmmintrin.h>

#include "dxapp.h"
#include "FPrimitive.h"





class FSurface : FPrimitive
{
public:
    FSurface(int sizeX, int sizeY);
    ~FSurface();


    // FPrimitive implementation
    virtual	HRESULT Init(LPD3D11Device device) override;
    virtual HRESULT Render(LPD3DDeviceContext context) override;
    virtual HRESULT Release() override;

protected:

    // gridsize
    int SizeX;
    int SizeY;

};