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
	virtual	HRESULT Init(LPD3D11Device device) = 0;
	virtual HRESULT Render(LPD3D11Device device, LPD3DDeviceContext context) = 0;
	virtual HRESULT Release() = 0;
};
