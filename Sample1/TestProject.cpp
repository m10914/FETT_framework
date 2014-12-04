
#include "TestProject.h"



TestProject::TestProject():
	mVertexShader(NULL),
	mPixelShader(NULL),
	mVertexShaderReflection(NULL),
	mPixelShaderReflection(NULL),

	mVertexLayout(NULL),

	mCBNeverChanges(NULL),
	mCBChangeOnResize(NULL),
	mCBChangesEveryFrame(NULL),

	mTextureRV(NULL),
	
	mSamplerLinear(NULL),
	mBackbufferSampler(NULL),
	mDepthSampler(NULL),

	mVMeshColor( 0.7f, 0.7f, 0.7f, 1.0f ),

	mRTSecondTex(NULL),
	mRTSecondRV(NULL),
	mRTSecondRTV(NULL),
	mDSSecondTex(NULL),
	mDSSecondRV(NULL),
	mDSSecondDSV(NULL)
{

}

TestProject::~TestProject()
{
}


/*
===================================================================
===================================================================
*/
HRESULT TestProject::RenderScene()
{
	// Update our time
	static float t = 0.0f;

	static DWORD dwTimeStart = 0;
	DWORD dwTimeCur = GetTickCount();
	if( dwTimeStart == 0 )
		dwTimeStart = dwTimeCur;
	t = ( dwTimeCur - dwTimeStart ) / 1000.0f;


	camDesc.phi = t*0.5f;
	camDesc.theta = XM_PI*0.8f + sin(t*0.8)*0.6;
	CBChangesEveryFrame cb;
	cb.mView = XMMatrixTranspose( camDesc.getViewMatrix() );

	//setup common stuff
	ID3D11SamplerState* aSamplers[] = { mSamplerLinear, mBackbufferSampler, mDepthSampler };
	mImmediateContext->PSSetSamplers( 0, 3, aSamplers );


	//--------------------------------------------------------------------
	// C U B E

	// set RT and clear it
	mImmediateContext->OMSetRenderTargets( 1, &mRTSecondRTV, mDSSecondDSV);
	float ClearColor[4] = { 0.525f, 0.525f, 0.525f, 1.0f }; // red, green, blue, alpha
	mImmediateContext->ClearRenderTargetView( mRTSecondRTV, ClearColor );
	mImmediateContext->ClearDepthStencilView( mDSSecondDSV, D3D11_CLEAR_DEPTH, 1.0f, 0 );

	//setup world matrux for cube
	mWorld = XMMatrixRotationY( t );

	// Modify the color
	mVMeshColor.x = ( sinf( t * 1.0f ) + 1.0f ) * 0.5f;
	mVMeshColor.y = ( cosf( t * 3.0f ) + 1.0f ) * 0.5f;
	mVMeshColor.z = ( sinf( t * 5.0f ) + 1.0f ) * 0.5f;

	cb.mWorld = XMMatrixTranspose( mWorld );
	cb.vMeshColor = mVMeshColor;
	mImmediateContext->UpdateSubresource( mCBChangesEveryFrame, 0, NULL, &cb, 0, 0 );

	// render stuff
	mImmediateContext->IASetInputLayout( mVertexLayout );

	mImmediateContext->VSSetShader( mVertexShader, NULL, 0 );
	mImmediateContext->PSSetShader( mPixelShader, NULL, 0 );

	mImmediateContext->VSSetConstantBuffers( 0, 1, &mCBNeverChanges );
	mImmediateContext->VSSetConstantBuffers( 1, 1, &mCBChangeOnResize );
	mImmediateContext->VSSetConstantBuffers( 2, 1, &mCBChangesEveryFrame );

	mImmediateContext->PSSetConstantBuffers( 1, 1, &mCBChangeOnResize );
	mImmediateContext->PSSetConstantBuffers( 2, 1, &mCBChangesEveryFrame );

	mImmediateContext->PSSetShaderResources( 0, 1, &mTextureRV );
	mImmediateContext->PSSetShaderResources( 1, 1, &mTextureRV );
	mImmediateContext->PSSetShaderResources( 2, 1, &mTextureRV );
	
	cube.Render(mDevice, mImmediateContext);


	//--------------------------------------------------------------------
	// R E F L E C T

	mImmediateContext->OMSetRenderTargets( 1, &mRenderTargetView, mDepthStencilView);

	// again render cube
	cube.Render(mDevice, mImmediateContext);

	mImmediateContext->VSSetShader( mVertexShaderReflection, NULL, 0 );
	mImmediateContext->PSSetShader( mPixelShaderReflection, NULL, 0 );
	mImmediateContext->PSSetShaderResources( 1, 1, &mRTSecondRV );
	mImmediateContext->PSSetShaderResources( 2, 1, &mDSSecondRV );
	
	mWorld = XMMatrixTranslation(0,-1.5,0) * XMMatrixScaling(10,1,10);
	cb.mWorld = XMMatrixTranspose( mWorld );
	mImmediateContext->UpdateSubresource( mCBChangesEveryFrame, 0, NULL, &cb, 0, 0 );
	mImmediateContext->VSSetConstantBuffers( 2, 1, &mCBChangesEveryFrame );

	plane.Render(mDevice, mImmediateContext);

	ID3D11ShaderResourceView* view[] = { NULL, NULL, NULL };
	mImmediateContext->PSSetShaderResources( 0, 3, view );

	return S_OK;
}


/*
===========================================================================
===========================================================================
*/
HRESULT TestProject::InitScene()
{
	HRESULT hr;

	PrepareRT();

	// create objects
	cube.Init(mDevice);
	plane.Init(mDevice);


	//----------------------------------------------------------------------------
	// Create shaders, input layout and other stuff like that

	// Compile the vertex shader
	ID3DBlob* pVSBlob = NULL;
	hr = CompileShaderFromFile( "TestProjectShader.fx", "VS", "vs_4_0", &pVSBlob );
	if( FAILED( hr ) )
	{
		MessageBox( NULL,
			"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", "Error", MB_OK );
		return hr;
	}

	// Create the vertex shader
	hr = mDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &mVertexShader );
	if( FAILED( hr ) )
	{    
		pVSBlob->Release();
		return hr;
	}

	// Vertex shader for reflection
	pVSBlob = NULL;
	hr = CompileShaderFromFile( "TestProjectShader.fx", "VS_Reflection", "vs_4_0", &pVSBlob );
	if( FAILED( hr ) )
	{
		MessageBox( NULL,
			"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", "Error", MB_OK );
		return hr;
	}

	// Create the vertex shader
	hr = mDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &mVertexShaderReflection );
	if( FAILED( hr ) )
	{    
		pVSBlob->Release();
		return hr;
	}


	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE( layout );

	// Create the input layout
	hr = mDevice->CreateInputLayout( layout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &mVertexLayout );
	pVSBlob->Release();
	if( FAILED( hr ) )
		return hr;


	// Compile the pixel shader
	// 1st shader - simple rendering
	
	ID3DBlob* pPSBlob = NULL;
	hr = CompileShaderFromFile( "TestProjectShader.fx", "PS", "ps_4_0", &pPSBlob );
	if( FAILED( hr ) )
	{
		MessageBox( NULL,
			"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", "Error", MB_OK );
		return hr;
	}

	// Create the pixel shader
	// 2nd shader - reflection surface

	hr = mDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &mPixelShader );
	pPSBlob->Release();
	if( FAILED( hr ) )
		return hr;

	pPSBlob = NULL;
	hr = CompileShaderFromFile( "TestProjectShader.fx", "PS_Reflection", "ps_4_0", &pPSBlob);
	if( FAILED( hr ) )
	{
		MessageBox( NULL,
			"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", "Error", MB_OK );
		return hr;
	}

	// Create the pixel shader
	hr = mDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &mPixelShaderReflection );
	pPSBlob->Release();
	if( FAILED( hr ) )
		return hr;

	//----------------------------------------------------------------------------
	// Create the constant buffers

	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(CBNeverChanges);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = mDevice->CreateBuffer( &bd, NULL, &mCBNeverChanges );
	if( FAILED( hr ) )
		return hr;

	bd.ByteWidth = sizeof(CBChangeOnResize);
	hr = mDevice->CreateBuffer( &bd, NULL, &mCBChangeOnResize );
	if( FAILED( hr ) )
		return hr;

	bd.ByteWidth = sizeof(CBChangesEveryFrame);
	hr = mDevice->CreateBuffer( &bd, NULL, &mCBChangesEveryFrame );
	if( FAILED( hr ) )
		return hr;

	// Load the Texture
	hr = D3DX11CreateShaderResourceViewFromFile( mDevice, "seafloor.dds", NULL, NULL, &mTextureRV, NULL );
	if( FAILED( hr ) )
		return hr;

	// Create the sample state
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory( &sampDesc, sizeof(sampDesc) );
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	V_RETURN( mDevice->CreateSamplerState( &sampDesc, &mSamplerLinear ) );

	ZeroMemory( &sampDesc, sizeof(sampDesc) );
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	V_RETURN( mDevice->CreateSamplerState( &sampDesc, &mBackbufferSampler ) );
	V_RETURN( mDevice->CreateSamplerState( &sampDesc, &mDepthSampler ) );


	// Initialize the world matrices
	mWorld = XMMatrixIdentity();


	// Initialize the view matrix
	camDesc.radius = 15;
	camDesc.phi = 0;
	camDesc.theta = 0;
	camDesc.vAt = XMVectorSet(0,0,0,0);
	camDesc.vUp = XMVectorSet(0,1,0,0);
	camDesc.fov = XM_PIDIV4;
	camDesc.aspect = swapChainDesc.BufferDesc.Width / (FLOAT)swapChainDesc.BufferDesc.Height;
	camDesc.nearPlane = 0.1f;
	camDesc.farPlane = 500.0f;


	// Initialize the projection matrix
	mProjection = camDesc.getProjMatrix();

	CBChangeOnResize cbChangesOnResize;
	cbChangesOnResize.mProjection = XMMatrixTranspose( mProjection );
	cbChangesOnResize.mScreenParams =
		XMFLOAT4(
			swapChainDesc.BufferDesc.Width,
			swapChainDesc.BufferDesc.Height,
			camDesc.nearPlane,
			camDesc.farPlane
			);
	mImmediateContext->UpdateSubresource( mCBChangeOnResize, 0, NULL, &cbChangesOnResize, 0, 0 );

	return S_OK;
}


HRESULT TestProject::PrepareRT()
{
	HRESULT hr;

	// create G-Buffer albedo texture
	D3D11_TEXTURE2D_DESC Desc;
    ZeroMemory( &Desc, sizeof( D3D11_TEXTURE2D_DESC ) );
    Desc.ArraySize = 1;
    Desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    Desc.Usage = D3D11_USAGE_DEFAULT;
    Desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    Desc.Width = swapChainDesc.BufferDesc.Width;
    Desc.Height = swapChainDesc.BufferDesc.Height;
    Desc.MipLevels = 1;
    Desc.SampleDesc.Count = 1;
    V_RETURN( mDevice->CreateTexture2D( &Desc, NULL, &mRTSecondTex ) );

	D3D11_RENDER_TARGET_VIEW_DESC DescRT;
	DescRT.Format = Desc.Format;
	DescRT.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	DescRT.Texture2D.MipSlice = 0;
	V_RETURN( mDevice->CreateRenderTargetView( mRTSecondTex, &DescRT, &mRTSecondRTV ) );

	// Create the resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC DescRV;
	DescRV.Format = Desc.Format;
	DescRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	DescRV.Texture2D.MipLevels = 1;
	DescRV.Texture2D.MostDetailedMip = 0;
	V_RETURN( mDevice->CreateShaderResourceView( mRTSecondTex, &DescRV, &mRTSecondRV ) );


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
	hr = mDevice->CreateTexture2D( &descDepth, NULL, &mDSSecondTex );
	V_RETURN(hr);

	// Create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory( &descDSV, sizeof(descDSV) );
	descDSV.Format = DXGI_FORMAT_D32_FLOAT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = mDevice->CreateDepthStencilView( mDSSecondTex, &descDSV, &mDSSecondDSV );
	V_RETURN(hr);

	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;
	shaderResourceViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
	hr = mDevice->CreateShaderResourceView( mDSSecondTex, &shaderResourceViewDesc, &mDSSecondRV);
	V_RETURN(hr);
}





HRESULT TestProject::ReleaseScene()
{
	cube.Release();
	plane.Release();

	SAFE_RELEASE(mVertexShader);
	SAFE_RELEASE(mPixelShader);

	SAFE_RELEASE(mVertexLayout);

	SAFE_RELEASE(mCBNeverChanges);
	SAFE_RELEASE(mCBChangeOnResize);
	SAFE_RELEASE(mCBChangesEveryFrame);
	SAFE_RELEASE(mTextureRV);
	SAFE_RELEASE(mSamplerLinear);

	return S_OK;
}



XMMATRIX CameraDesc::getViewMatrix()
{
	XMVECTOR Eye = XMVectorSet(
		radius*cos(theta)*sin(phi),
		radius*sin(theta),
		radius*cos(theta)*cos(phi),
		0.0f );

	XMMATRIX viewMat = XMMatrixLookAtLH( Eye, vAt, vUp );

	return viewMat;
}

XMMATRIX CameraDesc::getProjMatrix()
{
	return XMMatrixPerspectiveFovLH( fov, aspect, nearPlane, farPlane );
}