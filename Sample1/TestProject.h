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

#include "RenderTarget.h"

#include "PassthruPostEffect.h"
#include "HBAOPostEffect.h"
#include "DOF.h"


class NVGcontext;


//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------



struct __declspec(align(16)) CBChangesEveryFrame : CBMatrixSet
{
    XMMATRIX matMVPLight;
	XMFLOAT4 vMeshColor;
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

    //ordinary scene rendering
	std::unique_ptr<FETTVertexShader> mVertexShader;
	std::unique_ptr<FETTPixelShader>  mPixelShader;

    // trail
    std::unique_ptr<FETTVertexShader> mTrailVS;
    std::unique_ptr<FETTPixelShader>  mTrailPS;
    std::unique_ptr<FETTGeometryShader> mTrailGS;


    //-----------------------------------------------------------
    // input layouts pointers
	ID3D11InputLayout*                  mLayoutPT = NULL;
	ID3D11InputLayout*                  mLayoutPNT = NULL;


    // shader constant buffers
	ID3D11Buffer*                       mCBChangesEveryFrame = NULL;
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

    std::unique_ptr<RenderTarget>       mSecondRT;


    std::unique_ptr<PassthruPostEffect> passthruPFX;

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
    CBforDeferredPass mMemBufferForDeferredPass;

    NVGcontext* vg = NULL;

public:
	TestProject();
	~TestProject();

	void *operator new (unsigned int size)
	{ return _mm_malloc(size, 16); }

	void operator delete (void *p)
	{ _mm_free(p); }


protected:

    //debug rendering
    void _renderCamera(LPD3DDeviceContext context, LPD3D11Device device, XMMATRIX* invViewProjMatrix);
    void _renderPoints(LPD3DDeviceContext context, LPD3D11Device device, XMVECTOR* points, int numOfPoints);

    // render stages
    void _renderSceneToGBuffer();

    void _renderSceneObjects(bool bPlane = true, bool bCubes = true);
    void _renderTrails();

    void _renderGui();

	virtual HRESULT RenderScene() override;
	virtual HRESULT InitScene() override;
	virtual HRESULT ReleaseScene() override;
    virtual HRESULT FrameMove() override;
};
