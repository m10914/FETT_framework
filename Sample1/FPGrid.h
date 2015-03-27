/*
=============================================================================

Grid primitive

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


class __declspec(align(16)) FPGrid : public FPrimitive
{
public:
    /// width must be multiple of 2
    FPGrid(int width);
    ~FPGrid();


    // FPrimitive implementation
    virtual	HRESULT Init(LPD3D11Device device) override;
    virtual HRESULT Render(LPD3DDeviceContext context) override;
    virtual HRESULT Release() override;

protected:
    ID3D11Buffer*                       mVertexBuffer = NULL;
    ID3D11Buffer*                       mIndexBuffer = NULL;

    ///must be multiplier of 2
    int mGridWidth = 0;
    int numOfVertices = 0;
    int numOfIndices = 0;
    int numOfFaces = 0;

};





