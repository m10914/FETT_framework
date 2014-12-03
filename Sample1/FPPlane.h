/*
=============================================================================

Plane primitive

=============================================================================
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
#include "FPrimitive.h"


class __declspec(align(16)) FPPlane : public FPrimitive
{
public:

	FPPlane();
	~FPPlane();


	// FPrimitive implementation
	virtual	HRESULT Init(LPD3D11Device device) override;
	virtual HRESULT Render(LPD3D11Device device, LPD3DDeviceContext context) override;
	virtual HRESULT Release() override;

protected:
	ID3D11Buffer*                       mVertexBuffer;
	ID3D11Buffer*                       mIndexBuffer;

};





