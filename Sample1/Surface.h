/*
================================================================


================================================================
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
#include "camera.h"


// structure to be processed in compute shader
// and used in vertex shader as main input
#define GRID_DIMENSION 128

struct __declspec(align(16)) CBForCS
{
    XMFLOAT4 dTexcoord;

    XMFLOAT4 vCorner0;
    XMFLOAT4 vCorner1;
    XMFLOAT4 vCorner2;
    XMFLOAT4 vCorner3;
};




class __declspec(align(16)) FSurface : public FPrimitive
{
public:
    FSurface();
    ~FSurface();

    XMMATRIX projectorWorldViewInverted;

    // FPrimitive implementation
    virtual	HRESULT Init(LPD3D11Device device) override;
    virtual HRESULT Render(LPD3DDeviceContext context) override;
    virtual HRESULT Release() override;

    void setCamera(DXCamera* camera) { mCamera = camera; };

    bool fillConstantBuffer(CBForCS& buffrer);

    //test
    XMFLOAT3 positions[32];
    int numOfPositions;

protected:

    

    // gridsize
    DXCamera* mCamera;
    int SizeX;
    int SizeY;
    bool bVisible;

    ID3D11Buffer*                       mIndexBuffer;

    //vectors storing plane and geometry
    XMVECTOR plane, upperPlane, lowerPlane; 


    //------------------------
    // M E T H O D S

    void initBuffer(LPD3D11Device device);
    bool getProjectedPointsMatrix(XMMATRIX& mat);
    XMVECTOR calcWorldPosOfCorner(XMFLOAT2 uv, XMMATRIX* matrix);
};