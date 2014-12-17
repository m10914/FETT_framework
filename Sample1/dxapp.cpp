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
	// input
	bFirstCapture = true;
	DIKeyboard = NULL;
	DIMouse = NULL;

    // measurements
    deltaTime = 0;
    totalTime = 0;
    currentFrame = 0;
    startTime = 0;
}

DXApp::~DXApp()
{
	VertexFormatMgr::release();
}




/*
======================================================================
======================================================================
*/
HRESULT DXApp::Init(HWND lHwnd, HINSTANCE hInstance)
{
	HRESULT hr = S_OK;

	//create dinput
	{
		hr = DirectInput8Create(hInstance,
			DIRECTINPUT_VERSION,
			IID_IDirectInput8,
			(void**)&DirectInput,
			NULL); 
		hr = DirectInput->CreateDevice(GUID_SysKeyboard,
			&DIKeyboard,
			NULL);

		hr = DirectInput->CreateDevice(GUID_SysMouse,
			&DIMouse,
			NULL);

		hr = DIKeyboard->SetDataFormat(&c_dfDIKeyboard);
		hr = DIKeyboard->SetCooperativeLevel(lHwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);

		hr = DIMouse->SetDataFormat(&c_dfDIMouse);
		hr = DIMouse->SetCooperativeLevel(lHwnd, DISCL_NONEXCLUSIVE | DISCL_NOWINKEY | DISCL_FOREGROUND);

        DIKeyboard->Acquire();
        DIMouse->Acquire();
	}

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

    // create G-Buffer
	CreateMainGBuffer( );

    // create timers
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    ticksPerSecond = frequency.QuadPart;

    startTime = getCurrentComputerTime();


    // launch virtual function
	this->InitScene();


	return S_OK;
}

double DXApp::getCurrentComputerTime()
{
    LARGE_INTEGER t1;
    QueryPerformanceCounter(&t1);
    return t1.QuadPart * 1000.0 / ticksPerSecond;
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



HRESULT DXApp::PreRender()
{
    // First thing - get frame move
    {
        DIMOUSESTATE mouseCurrState;

        DIMouse->GetDeviceState(sizeof(DIMOUSESTATE), &mouseCurrState);
        DIKeyboard->GetDeviceState(sizeof(keyboardState), (LPVOID)&keyboardState);
        

        if(bFirstCapture)
        {
            bFirstCapture = false;
            mouseDX = 0;
            mouseDY = 0;
            mouseDZ = 0;
        }
        else
        {
            mouseDX = mouseCurrState.lX;// - mouseLastState.lX;
            mouseDZ = mouseCurrState.lZ;// - mouseLastState.lZ;
            mouseDY = mouseCurrState.lY;// - mouseLastState.lY;
        }

        mouseLastState = mouseCurrState;
    }


    if(startTime == 0)
    {
        startTime = getCurrentComputerTime();
        deltaTime = 0;
        lastFrameTime = getCurrentComputerTime();
        totalTime = 0;
    }
    else
    {
        deltaTime = getCurrentComputerTime() - lastFrameTime;
        totalTime = getCurrentComputerTime() - startTime;
        lastFrameTime = getCurrentComputerTime();
    }
    currentFrame++;

    this->FrameMove();

    return S_OK;
}


HRESULT DXApp::Render()
{	
    // prerender routine

	PreRender();


	//------------------------------
	// Main G-Buffer handling

	mImmediateContext->OMSetRenderTargets( 1, &mRenderTargetView, mDepthStencilView );

	float ClearColor[4] = { 0.525f, 0.525f, 0.525f, 1.0f }; // red, green, blue, alpha
	mImmediateContext->ClearRenderTargetView( mRenderTargetView, ClearColor );
	mImmediateContext->ClearDepthStencilView( mDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0 );
	

	this->RenderScene();


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

	//dinput
	{
		DIKeyboard->Unacquire();
		DIMouse->Unacquire();
		DirectInput->Release();
	}

	return S_OK;
}


