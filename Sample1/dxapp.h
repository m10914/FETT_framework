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


//some defines
#define LPD3D11Device ID3D11Device*
#define LPD3DDeviceContext ID3D11DeviceContext*

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


struct __declspec(align(16)) SimpleVertex
{
	XMFLOAT3 Pos;
	XMFLOAT2 Tex;
};




class DXApp
{

public:
	DXApp();
	~DXApp();

	HRESULT Init(HWND lHwnd);
	HRESULT Render();
	HRESULT Release();

protected:
	HRESULT CompileShaderFromFile( char* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut );

	HRESULT CreateMainGBuffer();

	virtual HRESULT RenderScene() = 0;
	virtual HRESULT InitScene() = 0;
	virtual HRESULT ReleaseScene() = 0;

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
};


