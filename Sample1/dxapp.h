/*
============================================================================
Basic framework class for all application.


============================================================================
*/

#pragma once
#pragma warning(disable:4005 4324)

#include <windows.h>

#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>

#include <DxErr.h>
#include <xnamath.h>

#include "VertexFormats.h"

#pragma comment (lib, "dinput8.lib")
#pragma comment (lib, "dxguid.lib")
#include <dinput.h>

//some defines
#define LPD3D11Device ID3D11Device*
#define LPD3DDeviceContext ID3D11DeviceContext*
#define LPD3D11Buffer ID3D11Buffer*


#define SAFE_RELEASE(x)		if (x) { (x)->Release();		(x) = NULL; }	//!< Safe D3D-style release

#if defined(_DEBUG)
#ifndef V
#define V(x)           { hr = (x); if( FAILED(hr) ) { DXTrace( __FILE__, (DWORD)__LINE__, hr, #x, true ); } }
#endif
#ifndef V_RETURN
#define V_RETURN(x)    { hr = (x); if( FAILED(hr) ) { return DXTrace( __FILE__, (DWORD)__LINE__, hr, #x, true ); } }
#endif
#else
#ifndef V
#define V(x)           { hr = (x); }
#endif
#ifndef V_RETURN
#define V_RETURN(x)    { hr = (x); if( FAILED(hr) ) { return hr; } }
#endif
#endif

#define KEYSTATE_DOWN 0x80



struct __declspec(align(16)) CBMatrixSet
{
    XMMATRIX mWorld;
    XMMATRIX mView;
    XMMATRIX mProjection;
};




class DXApp
{

public:
	DXApp();
	~DXApp();

	HRESULT Init(HWND lHwnd, HINSTANCE hInstance);
	HRESULT Render();
	HRESULT Release();
    HRESULT PreRender();

protected:

	HRESULT CreateMainGBuffer();

	virtual HRESULT RenderScene() = 0;
	virtual HRESULT InitScene() = 0;
	virtual HRESULT ReleaseScene() = 0;

    virtual HRESULT FrameMove() = 0; //pre-render function

protected:
	HWND mHwnd;

	D3D_DRIVER_TYPE  mDriverType;
	D3D_FEATURE_LEVEL mFeatureLevel;

	DXGI_SWAP_CHAIN_DESC swapChainDesc;

	ID3D11Device*                       mDevice;
	ID3D11DeviceContext*                mImmediateContext;
	IDXGISwapChain*                     mSwapChain;

	// main GBuffer
	ID3D11RenderTargetView*             mRenderTargetView;
	ID3D11Texture2D*                    mDepthStencil;
	ID3D11DepthStencilView*             mDepthStencilView;

	ID3D11ShaderResourceView*			mRenderTargetRV;
	ID3D11ShaderResourceView*			mDepthStencilRV;


	//------------------------------------------------------------
	// input 

	IDirectInputDevice8* DIKeyboard;
	IDirectInputDevice8* DIMouse;
	DIMOUSESTATE mouseLastState;
	LPDIRECTINPUT8 DirectInput;

	bool bFirstCapture;
	LONG mouseDX, mouseDY, mouseDZ;
	BYTE keyboardState[256];

public:
	bool isKeyInState(BYTE key)
	{
		return keyboardState[key] & KEYSTATE_DOWN == KEYSTATE_DOWN;
	};


    //-----------------------------------------------------
    // measurements

    double getCurrentComputerTime();


protected:
    double deltaTime; //in ms
    double totalTime; //in ms
    int currentFrame;

private:
    double startTime;
    double lastFrameTime;
    double ticksPerSecond;
};


