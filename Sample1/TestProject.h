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
#include "d3d9.h" //for d3dperf stuff only!

#include "FLightSource.h"



//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------



struct __declspec(align(16)) CBChangesEveryFrame : CBMatrixSet
{
	XMFLOAT4 vMeshColor;
};




class __declspec(align(16)) TestProject : public DXApp
{
private:

    // shaders
	ID3D11VertexShader*                 mVertexShader = NULL;
	ID3D11PixelShader*                  mPixelShader = NULL;

	ID3D11VertexShader*                 mVertexShaderQuad = NULL;
	ID3D11PixelShader*                  mPixelShaderQuad = NULL;

    // input layouts pointers
	ID3D11InputLayout*                  mLayoutPT = NULL;
	ID3D11InputLayout*                  mLayoutPNT = NULL;

    // shader buffers
	ID3D11Buffer*                       mCBChangesEveryFrame = NULL;
	ID3D11Buffer*                       mCBforCS = NULL;
	
	ID3D11ShaderResourceView*           mTextureRV = NULL;
	ID3D11SamplerState*                 mSamplerLinear = NULL;
	ID3D11SamplerState*					mBackbufferSampler = NULL;
	ID3D11SamplerState*					mDepthSampler = NULL;

    // compute
	ID3D11ComputeShader*                mCS = NULL;
	ID3D11Buffer*                       mGridBuffer = NULL;
	ID3D11ShaderResourceView*           mGridBufferSRV = NULL;
	ID3D11UnorderedAccessView*          mGridBufferUAV = NULL;


	// alternative render target
	ID3D11Texture2D*					mRTSecondTex = NULL;
	ID3D11ShaderResourceView*			mRTSecondRV = NULL;
	ID3D11RenderTargetView*				mRTSecondRTV = NULL;
	ID3D11Texture2D*					mDSSecondTex = NULL;
	ID3D11ShaderResourceView*			mDSSecondRV = NULL;
	ID3D11DepthStencilView*				mDSSecondDSV = NULL;


	// RT for shadow map
	ID3D11Texture2D*					mShadowMapTexture = NULL;
	ID3D11ShaderResourceView*			mShadowMapSRV = NULL;
	ID3D11DepthStencilView*				mShadowMapDSV = NULL;


	ID3D11RasterizerState*              mRSOrdinary = NULL;
	ID3D11RasterizerState*              mRSCullNone = NULL;
    ID3D11RasterizerState*              mRSWireframe = NULL;

	ID3D11DepthStencilState*			mDSOrdinary = NULL;
	ID3D11DepthStencilState*			mDSFullscreenPass = NULL;

	//----------------------------------
	// objects'n'stuff
	
    XMFLOAT4 mVMeshColor;

	DXCamera mainCamera;
    
	enum ControlState
	{
		CS_Default = 0,
		CS_FPSCamera,
		CS_MoveLight,

		CS_NumStates
	};
	ControlState mCurrentControlState = CS_Default;

	FPCube	cube;
	FPPlane	plane;

	FDirectionalLight mDirLight;


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
    void RenderQuad(LPD3DDeviceContext context, LPD3D11Device device, XMFLOAT2 offset, XMFLOAT2 relativeSize);


	virtual HRESULT RenderScene() override;
	virtual HRESULT InitScene() override;
	virtual HRESULT ReleaseScene() override;
    virtual HRESULT FrameMove() override;
};


#endif PROJ_TESTPROJECT
