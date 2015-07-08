/*
====================================================================

====================================================================
*/


#include "TestProject.h"

#include "camera.h"
#include "FPCube.h"
#include "FPPlane.h"


DXApp* initApplication()
{
    return new TestProject();
}



TestProject::TestProject():
	mVMeshColor( 0.7f, 0.7f, 0.7f, 1.0f )/*,
    plane(1024),
    cube(64)*/
{
}

TestProject::~TestProject()
{
}



HRESULT TestProject::FrameMove()
{
    // keyboard
    //----------------------------------

    /*if(isKeyPressed(DIK_ESCAPE))
        exit(1);*/
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
	else
        mDirLight.FrameMove(XMFLOAT3(0,0,0), XMFLOAT3(0, 0, 0));



    // update some buffers
    //---------------

    // Modify the color based on time
    mVMeshColor.x = ( sinf( totalTime * 0.001f ) + 1.0f ) * 0.5f;
    mVMeshColor.y = ( cosf( totalTime * 0.003f ) + 1.0f ) * 0.5f;
    mVMeshColor.z = ( sinf( totalTime * 0.005f ) + 1.0f ) * 0.5f;
    cb.vMeshColor = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);


    //-----------------------------------------------
    // update objects


    return S_OK;
}


/*
===================================================================
===================================================================
*/

void TestProject::_renderSceneObjects(bool bPlane, bool bCubes)
{
    if (bPlane)
    {
        //plane
        plane.position = XMFLOAT3(0, -0.5, 0);
        plane.scale = XMFLOAT3(100, 1, 100);
        FUtil::RenderPrimitive(&plane, mImmediateContext, cb, mCBChangesEveryFrame);
    }

    if (bCubes)
    {
        // invoke manual tesselation
        /*static bool bTes = false;
        if (!bTes)
        {
            bTes = true;
            cube.tesselate(0.05, mDevice, mImmediateContext);
        }*/

        //cubes
        cube.position = XMFLOAT3(0, 0, 0);
        FUtil::RenderPrimitive(&cube, mImmediateContext, cb, mCBChangesEveryFrame);

        cube.position = XMFLOAT3(3, 0, 0);
        FUtil::RenderPrimitive(&cube, mImmediateContext, cb, mCBChangesEveryFrame);

        cube.position = XMFLOAT3(-3, 0, 0);
        FUtil::RenderPrimitive(&cube, mImmediateContext, cb, mCBChangesEveryFrame);
    }
}


HRESULT TestProject::RenderScene()
{
	//setup common stuff
	ID3D11SamplerState* aSamplers[] = { mSamplerLinear, mBackbufferSampler, mDepthSampler };
	mImmediateContext->PSSetSamplers(0, 3, aSamplers);
	mImmediateContext->VSSetSamplers(0, 3, aSamplers);


    // Render scene to 3 textures: color, normal, depth (auto)
    _renderSceneToGBuffer();


    // deferred post-effect: applying lighting to scene
    _renderDeferredShading();


    //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
	// ADDITIONAL stuff for debug purpose:
    {
        mImmediateContext->IASetInputLayout( mLayoutPT );

        D3DPERF_BeginEvent(D3DCOLOR_RGBA(0, 255, 0, 1), L"Quads");

        mImmediateContext->PSSetShader(mPixelShaderQuad, NULL, 0);
        mImmediateContext->VSSetShader(mVertexShaderQuad, NULL, 0);


        // FULLSCREEN QUAD
        ID3D11ShaderResourceView* srviews[1] = { mSecondRT->getTextureSRV(0) };
        mImmediateContext->PSSetShaderResources(0, 1, srviews);

        _renderQuad(mImmediateContext, mDevice, XMFLOAT2(0.00, 0.00), XMFLOAT2(1, 1));


        D3DPERF_EndEvent();
    }
    
    // release some stuff
	ID3D11ShaderResourceView* view[] = { NULL, NULL, NULL };
	mImmediateContext->PSSetShaderResources( 0, 3, view );

	return S_OK;
}


void TestProject::_renderSceneToGBuffer()
{
    //--------------------------------------------------------------------
    // TO SECOND RT

    {
        DXCamera* curCamera = &mainCamera; //select camera
        cb.mView = XMMatrixTranspose(curCamera->getViewMatrix());
        XMMATRIX mProjection = curCamera->getProjMatrix();
        cb.mProjection = XMMatrixTranspose(mProjection);

        mDirLight.GetMVPMatrix(&cb.matMVPLight);
    }


    D3DPERF_BeginEvent(D3DCOLOR_RGBA(255, 0, 255, 0), L"RT render");

    UINT sref = 0;

    mImmediateContext->OMSetDepthStencilState(mDSOrdinary, sref);
    mImmediateContext->RSSetState(mRSOrdinary);

    mSecondRT->activate();
    mSecondRT->clear(XMFLOAT4{ 0.525f, 0.525f, 0.525f, 1.0f });

    // initial preparation
    mImmediateContext->IASetInputLayout(mLayoutPT);
    mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    mImmediateContext->VSSetShader(mVertexShader, NULL, 0);
    mImmediateContext->PSSetShader(mPixelShader, NULL, 0);

    mImmediateContext->VSSetConstantBuffers(0, 1, &mCBChangesEveryFrame);
    mImmediateContext->PSSetConstantBuffers(0, 1, &mCBChangesEveryFrame);

    ID3D11ShaderResourceView* rv[] = { mTextureRV };
    mImmediateContext->PSSetShaderResources(0, 1, rv);
    mImmediateContext->VSSetShaderResources(0, 1, rv);


    _renderSceneObjects();


    // unbind various stuff - shaders, constants
    rv[0] = NULL;
    mImmediateContext->PSSetShaderResources(0, 1, rv);
    mImmediateContext->VSSetShaderResources(0, 1, rv);

    D3DPERF_EndEvent();
}



void TestProject::_renderDeferredShading()
{
    UINT sref = 0;
    float ClearColor[4] = { 0.525f, 0.525f, 0.525f, 1.0f }; // red, green, blue, alpha

    mImmediateContext->RSSetState(mRSOrdinary);

    D3DPERF_BeginEvent(D3DCOLOR_RGBA(255, 255, 222, 0), L"Deferred final pass");

    mImmediateContext->OMSetRenderTargets(1, &mRenderTargetView, mDepthStencilView);
    mImmediateContext->ClearRenderTargetView(mRenderTargetView, ClearColor);
    mImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
    UpdateViewport(swapChainDesc);

    if (false)
    {
        //build buffers first
        {
            XMVECTOR det;
            XMMATRIX vmat = XMMatrixInverse(&det,
                XMMatrixTranspose(mainCamera.getProjMatrix()) *
                XMMatrixTranspose(mainCamera.getViewMatrix()));

            mMemBufferForDeferredPass.matMVPInv = vmat;

            mDirLight.GetMVPMatrix(&mMemBufferForDeferredPass.matMVPLight);

            mImmediateContext->UpdateSubresource(mCBforDeferredPass, 0, NULL, &mMemBufferForDeferredPass, 0, 0);
        }

        D3DPERF_BeginEvent(D3DCOLOR_RGBA(0, 255, 0, 0), L"Deferred");

        mImmediateContext->OMSetDepthStencilState(mDSFullscreenPass, sref);

        // fullscreen quad
        mImmediateContext->PSSetShader(mPixelShaderDeferred, NULL, 0);
        mImmediateContext->VSSetShader(mVertexShaderQuad, NULL, 0);

        ID3D11ShaderResourceView* srviews[2] = 
        { 
            mSecondRT->getTextureSRV(0), 
            mSecondRT->getDepthStencilSRV() 
        };
        mImmediateContext->PSSetShaderResources(0, 2, srviews);

        ID3D11Buffer* csbuffers[1] = { mCBforDeferredPass };
        mImmediateContext->PSSetConstantBuffers(0, 1, csbuffers);

        _renderQuad(mImmediateContext, mDevice, XMFLOAT2(0, 0), XMFLOAT2(1, 1));

        srviews[0] = NULL;
        srviews[1] = NULL;
        srviews[2] = NULL;
        srviews[3] = NULL;
        mImmediateContext->PSSetShaderResources(0, 4, srviews);


        D3DPERF_EndEvent();
    }

    D3DPERF_EndEvent();
}





//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


void TestProject::_renderCamera(LPD3DDeviceContext context, LPD3D11Device device, XMMATRIX* invViewProjMatrix)
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

void TestProject::_renderPoints(LPD3DDeviceContext context, LPD3D11Device device, XMVECTOR* points, int numOfPoints)
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

void TestProject::_renderQuad(LPD3DDeviceContext context, LPD3D11Device device, XMFLOAT2 offset, XMFLOAT2 relSize)
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

    //create render target
    mSecondRT = new RenderTarget(
        "second", 
        XMFLOAT2(swapChainDesc.BufferDesc.Width, swapChainDesc.BufferDesc.Width)
        );
    mSecondRT->appendTexture(0, DXGI_FORMAT_R32G32B32A32_FLOAT);
    mSecondRT->appendDepthStencil(DXGI_FORMAT_R32_TYPELESS);


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
    FUtil::InitPixelShader(mDevice, "Deferred.fx", "PS", "ps_4_0", &pPSBlob, &mPixelShaderDeferred);



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

    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	V_RETURN( mDevice->CreateSamplerState( &sampDesc, &mDepthSampler ) );


    // init camera
    mainCamera.setProjectionParams(XM_PIDIV4,
        swapChainDesc.BufferDesc.Width / swapChainDesc.BufferDesc.Height, 0.01, 300.0);
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
    desc.CullMode = D3D11_CULL_BACK;
    hr = mDevice->CreateRasterizerState(&desc, &mRSWireframe);
    if(FAILED(hr))
    {
        MessageBox(0, "Error: cannot create rasterizer state", "Error.", 0);
    }

    desc.CullMode = D3D11_CULL_BACK;
    desc.FillMode = D3D11_FILL_SOLID;
    desc.DepthBiasClamp = 0;
    desc.DepthBias = 1000;
    desc.SlopeScaledDepthBias = 1.5f;
    hr = mDevice->CreateRasterizerState(&desc, &mRSShadowMap);
    V_RETURN(hr);


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


	//--------------------------------------------------------------
	// B U F F E R S

	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = DXGI_FORMAT_D32_FLOAT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;

	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;
	shaderResourceViewDesc.Format = DXGI_FORMAT_R32_FLOAT;

	//TODO: create some buffers here

	return S_OK;
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

    SAFE_DELETE(mSecondRT);

	return S_OK;
}

//------------------------------------------------------------------------------





