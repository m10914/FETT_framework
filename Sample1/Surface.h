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
#include "FFT/ocean_simulator.h"

#define GRID_DIMENSION 256 //must be a multiple of 16
#define FRESNEL_TEX_SIZE 256
#define PERLIN_TEX_SIZE 64

struct OceanDescription
{
   //perlin buffer
   float PerlinSize;
   float PerlinSpeed;
   XMFLOAT3	PerlinAmplitude;
   XMFLOAT3	PerlinOctave;
   XMFLOAT3	PerlinGradient;

   XMFLOAT2 PerlinMove;

   // common part

   // Must be power of 2.
   int dmap_dim;
   // Typical value is 1000 ~ 2000
   float patch_length;
   // Adjust the time interval for simulation.
   float time_scale;
   // Amplitude for transverse wave. Around 1.0
   float wave_amplitude;
   // Wind direction. Normalization not required.
   XMFLOAT2 wind_dir;
   // Around 100 ~ 1000
   float wind_speed;
   // This value damps out the waves against the wind direction.
   // Smaller value means higher wind dependency.
   float wind_dependency;
   // The amplitude for longitudinal wave. Must be positive.
   float choppy_scale;


   //---------------------------------
   // methods

   void update(double appTime);
   void set(
       float iPerlinSize,
       float iPerlinSpeed,
       XMFLOAT3	iPerlinAmplitude,
       XMFLOAT3	iPerlinOctave,
       XMFLOAT3	iPerlinGradient,
       int idmap_dim,
       float ipatch_length,
       float itime_scale,
       float iwave_amplitude,
       XMFLOAT2 iwind_dir,
       float iwind_speed,
       float iwind_dependency,
       float ichoppy_scale);
};



// structure to be processed in compute shader
// and used in vertex shader as main input
struct __declspec(align(16)) CBForCS
{
    XMFLOAT4 dTexcoord;

    XMFLOAT4 vCorner0;
    XMFLOAT4 vCorner1;
    XMFLOAT4 vCorner2;
    XMFLOAT4 vCorner3;

    XMMATRIX worldMatrix;
    XMFLOAT4 eyePosition;

    XMFLOAT4	PerlinSize;
    XMFLOAT4	PerlinAmplitude;
    XMFLOAT4	PerlinOctave;
    XMFLOAT4	PerlinGradient;
    XMFLOAT4    PerlinMovement;
};



class __declspec(align(16)) FSurface : public FPrimitive
{
public:
    FSurface();
    ~FSurface();

    XMMATRIX projectorWorldViewInverted;

    // FPrimitive implementation
    HRESULT Init(LPD3D11Device device, OceanDescription* desc);
    virtual HRESULT Init(LPD3D11Device device) override { return S_OK; };
    virtual HRESULT Render(LPD3DDeviceContext context) override;
    virtual HRESULT Release() override;
    
    void Update(double appTime, double deltaTime);
    void setCamera(DXCamera* camera) { mCamera = camera; };

    bool fillConstantBuffer(CBForCS& buffrer, double deltaTime);

    //test
    XMFLOAT3 positions[32];
    int numOfPositions;

    //TODO: refactor this system
    OceanSimulator* mOceanSimulator;
    ID3D11ShaderResourceView* pFresnelSRV = NULL;
    ID3D11ShaderResourceView* pPerlinSRV = NULL;

protected:
    
    void initOcean(LPD3D11Device device);
    void releaseOcean();
    void updateOcean();


    // gridsize
    DXCamera* mCamera;
    int SizeX;
    int SizeY;
    bool bVisible;

    ID3D11Buffer*                       mIndexBuffer = NULL;
    ID3D11Texture1D*                    pFresnelTexture = NULL;

    OceanDescription*                   pOceanDesc = NULL;

    //vectors storing plane and geometry
    XMVECTOR plane, upperPlane, lowerPlane; 


    //------------------------
    // M E T H O D S

    void initBuffer(LPD3D11Device device);
    bool getProjectedPointsMatrix(XMMATRIX& mat);
    XMVECTOR calcWorldPosOfCorner(XMFLOAT2 uv, XMMATRIX* matrix);
};