/*
=============================================================================

Cube primitive

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

#include <vector>
#include "MathHelper.h"
#include <map>


#define KP(a,b) UINT(max(a,b)) << 16 | UINT(min(a,b))

class __declspec(align(16)) FPCube : public FPrimitive
{
public:

	FPCube();
	~FPCube();


	// FPrimitive implementation
	virtual	HRESULT Init(LPD3D11Device device) override;
	virtual HRESULT Render(LPD3DDeviceContext context) override;
	virtual HRESULT Release() override;

    void tesselate(float dist, LPD3D11Device device, LPD3DDeviceContext context); //subdivision
    
protected:
	ID3D11Buffer*                       mVertexBuffer;
	ID3D11Buffer*                       mIndexBuffer;

    int numPrims;

    void tesselateTriangle(
        float tessDist,
        std::vector<VertexFormatPT>& vertices,
        std::vector<WORD>& indices,
        std::map<UINT, WORD>& newVertices,
        WORD* triangleIndices,
        WORD** outTriangleIndices, int& outNumTriangles);
};





