/*
====================================================================

====================================================================
*/


#pragma once

#pragma warning(disable:4005)
//#define _XM_NO_INTRINSICS_ 1


#ifdef PROJ_TESTPROJECT

#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>
#include <xnamath.h>

#include <xmmintrin.h>

#include "dxapp.h"
#include "FUtil.h"

#include "camera.h"
#include "FPCube.h"
#include "FPPlane.h"
#include "Surface.h"


//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------

struct OceanDescription
{
    //perlin part
    float		PerlinSize;
    float       PerlinSpeed;
    XMFLOAT3	PerlinAmplitude;
    XMFLOAT3	PerlinOctave;
    XMFLOAT3	PerlinGradient;

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
};


struct __declspec(align(16)) CBPerlinBuffer
{
    float		PerlinSize;
    XMFLOAT3	PerlinAmplitude;
    XMFLOAT3	PerlinOctave;
    XMFLOAT3	PerlinGradient;
    XMFLOAT2    PerlinMovement;
};

struct __declspec(align(16)) CBChangesEveryFrame : CBMatrixSet, CBPerlinBuffer
{
    XMFLOAT4 mScreenParams;
    XMFLOAT4 mPerspectiveValues;

	XMFLOAT4 vMeshColor;	
	XMFLOAT4 SSRParams;

    XMFLOAT4 eyeVector;
    XMFLOAT4 sunDirection;
    XMFLOAT4 waterColor;
    XMFLOAT4 skyColor;
};




class __declspec(align(16)) TestProject : public DXApp
{
private:

    // shaders
	ID3D11VertexShader*                 mVertexShader;
	ID3D11PixelShader*                  mPixelShader;

    ID3D11VertexShader*                 mVertexShaderQuad;
    ID3D11PixelShader*                  mPixelShaderQuad;

	ID3D11VertexShader*                 mVertexShaderReflection;
	ID3D11PixelShader*                  mPixelShaderReflection;

    ID3D11VertexShader*                 mVertexShaderWater;
    ID3D11PixelShader*                  mPixelShaderWater;

    // input layouts pointers
    ID3D11InputLayout*                  mLayoutPT;
    ID3D11InputLayout*                  mLayoutPNT;

    // shader buffers
	ID3D11Buffer*                       mCBChangesEveryFrame;
    ID3D11Buffer*                       mCBforCS;
	
	ID3D11ShaderResourceView*           mTextureRV;
	ID3D11SamplerState*                 mSamplerLinear;
	ID3D11SamplerState*					mBackbufferSampler;
	ID3D11SamplerState*					mDepthSampler;

    // compute
    ID3D11ComputeShader*                mCS;
    ID3D11Buffer*                       mGridBuffer;
    ID3D11ShaderResourceView*           mGridBufferSRV;
    ID3D11UnorderedAccessView*          mGridBufferUAV;


	// alternative render target
	ID3D11Texture2D*					mRTSecondTex;
	ID3D11ShaderResourceView*			mRTSecondRV;
	ID3D11RenderTargetView*				mRTSecondRTV;
	ID3D11Texture2D*					mDSSecondTex;
	ID3D11ShaderResourceView*			mDSSecondRV;
	ID3D11DepthStencilView*				mDSSecondDSV;

    ID3D11RasterizerState*              mRSOrdinary;
    ID3D11RasterizerState*              mRSCullNone;
    ID3D11RasterizerState*              mRSWireframe;

	//----------------------------------
	// objects'n'stuff
	
    XMFLOAT4 mVMeshColor;

	DXCamera mainCamera;
    DXCamera observeCamera;
    bool bViewCameraMain;
    bool bControlCameraMain;

	FPCube	cube;
	FPPlane	plane;
    FSurface surface;

    CBChangesEveryFrame cb;
    OceanDescription oceanDesc;


public:
	TestProject();
	~TestProject();

	void *operator new (unsigned int size)
	{ return _mm_malloc(size, 16); }

	void operator delete (void *p)
	{ _mm_free(p); }


protected:

	HRESULT PrepareRT();
    
    //debug rendering
    void RenderCamera(LPD3DDeviceContext context, LPD3D11Device device, XMMATRIX* invViewProjMatrix);
    void RenderPoints(LPD3DDeviceContext context, LPD3D11Device device, XMVECTOR* points, int numOfPoints);
    void RenderQuad(LPD3DDeviceContext context, LPD3D11Device device, XMFLOAT2 offset, XMFLOAT2 relativeSize);


	virtual HRESULT RenderScene() override;
	virtual HRESULT InitScene() override;
	virtual HRESULT ReleaseScene() override;
    virtual HRESULT FrameMove() override;
};


#endif PROJ_TESTPROJECT
