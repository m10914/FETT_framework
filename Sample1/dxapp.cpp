/*
======================================================================
======================================================================
*/

#include "dxapp.h"





DXApp::DXApp():
	mDriverType(D3D_DRIVER_TYPE_NULL),
	mFeatureLevel(D3D_FEATURE_LEVEL_11_0),
	mDevice(NULL),
	mImmediateContext(NULL),
	mSwapChain(NULL),
	mRenderTargetView(NULL),
	mDepthStencilView(NULL),
	mDepthStencil(NULL),
	mRenderTargetRV(NULL),
	mDepthStencilRV(NULL)
{
	//init vars here
}

DXApp::~DXApp()
{
	//destroy everything here
}




/*
======================================================================
======================================================================
*/
HRESULT DXApp::Init(HWND lHwnd)
{
	HRESULT hr = S_OK;

	mHwnd = lHwnd;

	RECT rc;
	GetClientRect( lHwnd, &rc );
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = ARRAYSIZE( driverTypes );

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE( featureLevels );

	DXGI_SWAP_CHAIN_DESC &sd = swapChainDesc;
	ZeroMemory( &sd, sizeof( sd ) );
	sd.BufferCount = 1;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
	sd.OutputWindow = lHwnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
	{
		mDriverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDeviceAndSwapChain(
			NULL,
			mDriverType,
			NULL,
			createDeviceFlags,
			featureLevels,
			numFeatureLevels,
			D3D11_SDK_VERSION,
			&sd, &mSwapChain, &mDevice, &mFeatureLevel, &mImmediateContext );
		if( SUCCEEDED( hr ) )
			break;
	}
	if( FAILED( hr ) )
		return hr;

	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	mImmediateContext->RSSetViewports( 1, &vp );


	CreateMainGBuffer( );


	this->InitScene();


	return S_OK;
}

HRESULT DXApp::CreateMainGBuffer()
{
	HRESULT hr;
	
	// ALBEDO
	ID3D11Texture2D* pBackBuffer = NULL;
	hr = mSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer );
	if( FAILED( hr ) )
		return hr;

	hr = mDevice->CreateRenderTargetView( pBackBuffer, NULL, &mRenderTargetView );
	pBackBuffer->Release();
	if( FAILED( hr ) )
		return hr;

	// create shader-resource-view
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;

	hr = mDevice->CreateShaderResourceView(pBackBuffer, &shaderResourceViewDesc, &mRenderTargetRV);
	V_RETURN(hr);


	// DEPTH_STENCIL
	D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory( &descDepth, sizeof(descDepth) );
	descDepth.Width = swapChainDesc.BufferDesc.Width;
	descDepth.Height = swapChainDesc.BufferDesc.Height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_R32_TYPELESS;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = mDevice->CreateTexture2D( &descDepth, NULL, &mDepthStencil );
	V_RETURN(hr);

	// Create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory( &descDSV, sizeof(descDSV) );
	descDSV.Format = DXGI_FORMAT_D32_FLOAT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = mDevice->CreateDepthStencilView( mDepthStencil, &descDSV, &mDepthStencilView );
	V_RETURN(hr);

	shaderResourceViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
	hr = mDevice->CreateShaderResourceView(mDepthStencil, &shaderResourceViewDesc, &mDepthStencilRV);
	V_RETURN(hr);



	

}




//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DX11
//--------------------------------------------------------------------------------------
HRESULT DXApp::CompileShaderFromFile( char* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut )
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;

#if defined( DEBUG ) || defined( _DEBUG )
	dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

	ID3DBlob* pErrorBlob;
	hr = D3DX11CompileFromFile( szFileName, NULL, NULL, szEntryPoint, szShaderModel, 
		dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL );
	if( FAILED(hr) )
	{
		if( pErrorBlob != NULL )
			OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );
		if( pErrorBlob ) pErrorBlob->Release();
		return hr;
	}
	if( pErrorBlob ) pErrorBlob->Release();

	return S_OK;
}




HRESULT DXApp::Render()
{	
	//------------------------------
	// Main G-Buffer handling

	//set render targets
	mImmediateContext->OMSetRenderTargets( 1, &mRenderTargetView, mDepthStencilView );

	// Clear the back buffer
	float ClearColor[4] = { 0.525f, 0.525f, 0.525f, 1.0f }; // red, green, blue, alpha
	mImmediateContext->ClearRenderTargetView( mRenderTargetView, ClearColor );

	// Clear the depth buffer to 1.0 (max depth)
	mImmediateContext->ClearDepthStencilView( mDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0 );
	

	this->RenderScene();


	// Present our back buffer to our front buffer
	mSwapChain->Present( 0, 0 );

	return S_OK;
}



HRESULT DXApp::Release()
{
	this->ReleaseScene();

	if( mImmediateContext ) mImmediateContext->ClearState();

	SAFE_RELEASE( mDepthStencil );
	SAFE_RELEASE( mDepthStencilView );
	SAFE_RELEASE( mRenderTargetView );
	SAFE_RELEASE( mSwapChain );
	SAFE_RELEASE( mImmediateContext );

	SAFE_RELEASE( mDevice );

	return S_OK;
}


