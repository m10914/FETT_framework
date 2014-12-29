/*
====================================================================

====================================================================
*/
#pragma once

#pragma warning(disable:4005)
//#define _XM_NO_INTRINSICS_ 1

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


struct __declspec(align(16)) CBChangesEveryFrame : CBMatrixSet
{
    XMFLOAT4 mScreenParams;
    XMFLOAT4 mPerspectiveValues;

	XMFLOAT4 vMeshColor;	
	XMFLOAT4 SSRParams;  
};




class __declspec(align(16)) TestProject : public DXApp
{
private:

    // shaders
	ID3D11VertexShader*                 mVertexShader;
	ID3D11PixelShader*                  mPixelShader;

	ID3D11VertexShader*                 mVertexShaderReflection;
	ID3D11PixelShader*                  mPixelShaderReflection;

    ID3D11VertexShader*                 mVertexShaderWater;
    ID3D11PixelShader*                  mPixelShaderWater;

    // input layouts pointers
    ID3D11InputLayout*                  mLayoutPT;
    ID3D11InputLayout*                  mLayoutPNT;

    // shader buffers
	ID3D11Buffer*                       mCBChangesEveryFrame;
	
	ID3D11ShaderResourceView*           mTextureRV;
	ID3D11SamplerState*                 mSamplerLinear;
	ID3D11SamplerState*					mBackbufferSampler;
	ID3D11SamplerState*					mDepthSampler;

	// alternative render target
	ID3D11Texture2D*					mRTSecondTex;
	ID3D11ShaderResourceView*			mRTSecondRV;
	ID3D11RenderTargetView*				mRTSecondRTV;
	ID3D11Texture2D*					mDSSecondTex;
	ID3D11ShaderResourceView*			mDSSecondRV;
	ID3D11DepthStencilView*				mDSSecondDSV;

    ID3D11RasterizerState*              mRSOrdinary;
    ID3D11RasterizerState*              mRSCullNone;

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

	virtual HRESULT RenderScene() override;
	virtual HRESULT InitScene() override;
	virtual HRESULT ReleaseScene() override;
    virtual HRESULT FrameMove() override;
};


