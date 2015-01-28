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

	mLayoutPT(NULL),
    mLayoutPNT(NULL),

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
	mDSSecondDSV(NULL),

    mRSCullNone(NULL),
    mRSOrdinary(NULL)
{
    mVertexShader = NULL;
    mPixelShader = NULL;

    mPixelShaderQuad = NULL;
    mVertexShaderQuad = NULL;

    mVertexShaderReflection = NULL;
    mPixelShaderReflection = NULL;

    mVertexShaderWater = NULL;
    mPixelShaderWater = NULL;


    mCBChangesEveryFrame = NULL;
    mCBforCS = NULL;


    //----------------------------------------------------
    // projected grid

    surface = FSurface();

    // compute shader for projected grid
    mCS = NULL;
    mGridBuffer = NULL;
    mGridBufferSRV = NULL;
    mGridBufferUAV = NULL;

    // init ocean params
    oceanDesc = OceanDescription{
        1.0f, //size
        0.06f, //speed
        XMFLOAT3(35, 42, 57), //amplitude
        XMFLOAT3(1.4f, 1.6f, 2.2f), //gradient
        XMFLOAT3(1.12f, 0.59f, 0.23f), //octave

        512, //dmap_dim
        2000.0f, //patch_length
        0.8f, //time_scale
        0.35f, //wave_amplitude
        XMFLOAT2(0.8f, 0.6f), //wind_dir
        600.0f, //wind_speed
        0.07f, //wind_dependency
        1.3f //choppy_scale
    };

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
    if(isKeyPressed(DIK_TAB))
        bViewCameraMain = !bViewCameraMain;
    if(isKeyPressed(DIK_C))
        bControlCameraMain = !bControlCameraMain;

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


    // update cameras
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

    if(bControlCameraMain)
    {
        mainCamera.FrameMove(offset, XMFLOAT3(mouseDX, mouseDY, mouseDZ));
        observeCamera.FrameMove(XMFLOAT3(0,0,0), XMFLOAT3(0,0,0));
    }
    else
    {
        mainCamera.FrameMove(XMFLOAT3(0,0,0), XMFLOAT3(0,0,0));
        observeCamera.FrameMove(offset, XMFLOAT3(mouseDX, mouseDY, mouseDZ));
    }


    // update some buffers
    //---------------

    DXCamera* curCamera = bViewCameraMain ? &mainCamera : &observeCamera;

    cb.mView = XMMatrixTranspose( curCamera->getViewMatrix() );

    cb.SSRParams = XMFLOAT4(
        32.0f,
        0.0025f,
        42.0 / swapChainDesc.BufferDesc.Height,
        0
        );

    cb.mScreenParams = XMFLOAT4(
        swapChainDesc.BufferDesc.Width, swapChainDesc.BufferDesc.Height,
        curCamera->getNearPlane(), curCamera->getFarPlane() );

    XMMATRIX mProjection = curCamera->getProjMatrix();
    cb.mProjection = XMMatrixTranspose( mProjection );

    cb.mScreenParams =
        XMFLOAT4(
        swapChainDesc.BufferDesc.Width,
        swapChainDesc.BufferDesc.Height,
        curCamera->getNearPlane(),
        curCamera->getFarPlane()
        );

    cb.mPerspectiveValues =
        XMFLOAT4(
        1.0f / mProjection.m[0][0],
        1.0f / mProjection.m[1][1],
        mProjection.m[3][2],
        -mProjection.m[2][2]
    );

    // Modify the color based on time
    mVMeshColor.x = ( sinf( totalTime * 0.001f ) + 1.0f ) * 0.5f;
    mVMeshColor.y = ( cosf( totalTime * 0.003f ) + 1.0f ) * 0.5f;
    mVMeshColor.z = ( sinf( totalTime * 0.005f ) + 1.0f ) * 0.5f;
    cb.vMeshColor = mVMeshColor;

    auto eye = curCamera->getEye();
    cb.eyeVector = XMFLOAT4(eye.m128_f32[0], eye.m128_f32[1], eye.m128_f32[2], eye.m128_f32[3]);
    cb.sunDirection = XMFLOAT4(0, 0, 0, 0);
    cb.waterColor = XMFLOAT4(0, 0, 0, 0);
    cb.skyColor = XMFLOAT4(0, 0, 0, 0);


    //-----------------------------------------------
    // update objects

    D3DPERF_BeginEvent(D3DCOLOR_RGBA(255, 0, 0, 0), L"Update surface");
    surface.Update(totalTime, deltaTime);
    D3DPERF_EndEvent();

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
	mImmediateContext->PSSetSamplers( 0, 3, aSamplers );
    mImmediateContext->VSSetSamplers( 0, 3, aSamplers );
    mImmediateContext->CSSetSamplers( 0, 3, aSamplers );


    //--------------------------------------------------------------------
    // TO TEXTURE

    D3DPERF_BeginEvent(D3DCOLOR_RGBA(255, 255, 255, 0), L"RT render");

	// set RT and clear it
	mImmediateContext->OMSetRenderTargets( 1, &mRTSecondRTV, mDSSecondDSV);
	float ClearColor[4] = { 0.525f, 0.525f, 0.525f, 1.0f }; // red, green, blue, alpha
	mImmediateContext->ClearRenderTargetView( mRTSecondRTV, ClearColor );
	mImmediateContext->ClearDepthStencilView( mDSSecondDSV, D3D11_CLEAR_DEPTH, 1.0f, 0 );



	// initial preparation
	mImmediateContext->IASetInputLayout( mLayoutPT );

	mImmediateContext->VSSetShader( mVertexShader, NULL, 0 );
	mImmediateContext->PSSetShader( mPixelShader, NULL, 0 );

	mImmediateContext->VSSetConstantBuffers( 0, 1, &mCBChangesEveryFrame );
	mImmediateContext->PSSetConstantBuffers( 0, 1, &mCBChangesEveryFrame );

	mImmediateContext->PSSetShaderResources( 0, 1, &mTextureRV );
	mImmediateContext->PSSetShaderResources( 1, 1, &mTextureRV );
	mImmediateContext->PSSetShaderResources( 2, 1, &mTextureRV );
	

	// render cubes
    // it's the same cube actually
	if(true)
    {
        D3DPERF_BeginEvent(D3DCOLOR_RGBA(0, 255, 0, 0), L"Cubes");

        cube.position = XMFLOAT3(0, 0, 0);
        FUtil::RenderPrimitive( &cube, mImmediateContext, cb, mCBChangesEveryFrame );

		cube.position = XMFLOAT3(3, 0, 0);
		FUtil::RenderPrimitive( &cube, mImmediateContext, cb, mCBChangesEveryFrame );

        cube.position = XMFLOAT3(-3, 0, 0);
        FUtil::RenderPrimitive( &cube, mImmediateContext, cb, mCBChangesEveryFrame );

        D3DPERF_EndEvent();
	}



    D3DPERF_EndEvent();


	//--------------------------------------------------------------------
	// TO BACKBUFFER

    D3DPERF_BeginEvent(D3DCOLOR_RGBA(255, 255, 222, 0), L"Backbuffer render");

	mImmediateContext->OMSetRenderTargets( 1, &mRenderTargetView, mDepthStencilView);

	// render cubes
	if(true){

        D3DPERF_BeginEvent(D3DCOLOR_RGBA(0, 255, 0, 0), L"Cubes");

        mImmediateContext->RSSetState( mRSOrdinary );

        cube.rotationEuler = XMFLOAT3( 0, totalTime * 0.0001f, 0 );
        cube.position = XMFLOAT3(0, 0, 0);
        FUtil::RenderPrimitive( &cube, mImmediateContext, cb, mCBChangesEveryFrame );

        cube.position = XMFLOAT3(3, 0, 0);
        FUtil::RenderPrimitive( &cube, mImmediateContext, cb, mCBChangesEveryFrame );

        cube.position = XMFLOAT3(-3, 0, 0);
        FUtil::RenderPrimitive( &cube, mImmediateContext, cb, mCBChangesEveryFrame );

        D3DPERF_EndEvent();
    }


    // render cameras frustums
    if(!bViewCameraMain)
    {
        XMVECTOR det;
        XMMATRIX invViewProj = mainCamera.getViewMatrix() * mainCamera.getProjMatrix();
        invViewProj = XMMatrixInverse( &det, invViewProj );
        cb.vMeshColor = XMFLOAT4(1, 0, 0, 1);
        RenderCamera(mImmediateContext, mDevice, &invViewProj);

        cb.vMeshColor = XMFLOAT4(0, 1, 0 ,1);
        RenderCamera(mImmediateContext, mDevice, &surface.projectorWorldViewInverted);
    }

    // render debug points
    if(false)
    {
        cb.vMeshColor = XMFLOAT4(1, 1, 1, 1);

        XMVECTOR* vectors = NULL;
        vectors = (XMVECTOR*)_mm_malloc(surface.numOfPositions*sizeof(XMVECTOR), 16);
        
        for(int i = 0; i < surface.numOfPositions; i++)
            vectors[i] = XMVectorSet(surface.positions[i].x, surface.positions[i].y, surface.positions[i].z, 1);

        RenderPoints(mImmediateContext, mDevice, vectors, surface.numOfPositions);
        
        _mm_free(vectors);
        vectors = NULL;
    }

    // projected grid (along with water shader)
    D3DPERF_BeginEvent(D3DCOLOR_RGBA(0, 255, 0, 0), L"Water grid");
    CBForCS cbSurface;
    bool bGrid = surface.fillConstantBuffer(cbSurface, totalTime);
    if(bGrid)
    {
        //put additional info into constant buffer
        cbSurface.eyePosition = FUtil::FromVector3(mainCamera.getEye());


        ID3D11ShaderResourceView* aRViews[5];
        
        //--------------------------------
        // do GPU computing 

        mImmediateContext->UpdateSubresource(mCBforCS, 0, NULL, &cbSurface, 0, 0);
        mImmediateContext->CSSetShader(mCS, NULL, 0);

        // bind resources
        ID3D11UnorderedAccessView* aUAViews[1] = { mGridBufferUAV };
        mImmediateContext->CSSetUnorderedAccessViews(0, 1, aUAViews, (UINT*)(&aUAViews));
        mImmediateContext->CSSetConstantBuffers(0, 1, &mCBforCS);

        aRViews[0] = surface.mOceanSimulator->getD3D11DisplacementMap();
        aRViews[1] = surface.pPerlinSRV;
        mImmediateContext->CSSetShaderResources(0, 2, aRViews);

        // launch!
        int tG = ceil(GRID_DIMENSION/16.f);
        mImmediateContext->Dispatch(tG, tG, 1); //numthreads(16,16,1)

        // unbind
        aUAViews[0] = NULL;
        mImmediateContext->CSSetUnorderedAccessViews(0, 1, aUAViews, (UINT*)(&aUAViews));

        aRViews[0] = NULL;
        aRViews[1] = NULL;
        mImmediateContext->CSSetShaderResources(0, 2, aRViews);


        //----------------------------------
        // do rendering
        
        // bind computed buffer
        aRViews[0] = mGridBufferSRV;
        aRViews[1] = surface.mOceanSimulator->getD3D11DisplacementMap();
        aRViews[2] = surface.mOceanSimulator->getD3D11GradientMap();
        aRViews[3] = surface.pFresnelSRV;
        aRViews[4] = surface.pPerlinSRV;
        mImmediateContext->VSSetShaderResources(3, 5, aRViews);
        mImmediateContext->PSSetShaderResources(3, 5, aRViews);

        mImmediateContext->RSSetState( mRSOrdinary );
        mImmediateContext->IASetInputLayout( NULL );

        mImmediateContext->VSSetShader(mVertexShaderWater, NULL, 0);
        mImmediateContext->PSSetShader(mPixelShaderWater, NULL, 0);
            
        cb.vMeshColor = XMFLOAT4(0, 1, 0, 1);
        surface.position = XMFLOAT3(0, 0, 0);

        //add perlin vars
        // set perlin params
        cb.PerlinAmplitude = g_PerlinAmplitude;
        cb.PerlinGradient = g_PerlinGradient;
        cb.PerlinOctave = g_PerlinOctave;
        cb.PerlinSize = g_PerlinSize;

        float mul = (float)totalTime * 0.001 * g_PerlinSpeed;
        XMFLOAT2 perlin_move = XMFLOAT2(mul*windDir.x, mul*windDir.y);
        buffrer.PerlinMovement = perlin_move;

        FUtil::RenderPrimitive(&surface, mImmediateContext, cb, mCBChangesEveryFrame);

        ID3D11ShaderResourceView* aRViewsNULL[3] = { NULL, NULL, NULL };
        mImmediateContext->VSSetShaderResources(3, 3, aRViewsNULL);
        mImmediateContext->PSSetShaderResources(3, 3, aRViewsNULL);

        mImmediateContext->RSSetState(mRSOrdinary);
    }
    D3DPERF_EndEvent();


    //quads   
    {
        mImmediateContext->IASetInputLayout( mLayoutPT );

        D3DPERF_BeginEvent(D3DCOLOR_RGBA(0, 0, 0, 0), L"Quads");

        mImmediateContext->PSSetShader(mPixelShaderQuad, NULL, 0);
        mImmediateContext->VSSetShader(mVertexShaderQuad, NULL, 0);

        ID3D11ShaderResourceView* srviews[1] = { surface.mOceanSimulator->getD3D11DisplacementMap() };
        mImmediateContext->PSSetShaderResources(0, 1, srviews);

        RenderQuad(mImmediateContext, mDevice, XMFLOAT2(0.01, 0.01), XMFLOAT2(0.2,0.2));

        srviews[0] = surface.mOceanSimulator->getD3D11GradientMap();
        mImmediateContext->PSSetShaderResources(0, 1, srviews);

        RenderQuad(mImmediateContext, mDevice, XMFLOAT2(0.22, 0.01), XMFLOAT2(0.2, 0.2));

        srviews[0] = NULL;
        mImmediateContext->PSSetShaderResources(0, 1, srviews);

        D3DPERF_EndEvent();
    }
    

    // SSR plane
    if(true)
    {
        D3DPERF_BeginEvent(D3DCOLOR_RGBA(0, 255, 255, 0), L"SSR");

        mImmediateContext->IASetInputLayout( mLayoutPT );

        // enable reflection shaders
	    mImmediateContext->VSSetShader( mVertexShaderReflection, NULL, 0 );
	    mImmediateContext->PSSetShader( mPixelShaderReflection, NULL, 0 );
	    mImmediateContext->PSSetShaderResources( 1, 1, &mRTSecondRV );
	    mImmediateContext->PSSetShaderResources( 2, 1, &mDSSecondRV );
	
        // render plane
        plane.position = XMFLOAT3(0, -0.3, 0);
        plane.scale = XMFLOAT3(10, 1, 10);
        //FUtil::RenderPrimitive( &plane, mImmediateContext, cb, mCBChangesEveryFrame );

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

    surface.Init(mDevice, &oceanDesc);
    surface.setCamera(&mainCamera);

    // Define the input layout
    mLayoutPT = VertexFormatMgr::getPTLayout(mDevice);
    mLayoutPNT = VertexFormatMgr::getPNTLayout(mDevice);


	//----------------------------------------------------------------------------
	// Create shaders


	ID3DBlob* pVSBlob = NULL;
    
    FUtil::InitVertexShader(mDevice, "TestProjectShader.fx", "VS", "vs_4_0", &pVSBlob, &mVertexShader);
    FUtil::InitVertexShader(mDevice, "TestProjectShader.fx", "VS_QUAD", "vs_4_0", &pVSBlob, &mVertexShaderQuad);
    FUtil::InitVertexShader( mDevice, "TestProjectShader.fx", "VS_Reflection", "vs_4_0", &pVSBlob, &mVertexShaderReflection );
    FUtil::InitVertexShader( mDevice, "TestProjectShader.fx", "VS_WAT", "vs_4_0", &pVSBlob, &mVertexShaderWater );


    ID3DBlob* pPSBlob = NULL;

    FUtil::InitPixelShader(mDevice, "TestProjectShader.fx", "PS", "ps_4_0", &pPSBlob, &mPixelShader);
    FUtil::InitPixelShader(mDevice, "TestProjectShader.fx", "PS_QUAD", "ps_4_0", &pPSBlob, &mPixelShaderQuad);
    FUtil::InitPixelShader( mDevice, "TestProjectShader.fx", "PS_Reflection", "ps_4_0", &pPSBlob, &mPixelShaderReflection);
    FUtil::InitPixelShader( mDevice, "TestProjectShader.fx", "PS_WAT", "ps_4_0", &pPSBlob, &mPixelShaderWater);


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

    observeCamera.setProjectionParams(XM_PIDIV4, swapChainDesc.BufferDesc.Width / swapChainDesc.BufferDesc.Height, 0.01, 1000.0);
    observeCamera.setOrbitParams( 25, XMFLOAT3(0,0,0) );


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



    //---------------------------------------
    // compute shader initialization

    ID3DBlob* pCSBlob = NULL;
    hr = FUtil::CompileShaderFromFile("ProjectedGrid.hlsl", "CSMain", "cs_4_0", &pCSBlob);
    if (FAILED(hr))
    {
        FUtil::Log("Error: cannot compile compute shader for projected grid");
        return hr;
    }
    V_RETURN(mDevice->CreateComputeShader(pCSBlob->GetBufferPointer(), pCSBlob->GetBufferSize(), NULL, &mCS));
    


    //create constant buffer
    ZeroMemory(&bd, sizeof(bd));
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
    V_RETURN(mDevice->CreateUnorderedAccessView(mGridBuffer, &DescUAV, &mGridBufferUAV));



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

    SAFE_RELEASE(mVertexShaderQuad);
    SAFE_RELEASE(mPixelShaderQuad);

    SAFE_RELEASE(mVertexShaderReflection);
    SAFE_RELEASE(mPixelShaderReflection);

	SAFE_RELEASE(mCBChangesEveryFrame);
	SAFE_RELEASE(mTextureRV);
	SAFE_RELEASE(mSamplerLinear);

    SAFE_RELEASE(mRSCullNone);
    SAFE_RELEASE(mRSOrdinary);


    // compute shaders and resources
    surface.Release();

    SAFE_RELEASE(mCBforCS);

    SAFE_RELEASE(mVertexShaderWater);
    SAFE_RELEASE(mPixelShaderWater);


	return S_OK;
}

//------------------------------------------------------------------------------








#endif //PROJ_TESTPROJECT
