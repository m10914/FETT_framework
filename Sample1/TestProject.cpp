/*
====================================================================

====================================================================
*/

#ifdef PROJ_TESTPROJECT


#include "TestProject.h"

#include "camera.h"
#include "FPCube.h"
#include "FPPlane.h"


DXApp* initApplication()
{
    return new TestProject();
}



TestProject::TestProject():
	mVMeshColor( 0.7f, 0.7f, 0.7f, 1.0f )
{
}

TestProject::~TestProject()
{
}



HRESULT TestProject::FrameMove()
{
    // keyboard
    //----------------------------------

    if(isKeyPressed(DIK_ESCAPE))
        exit(1);
	if (isKeyPressed(DIK_TAB))
	{
		//change mode
		mCurrentControlState = (ControlState)(mCurrentControlState + 1);
		if (mCurrentControlState == CS_NumStates)
			mCurrentControlState = CS_Default;
	}


    // move camera
    XMFLOAT3 offset = XMFLOAT3(0,0,0);
    if(isKeyDown(DIK_W))
    {
        offset.x += deltaTime * 0.001;
    }
    if(isKeyDown(DIK_S))
    {
        offset.x -= deltaTime * 0.001;
    }
    if(isKeyDown(DIK_A))
    {
        offset.z += deltaTime * 0.001;
    }
    if(isKeyDown(DIK_D))
    {
        offset.z -= deltaTime * 0.001;
    }
    if(isKeyDown(DIK_E))
    {
        offset.y += deltaTime * 0.001;
    }
    if(isKeyDown(DIK_Q))
    {
        offset.y -= deltaTime * 0.001;
    }


    // update camera and lights
    //---------------
    
    // keyboard camera control - if mouse is disabled somehow
    if(!isLMBDown())
    {
        mouseDX = 0;
        mouseDY = 0;
        if(isKeyDown(DIK_I))
            mouseDY += deltaTime * 5.0f;
        if(isKeyDown(DIK_K))
            mouseDY -= deltaTime * 5.0f;
        if(isKeyDown(DIK_J))
            mouseDX -= deltaTime * 5.0f;
        if(isKeyDown(DIK_L))
            mouseDX += deltaTime * 5.0f;
    }

	// update camera and lightsource
    if(mCurrentControlState == CS_Default)
        mainCamera.FrameMove(offset, XMFLOAT3(mouseDX, mouseDY, mouseDZ));
    else
        mainCamera.FrameMove(XMFLOAT3(0,0,0), XMFLOAT3(0,0,0));

	if (mCurrentControlState == CS_FPSCamera)
	{
		//TODO
	}

	if (mCurrentControlState == CS_MoveLight)
		mDirLight.FrameMove(XMFLOAT3(0,0,0), XMFLOAT3(mouseDX, mouseDY, mouseDZ));
	else mDirLight.FrameMove(XMFLOAT3(0, 0, 0), XMFLOAT3(0, 0, 0));



    // update some buffers
    //---------------

	DXCamera* curCamera = &mainCamera; //select camera

    cb.mView = XMMatrixTranspose( curCamera->getViewMatrix() );


    XMMATRIX mProjection = curCamera->getProjMatrix();
    cb.mProjection = XMMatrixTranspose( mProjection );

    // Modify the color based on time
    mVMeshColor.x = ( sinf( totalTime * 0.001f ) + 1.0f ) * 0.5f;
    mVMeshColor.y = ( cosf( totalTime * 0.003f ) + 1.0f ) * 0.5f;
    mVMeshColor.z = ( sinf( totalTime * 0.005f ) + 1.0f ) * 0.5f;
    cb.vMeshColor = mVMeshColor;


    //-----------------------------------------------
    // update objects




    return S_OK;
}


/*
===================================================================
===================================================================
*/
HRESULT TestProject::RenderScene()
{
	//setup common stuff
	ID3D11SamplerState* aSamplers[] = { mSamplerLinear, mBackbufferSampler, mDepthSampler };
	mImmediateContext->PSSetSamplers(0, 3, aSamplers);
	mImmediateContext->VSSetSamplers(0, 3, aSamplers);
	mImmediateContext->CSSetSamplers(0, 3, aSamplers);


	//--------------------------------------------------------------------
	// TO SECOND RT

	D3DPERF_BeginEvent(D3DCOLOR_RGBA(255, 0, 255, 0), L"RT render");

	UINT sref = 0;
	mImmediateContext->OMSetDepthStencilState(mDSOrdinary, sref);


	// set RT and clear it
	mImmediateContext->OMSetRenderTargets(1, &mRTSecondRTV, mDSSecondDSV);
	float ClearColor[4] = { 0.525f, 0.525f, 0.525f, 1.0f }; // red, green, blue, alpha
	mImmediateContext->ClearRenderTargetView(mRTSecondRTV, ClearColor);
	mImmediateContext->ClearDepthStencilView(mDSSecondDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);



	// initial preparation
	mImmediateContext->IASetInputLayout(mLayoutPT);

	mImmediateContext->VSSetShader(mVertexShader, NULL, 0);
	mImmediateContext->PSSetShader(mPixelShader, NULL, 0);

	mImmediateContext->VSSetConstantBuffers(0, 1, &mCBChangesEveryFrame);
	mImmediateContext->PSSetConstantBuffers(0, 1, &mCBChangesEveryFrame);

	mImmediateContext->PSSetShaderResources(0, 1, &mTextureRV);
	mImmediateContext->PSSetShaderResources(1, 1, &mTextureRV);
	mImmediateContext->PSSetShaderResources(2, 1, &mTextureRV);


	// render cubes
	// it's the same cube actually
	if (true)
	{
		D3DPERF_BeginEvent(D3DCOLOR_RGBA(0, 255, 0, 0), L"Render scene - g-buffer");

		//plane
		plane.position = XMFLOAT3(0, -0.5, 0);
		plane.scale = XMFLOAT3(100, 1, 100);
		FUtil::RenderPrimitive(&plane, mImmediateContext, cb, mCBChangesEveryFrame);

		//cubes
		cube.position = XMFLOAT3(0, 0, 0);
		FUtil::RenderPrimitive(&cube, mImmediateContext, cb, mCBChangesEveryFrame);

		cube.position = XMFLOAT3(3, 0, 0);
		FUtil::RenderPrimitive(&cube, mImmediateContext, cb, mCBChangesEveryFrame);

		cube.position = XMFLOAT3(-3, 0, 0);
		FUtil::RenderPrimitive(&cube, mImmediateContext, cb, mCBChangesEveryFrame);

		D3DPERF_EndEvent();
	}


	D3DPERF_EndEvent();





	//--------------------------------------------------------------------
	// DEFERRED

	D3DPERF_BeginEvent(D3DCOLOR_RGBA(0, 255, 255, 0), L"-= Shadow Maps =-");


	// placeholder - just render from light's position
	mImmediateContext->OMSetRenderTargets(0, NULL, mShadowMapDSV);
	mImmediateContext->ClearDepthStencilView(mShadowMapDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

	//--------------------------------
	// do GPU computing 
	{
		//build buffer
		{
			mDirLight.GetMVPMatrix(&mMemBufferForCS.matMVPLight);

			XMVECTOR det;
			XMMATRIX vmat = XMMatrixInverse(&det,
					XMMatrixTranspose(mainCamera.getProjMatrix()) *
					XMMatrixTranspose(mainCamera.getViewMatrix())	);
			
			mMemBufferForCS.matMVPInv = vmat;

			mImmediateContext->UpdateSubresource(mCBforCS, 0, NULL, &mMemBufferForCS, 0, 0);
		}

		D3DPERF_BeginEvent(D3DCOLOR_RGBA(0, 0, 255, 0), L"reprojection");

		ID3D11Buffer* csbuffers[1] = { mCBforCS };
		mImmediateContext->CSSetShader(mCS, NULL, 0);
		mImmediateContext->CSSetConstantBuffers(0, 1, csbuffers);

		// bind resources

		static float val[1048576];
		memset(&val, 0, sizeof(float)* 1048576);
		mImmediateContext->UpdateSubresource(mReprojectionBuffer, 0, NULL, &val, 0, 0);
		ID3D11UnorderedAccessView* aUAViews[1] = { mReprojectionUAV };
		mImmediateContext->CSSetUnorderedAccessViews(0, 1, aUAViews, (UINT*)(&aUAViews));

		ID3D11ShaderResourceView* srviews[1] = { mDSSecondSRV };
		mImmediateContext->CSSetShaderResources(0, 1, srviews);

		// launch!
		int tG = ceil(1024 / 16.f);
		mImmediateContext->Dispatch(tG, tG, 1); //numthreads(16,16,1)

		// unbind
		aUAViews[0] = NULL;
		mImmediateContext->CSSetUnorderedAccessViews(0, 1, aUAViews, (UINT*)(&aUAViews));
		srviews[0] = NULL;
		mImmediateContext->CSSetShaderResources(0, 1, srviews);
		csbuffers[0] = NULL;
		mImmediateContext->CSSetConstantBuffers(0, 1, csbuffers);

		if (true)
		{
			// visualize

			// copy to intermediate buffer
			mImmediateContext->CopyResource(mReprojectionTransferBuffer, mReprojectionBuffer);

			// copy to texture
			D3D11_MAPPED_SUBRESOURCE msSrc, msDest;
			mImmediateContext->Map(mReprojectionTransferBuffer, 0, D3D11_MAP_READ, 0, &msSrc);
			mImmediateContext->Map(mReprojectionVisualTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &msDest);

			memcpy(msDest.pData, msSrc.pData, sizeof(float)* 1024 * 1024);

			mImmediateContext->Unmap(mReprojectionVisualTexture, 0);
			mImmediateContext->Unmap(mReprojectionTransferBuffer, 0);
		}


		D3DPERF_EndEvent();
	}


	mDirLight.GetProjectionMatrix(&cb.mProjection);
	mDirLight.GetTransformMatrix(&cb.mView);
	cb.mWorld = XMMatrixIdentity();

	// 1st stage - reprojection to texture
	{
		//plane
		plane.position = XMFLOAT3(0, -0.5, 0);
		plane.scale = XMFLOAT3(100, 1, 100);
		FUtil::RenderPrimitive(&plane, mImmediateContext, cb, mCBChangesEveryFrame);

		//cubes
		cube.position = XMFLOAT3(0, 0, 0);
		FUtil::RenderPrimitive(&cube, mImmediateContext, cb, mCBChangesEveryFrame);

		cube.position = XMFLOAT3(3, 0, 0);
		FUtil::RenderPrimitive(&cube, mImmediateContext, cb, mCBChangesEveryFrame);

		cube.position = XMFLOAT3(-3, 0, 0);
		FUtil::RenderPrimitive(&cube, mImmediateContext, cb, mCBChangesEveryFrame);
	}

	D3DPERF_EndEvent();


	// 2nd stage - compute shaders: calculating importance maps (2 1D textures)


	// 3rd stage - render to shadows


	// 4th stage - rendering with shadows

	D3DPERF_BeginEvent(D3DCOLOR_RGBA(255, 255, 222, 0), L"Backbuffer render");

	mImmediateContext->OMSetRenderTargets(1, &mRenderTargetView, mDepthStencilView);
	mImmediateContext->ClearRenderTargetView(mRenderTargetView, ClearColor);
	mImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	{
        D3DPERF_BeginEvent(D3DCOLOR_RGBA(0, 255, 0, 0), L"Deferred");

		mImmediateContext->OMSetDepthStencilState(mDSFullscreenPass, sref);

		// fullscreen quad
		mImmediateContext->PSSetShader(mPixelShaderQuad, NULL, 0);
		mImmediateContext->VSSetShader(mVertexShaderQuad, NULL, 0);

		ID3D11ShaderResourceView* srviews[1] = { mReprojectionVisualSRV };
		mImmediateContext->PSSetShaderResources(0, 1, srviews);

		RenderQuad(mImmediateContext, mDevice, XMFLOAT2(0, 0), XMFLOAT2(1, 1));

        D3DPERF_EndEvent();
    }


	// ADDITIONAL stuff for debug purpose:
    // quads   
    {
        mImmediateContext->IASetInputLayout( mLayoutPT );

        D3DPERF_BeginEvent(D3DCOLOR_RGBA(0, 0, 0, 0), L"Quads");

        mImmediateContext->PSSetShader(mPixelShaderQuad, NULL, 0);
        mImmediateContext->VSSetShader(mVertexShaderQuad, NULL, 0);

        ID3D11ShaderResourceView* srviews[1] = { mRTSecondSRV };
        mImmediateContext->PSSetShaderResources(0, 1, srviews);

        RenderQuad(mImmediateContext, mDevice, XMFLOAT2(0.01, 0.01), XMFLOAT2(0.2,0.2));

		// depth map
		srviews[0] = mShadowMapSRV;
		mImmediateContext->PSSetShaderResources(0, 1, srviews);

		RenderQuad(mImmediateContext, mDevice, XMFLOAT2(0.21, 0.01), XMFLOAT2(0.2, 0.2));

        D3DPERF_EndEvent();
    }
    



	ID3D11ShaderResourceView* view[] = { NULL, NULL, NULL };
	mImmediateContext->PSSetShaderResources( 0, 3, view );

    D3DPERF_EndEvent();

	return S_OK;
}


void TestProject::RenderCamera(LPD3DDeviceContext context, LPD3D11Device device, XMMATRIX* invViewProjMatrix)
{
    ID3D11Buffer* mVertexBuffer = NULL;

    //create buffers
    HRESULT hr;

    // Create vertex buffer
    VertexFormatPT vertices[] =
    {
        //front
        { XMFLOAT3( -1.0f,-1.0f, -1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
        { XMFLOAT3( +1.0f,-1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
        { XMFLOAT3( -1.0f,-1.0f, -1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
        { XMFLOAT3( -1.0f,+1.0f, -1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

        { XMFLOAT3( +1.0f,-1.0f, -1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
        { XMFLOAT3( +1.0f,+1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
        { XMFLOAT3( -1.0f,+1.0f, -1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
        { XMFLOAT3( +1.0f,+1.0f, -1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

        //back
        { XMFLOAT3( -1.0f,-1.0f, +1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
        { XMFLOAT3( +1.0f,-1.0f, +1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
        { XMFLOAT3( -1.0f,-1.0f, +1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
        { XMFLOAT3( -1.0f,+1.0f, +1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

        { XMFLOAT3( +1.0f,-1.0f, +1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
        { XMFLOAT3( +1.0f,+1.0f, +1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
        { XMFLOAT3( -1.0f,+1.0f, +1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
        { XMFLOAT3( +1.0f,+1.0f, +1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

        //connection
        { XMFLOAT3( -1.0f,-1.0f, -1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
        { XMFLOAT3( -1.0f,-1.0f, +1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
        { XMFLOAT3( +1.0f,-1.0f, -1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
        { XMFLOAT3( +1.0f,-1.0f, +1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

        { XMFLOAT3( -1.0f,+1.0f, -1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
        { XMFLOAT3( -1.0f,+1.0f, +1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
        { XMFLOAT3( +1.0f,+1.0f, -1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
        { XMFLOAT3( +1.0f,+1.0f, +1.0f ), XMFLOAT2( 0.0f, 1.0f ) },
    };

    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory( &InitData, sizeof(InitData) );

    D3D11_BUFFER_DESC bd;
    ZeroMemory( &bd, sizeof(bd) );
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(vertices);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    InitData.pSysMem = vertices;
    hr = device->CreateBuffer( &bd, &InitData, &mVertexBuffer );
    if( FAILED( hr ) )
        return;


    //render buffer with current camera
    cb.mWorld = XMMatrixTranspose( *invViewProjMatrix );
    context->UpdateSubresource( mCBChangesEveryFrame, 0, NULL, &cb, 0, 0 );

    // Set vertex buffer
    UINT stride = sizeof( VertexFormatPT );
    UINT offset = 0;
    context->IASetVertexBuffers( 0, 1, &mVertexBuffer, &stride, &offset );

    // Set primitive topology
    context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_LINELIST );

    // draw dat shit
    context->Draw( 24, 0 ); 

    SAFE_RELEASE( mVertexBuffer );

    return;
}

void TestProject::RenderPoints(LPD3DDeviceContext context, LPD3D11Device device, XMVECTOR* points, int numOfPoints)
{
    //assemble points
    ID3D11Buffer* mVertexBuffer = NULL;

    //create buffers
    HRESULT hr;

    VertexFormatPT vertices[16];

    for(int i = 0; i < numOfPoints; i++)
    {
        XMVECTOR* pt = &points[i];
        vertices[i].Pos = XMFLOAT3( pt->m128_f32[0], pt->m128_f32[1], pt->m128_f32[2] );
        vertices[i].Tex = XMFLOAT2(0,0);
    }


    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory( &InitData, sizeof(InitData) );

    D3D11_BUFFER_DESC bd;
    ZeroMemory( &bd, sizeof(bd) );
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(vertices);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    InitData.pSysMem = vertices;
    hr = device->CreateBuffer( &bd, &InitData, &mVertexBuffer );
    if( FAILED( hr ) )
        goto render_points_end;

    //render buffer with current camera
    XMMATRIX mat = XMMatrixTranslation(0,0,0);
    cb.mWorld = XMMatrixTranspose( mat );
    context->UpdateSubresource( mCBChangesEveryFrame, 0, NULL, &cb, 0, 0 );

    // Set vertex buffer
    UINT stride = sizeof( VertexFormatPT );
    UINT offset = 0;
    context->IASetVertexBuffers( 0, 1, &mVertexBuffer, &stride, &offset );

    // Set primitive topology
    context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_POINTLIST );

    // draw dat shit
    context->Draw( numOfPoints, 0 ); 


render_points_end:
    SAFE_RELEASE(mVertexBuffer);
    return;
}

void TestProject::RenderQuad(LPD3DDeviceContext context, LPD3D11Device device, XMFLOAT2 offset, XMFLOAT2 relSize)
{
    //assemble points
    ID3D11Buffer* mVertexBuffer = NULL;

    //create buffers
    HRESULT hr;


    VertexFormatPT vertices[] =
    {
        { XMFLOAT3(2.0*offset.x - 1.0, 2.0*(offset.y + relSize.y) - 1.0, 0), XMFLOAT2(0, 0) },
        { XMFLOAT3(2.0*(offset.x + relSize.x) - 1.0, 2.0*(offset.y + relSize.y) - 1.0, 0), XMFLOAT2(1, 0) },
        { XMFLOAT3(2.0*offset.x-1.0, 2.0*offset.y-1.0, 0), XMFLOAT2(0, 1) },      
        { XMFLOAT3(2.0*(offset.x + relSize.x)-1.0, 2.0*offset.y-1.0, 0), XMFLOAT2(1, 1) },

        /*{ XMFLOAT3(offset.x, offset.y, 0), XMFLOAT2(0, 0) },    
        { XMFLOAT3(offset.x + relSize.x, offset.y + relSize.y, 0), XMFLOAT2(1, 1) },*/
    };

    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));

    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(vertices);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    InitData.pSysMem = vertices;
    hr = device->CreateBuffer(&bd, &InitData, &mVertexBuffer);
    if (FAILED(hr))
        goto render_quad_end;

    // Set vertex buffer
    UINT stride = sizeof(VertexFormatPT);
    UINT loffset = 0;
    context->IASetVertexBuffers(0, 1, &mVertexBuffer, &stride, &loffset);

    // Set primitive topology
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // draw dat shit
    context->Draw(4, 0);


render_quad_end:
    SAFE_RELEASE(mVertexBuffer);
    return;
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


    // Define the input layout
    mLayoutPT = VertexFormatMgr::getPTLayout(mDevice);
    mLayoutPNT = VertexFormatMgr::getPNTLayout(mDevice);


	//----------------------------------------------------------------------------
	// Create shaders


	ID3DBlob* pVSBlob = NULL;
    
    FUtil::InitVertexShader(mDevice, "TestProjectShader.fx", "VS", "vs_4_0", &pVSBlob, &mVertexShader);
    FUtil::InitVertexShader(mDevice, "TestProjectShader.fx", "VS_QUAD", "vs_4_0", &pVSBlob, &mVertexShaderQuad);

    ID3DBlob* pPSBlob = NULL;

    FUtil::InitPixelShader(mDevice, "TestProjectShader.fx", "PS", "ps_4_0", &pPSBlob, &mPixelShader);
    FUtil::InitPixelShader(mDevice, "TestProjectShader.fx", "PS_QUAD", "ps_4_0", &pPSBlob, &mPixelShaderQuad);


	//----------------------------------------------------------------------------
	// Create the constant buffers

	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
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


    // init camera
    mainCamera.setProjectionParams(XM_PIDIV4, swapChainDesc.BufferDesc.Width / swapChainDesc.BufferDesc.Height, 1.0, 100.0);
    mainCamera.setOrbitParams( 15, XMFLOAT3(0,0,0) );

   
    // add rasterizing states
    D3D11_RASTERIZER_DESC desc;
    ZeroMemory(&desc, sizeof(D3D11_RASTERIZER_DESC));
    desc.CullMode = D3D11_CULL_NONE;
    desc.FillMode = D3D11_FILL_SOLID;
    hr = mDevice->CreateRasterizerState(&desc, &mRSCullNone);
    if(FAILED(hr))
    {
        MessageBox(0, "Error: cannot create rasterizer state", "Error.", 0);
    }

    desc.CullMode = D3D11_CULL_BACK;
    hr = mDevice->CreateRasterizerState(&desc, &mRSOrdinary);
    if(FAILED(hr))
    {
        MessageBox(0, "Error: cannot create rasterizer state", "Error.", 0);
    }

    desc.FillMode = D3D11_FILL_WIREFRAME;
    desc.CullMode = D3D11_CULL_NONE;
    hr = mDevice->CreateRasterizerState(&desc, &mRSWireframe);
    if(FAILED(hr))
    {
        MessageBox(0, "Error: cannot create rasterizer state", "Error.", 0);
    }


	// add Depth-stencil states
	D3D11_DEPTH_STENCIL_DESC dsDesc;
	ZeroMemory(&dsDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	dsDesc.DepthEnable = true;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;

	hr = mDevice->CreateDepthStencilState(&dsDesc, &mDSOrdinary);
	if (FAILED(hr))
	{
		MessageBox(0, "Error: cannot create rasterizer state", "Error.", 0);
	}

	dsDesc.DepthEnable = false;
	hr = mDevice->CreateDepthStencilState(&dsDesc, &mDSFullscreenPass);
	if (FAILED(hr))
	{
		MessageBox(0, "Error: cannot create rasterizer state", "Error.", 0);
	}


    //---------------------------------------
    // compute shader initialization

    ID3DBlob* pCSBlob = NULL;
    hr = FUtil::CompileShaderFromFile("Reprojection.hlsl", "CSMain", "cs_4_0", &pCSBlob);
    if (FAILED(hr))
    {
        FUtil::Log("Error: cannot compile compute shader for reprojection");
        return hr;
    }
    V_RETURN(mDevice->CreateComputeShader(pCSBlob->GetBufferPointer(), pCSBlob->GetBufferSize(), NULL, &mCS));
    


    //create constant buffer
	/*ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.ByteWidth = sizeof(CBForCS);
    hr = mDevice->CreateBuffer(&bd, NULL, &mCBforCS);
    if (FAILED(hr))
        return hr;


    // create buffer
    ZeroMemory(&bd, sizeof(bd));
    bd.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    bd.ByteWidth = GRID_DIMENSION * GRID_DIMENSION * sizeof(float)*4;
    bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bd.StructureByteStride = sizeof(float)*4;
    bd.Usage = D3D11_USAGE_DEFAULT;

    float* arr = new float[4*GRID_DIMENSION*GRID_DIMENSION];
    memset(arr, 0, sizeof(arr));
    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem = arr;
    
    if (FAILED(mDevice->CreateBuffer(&bd, &InitData, &mGridBuffer)))
    {
        FUtil::Log("Error: cannot create buffer for projected grid");
        return hr;
    }
    delete [] arr;

    // create views
    D3D11_SHADER_RESOURCE_VIEW_DESC DescRV;
    ZeroMemory(&DescRV, sizeof(DescRV));
    DescRV.Format = DXGI_FORMAT_UNKNOWN;
    DescRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    DescRV.Buffer.FirstElement = 0;
    DescRV.Buffer.NumElements = bd.ByteWidth / bd.StructureByteStride;
    mDevice->CreateShaderResourceView(mGridBuffer, &DescRV, &mGridBufferSRV);

    D3D11_UNORDERED_ACCESS_VIEW_DESC DescUAV;
    ZeroMemory(&DescUAV, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
    DescUAV.Format = DXGI_FORMAT_UNKNOWN;
    DescUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    DescUAV.Buffer.FirstElement = 0;
    DescUAV.Buffer.NumElements = bd.ByteWidth / bd.StructureByteStride;
    V_RETURN(mDevice->CreateUnorderedAccessView(mGridBuffer, &DescUAV, &mGridBufferUAV));*/



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
	V_RETURN( mDevice->CreateShaderResourceView( mRTSecondTex, &DescRV, &mRTSecondSRV ) );


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
	hr = mDevice->CreateShaderResourceView( mDSSecondTex, &shaderResourceViewDesc, &mDSSecondSRV);
	V_RETURN(hr);



	//-------------------------------------------------
	// create depth-stencil for shadow map
	{
		D3D11_TEXTURE2D_DESC smDesc;
		ZeroMemory(&smDesc, sizeof(D3D11_TEXTURE2D_DESC));
		smDesc.Height = 1024;
		smDesc.Width = 1024;
		smDesc.MipLevels = 1;
		smDesc.ArraySize = 1;
		smDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		smDesc.SampleDesc.Count = 1;
		smDesc.SampleDesc.Quality = 0;
		smDesc.Usage = D3D11_USAGE_DEFAULT;
		smDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		smDesc.CPUAccessFlags = 0;
		smDesc.MiscFlags = 0;
		hr = mDevice->CreateTexture2D(&smDesc, NULL, &mShadowMapTexture);
		V_RETURN(hr);

		//reuse
		hr = mDevice->CreateDepthStencilView(mShadowMapTexture, &descDSV, &mShadowMapDSV);
		V_RETURN(hr);

		hr = mDevice->CreateShaderResourceView(mShadowMapTexture, &shaderResourceViewDesc, &mShadowMapSRV);
		V_RETURN(hr);
	}


	// create reprojection texture and its views
	{
		// prepare for visualization - debug purpose only
		{
			D3D11_BUFFER_DESC tempBDesc;
			ZeroMemory(&tempBDesc, sizeof(D3D11_BUFFER_DESC));
			tempBDesc.BindFlags = 0;
			tempBDesc.Usage = D3D11_USAGE_STAGING;
			tempBDesc.ByteWidth = 1024 * 1024 * sizeof(float);
			tempBDesc.StructureByteStride = sizeof(float);
			tempBDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

			float* arr = new float[1024 * 1024];
			memset(arr, 0, sizeof(arr));
			D3D11_SUBRESOURCE_DATA InitData;
			InitData.pSysMem = arr;
			hr = mDevice->CreateBuffer(&tempBDesc, &InitData, &mReprojectionTransferBuffer);
			delete[] arr;
			V_RETURN(hr);

			D3D11_TEXTURE2D_DESC tempTDesc;
			ZeroMemory(&tempTDesc, sizeof(D3D11_TEXTURE2D_DESC));
			tempTDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			tempTDesc.Usage = D3D11_USAGE_DYNAMIC;
			tempTDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			tempTDesc.ArraySize = 1;
			tempTDesc.Format = DXGI_FORMAT_R32_FLOAT;
			tempTDesc.Height = 1024;
			tempTDesc.Width = 1024;
			tempTDesc.MipLevels = 1;
			tempTDesc.MiscFlags = 0;
			tempTDesc.SampleDesc.Count = 1;
			tempTDesc.SampleDesc.Quality = 0;
			hr = mDevice->CreateTexture2D(&tempTDesc, NULL, &mReprojectionVisualTexture);
			V_RETURN(hr);

			D3D11_SHADER_RESOURCE_VIEW_DESC tempSRVDesc;
			tempSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			tempSRVDesc.Texture2D.MostDetailedMip = 0;
			tempSRVDesc.Texture2D.MipLevels = 1;
			tempSRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
			hr = mDevice->CreateShaderResourceView(mReprojectionVisualTexture, &tempSRVDesc, &mReprojectionVisualSRV);
			V_RETURN(hr);
		}

		// constant buffer
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;
		bd.ByteWidth = sizeof(CBforCS);
		hr = mDevice->CreateBuffer(&bd, NULL, &mCBforCS);
		if (FAILED(hr))
			return hr;


		D3D11_BUFFER_DESC sbDesc;
		ZeroMemory(&sbDesc, sizeof(D3D11_BUFFER_DESC));
		sbDesc.Usage = D3D11_USAGE_DEFAULT;
		sbDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
		sbDesc.ByteWidth = 1024 * 1024 * sizeof(float);
		sbDesc.StructureByteStride = sizeof(float);
		sbDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

		float* arr = new float[1024*1024];
		memset(arr, 0, sizeof(arr));
		D3D11_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = arr;
		hr = mDevice->CreateBuffer(&sbDesc, &InitData, &mReprojectionBuffer);
		delete[] arr;

		V_RETURN(hr);

		D3D11_SHADER_RESOURCE_VIEW_DESC sbSRVDesc;
		ZeroMemory(&sbSRVDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
		sbSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
		sbSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		sbSRVDesc.Buffer.ElementWidth = sizeof(float);
		sbSRVDesc.Buffer.ElementOffset = 0;
		sbSRVDesc.Buffer.FirstElement = 0;
		sbSRVDesc.Buffer.NumElements = 1024 * 1024;
		hr = mDevice->CreateShaderResourceView(mReprojectionBuffer, &sbSRVDesc, &mReprojectionSRV);
		V_RETURN(hr);

		D3D11_UNORDERED_ACCESS_VIEW_DESC sbUAVDesc;
		ZeroMemory(&sbUAVDesc, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
		sbUAVDesc.Format = DXGI_FORMAT_UNKNOWN;
		sbUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		sbUAVDesc.Buffer.FirstElement = 0;
		sbUAVDesc.Buffer.NumElements = 1024 * 1024;
		hr = mDevice->CreateUnorderedAccessView(mReprojectionBuffer, &sbUAVDesc, &mReprojectionUAV);
		V_RETURN(hr);
	}

}




HRESULT TestProject::ReleaseScene()
{
	cube.Release();
	plane.Release();

	SAFE_RELEASE(mVertexShader);
	SAFE_RELEASE(mPixelShader);

    SAFE_RELEASE(mVertexShaderQuad);
    SAFE_RELEASE(mPixelShaderQuad);

	SAFE_RELEASE(mCBChangesEveryFrame);
	SAFE_RELEASE(mTextureRV);
	SAFE_RELEASE(mSamplerLinear);

    SAFE_RELEASE(mRSCullNone);
    SAFE_RELEASE(mRSOrdinary);

	SAFE_RELEASE(mDSOrdinary);
	SAFE_RELEASE(mDSFullscreenPass);


	SAFE_RELEASE(mShadowMapDSV);
	SAFE_RELEASE(mShadowMapSRV);
	SAFE_RELEASE(mShadowMapTexture);


    // compute shaders and resources

	SAFE_RELEASE(mCS);
    SAFE_RELEASE(mCBforCS);

	SAFE_RELEASE(mReprojectionSRV);
	SAFE_RELEASE(mReprojectionUAV);
	SAFE_RELEASE(mReprojectionBuffer);


	return S_OK;
}

//------------------------------------------------------------------------------








#endif //PROJ_TESTPROJECT
