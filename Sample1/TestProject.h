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
#include "FPGrid.h"
#include "FPGridCube.h"

#include "d3d9.h" //for d3dperf stuff only!

#include "FLightSource.h"



//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------



struct __declspec(align(16)) CBChangesEveryFrame : CBMatrixSet
{
    XMMATRIX matMVPLight;
	XMFLOAT4 vMeshColor;
};


struct __declspec(align(16)) CBforCSReprojection
{
	XMMATRIX matMVPInv;
	XMMATRIX matMVPLight;
};

struct __declspec(align(16)) CBforDeferredPass
{
    XMMATRIX matMVPLight;
    XMMATRIX matMVPInv;
};


class __declspec(align(16)) TestProject : public DXApp
{
private:

    //-----------------------------------------------------------
    // shaders
	ID3D11VertexShader*                 mVertexShader = NULL;
	ID3D11PixelShader*                  mPixelShader = NULL;

	ID3D11VertexShader*                 mVertexShaderQuad = NULL;
	ID3D11PixelShader*                  mPixelShaderQuad = NULL;

    //special shaders
    ID3D11VertexShader*                 mVertexShaderRTW = NULL;
    ID3D11PixelShader*                  mPixelShaderRTW = NULL;
    ID3D11PixelShader*                  mPixelShaderDeferred = NULL;


    //-----------------------------------------------------------
    // input layouts pointers
	ID3D11InputLayout*                  mLayoutPT = NULL;
	ID3D11InputLayout*                  mLayoutPNT = NULL;


    // shader constant buffers
	ID3D11Buffer*                       mCBChangesEveryFrame = NULL;
	ID3D11Buffer*                       mCBforCS = NULL;
    ID3D11Buffer*                       mCBforDeferredPass = NULL;


	ID3D11ShaderResourceView*           mTextureRV = NULL;
	ID3D11SamplerState*                 mSamplerLinear = NULL;
	ID3D11SamplerState*					mBackbufferSampler = NULL;
	ID3D11SamplerState*					mDepthSampler = NULL;

	ID3D11RasterizerState*              mRSOrdinary = NULL;
    ID3D11RasterizerState*              mRSShadowMap = NULL;
	ID3D11RasterizerState*              mRSCullNone = NULL;
	ID3D11RasterizerState*              mRSWireframe = NULL;

	ID3D11DepthStencilState*			mDSOrdinary = NULL;
	ID3D11DepthStencilState*			mDSFullscreenPass = NULL;

    // compute
	ID3D11ComputeShader*                mComputeShaderReprojection = NULL;
    ID3D11ComputeShader*				mComputeShaderImportance = NULL;
    ID3D11ComputeShader*				mComputeShaderWarp = NULL;
    ID3D11ComputeShader*                mComputeShaderBlurWarp = NULL;


	// alternative render target
	ID3D11Texture2D*					mRTSecondTex = NULL;
	ID3D11ShaderResourceView*			mRTSecondSRV = NULL;
	ID3D11RenderTargetView*				mRTSecondRTV = NULL;
	ID3D11Texture2D*					mDSSecondTex = NULL;
	ID3D11ShaderResourceView*			mDSSecondSRV = NULL;
	ID3D11DepthStencilView*				mDSSecondDSV = NULL;


	// RT for shadow map
	ID3D11Texture2D*					mShadowMapTexture = NULL;
	ID3D11ShaderResourceView*			mShadowMapSRV = NULL;
	ID3D11DepthStencilView*				mShadowMapDSV = NULL;
	
    //>>>>>> unnecessary - for debug purpose only
    ID3D11Texture2D*                    mSMTempVis = NULL;
    ID3D11ShaderResourceView*           mSMTempVisSRV = NULL;
    ID3D11RenderTargetView*             mSMTempVisRTV = NULL;


	// a couple of textures for importance map
	ID3D11Buffer*						mReprojectionBuffer = NULL;
	ID3D11ShaderResourceView*			mReprojectionSRV = NULL;
	ID3D11UnorderedAccessView*			mReprojectionUAV = NULL;


    // swapchain for warp buffers
    ID3D11Buffer*						mWarpBuffer[2];
    ID3D11ShaderResourceView*			mWarpBufferSRV[2];
    ID3D11UnorderedAccessView*			mWarpBufferUAV[2];


	// stuff for visualizing - DEBUG purpose only
	//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
	
	ID3D11Buffer*						mReprojectionTransferBuffer = NULL;
	ID3D11Texture2D*					mReprojectionVisualTexture = NULL;
	ID3D11ShaderResourceView*			mReprojectionVisualSRV = NULL;

	ID3D11VertexShader*                 mVertexShaderBufferVis = NULL;
	ID3D11PixelShader*                  mPixelShaderBufferVis = NULL;

	//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


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

	// constant buffers
    CBChangesEveryFrame cb;
	CBforCSReprojection	mMemBufferForCSReprojection;
    CBforDeferredPass mMemBufferForDeferredPass;

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
    void _renderCamera(LPD3DDeviceContext context, LPD3D11Device device, XMMATRIX* invViewProjMatrix);
    void _renderPoints(LPD3DDeviceContext context, LPD3D11Device device, XMVECTOR* points, int numOfPoints);
    void _renderQuad(LPD3DDeviceContext context, LPD3D11Device device, XMFLOAT2 offset, XMFLOAT2 relativeSize);


    // render stages
    void _renderSceneToGBuffer();
    void _renderComputeWarpMaps();
    void _renderShadowMaps();
    void _renderDeferredShading();

    void _renderSceneObjects(bool bPlane = true, bool bCubes = true);

	virtual HRESULT RenderScene() override;
	virtual HRESULT InitScene() override;
	virtual HRESULT ReleaseScene() override;
    virtual HRESULT FrameMove() override;
};
