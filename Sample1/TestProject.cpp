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
    mWarpBuffer[0] = NULL;
    mWarpBuffer[1] = NULL;
    mWarpBufferSRV[0] = NULL;
    mWarpBufferSRV[1] = NULL;
    mWarpBufferUAV[0] = NULL;
    mWarpBufferUAV[1] = NULL;

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
        static bool bTes = false;
        if (!bTes)
        {
            bTes = true;
            cube.tesselate(0.1, mDevice, mImmediateContext);
        }
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
	mImmediateContext->CSSetSamplers(0, 3, aSamplers);


    // render scene to shadow maps using warp maps
    _renderShadowMaps();

    // Render scene to 3 textures: color, normal, depth (auto)
    _renderSceneToGBuffer();

	// compute warp maps
    _renderComputeWarpMaps();

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
        ID3D11ShaderResourceView* srviews[1] = { mRTSecondSRV };
        //ID3D11ShaderResourceView* srviews[1] = { mReprojectionVisualSRV };
        mImmediateContext->PSSetShaderResources(0, 1, srviews);

        _renderQuad(mImmediateContext, mDevice, XMFLOAT2(0.00, 0.00), XMFLOAT2(1, 1));


        // reprojection texture
        /*
        srviews[0] = mReprojectionVisualSRV;
        mImmediateContext->PSSetShaderResources(0, 1, srviews);

        _renderQuad(mImmediateContext, mDevice, XMFLOAT2(0.01, 0.01), XMFLOAT2(0.2,0.2));
        */

		// depth map
		srviews[0] = mSMTempVisSRV;
		mImmediateContext->PSSetShaderResources(0, 1, srviews);

		_renderQuad(mImmediateContext, mDevice, XMFLOAT2(0.01, 0.01), XMFLOAT2(0.2, 0.2));


		// buffer visualize
		//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

		mImmediateContext->PSSetShader(mPixelShaderBufferVis, NULL, 0);
		mImmediateContext->VSSetShader(mVertexShaderBufferVis, NULL, 0);

		srviews[0] = mWarpBufferSRV[0];
        mImmediateContext->PSSetShaderResources(0, 1, srviews);

		_renderQuad(mImmediateContext, mDevice, XMFLOAT2(0.01, 0.22), XMFLOAT2(0.2, 0.02));

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
    float ClearColor[4] = { 0.525f, 0.525f, 0.525f, 1.0f }; // red, green, blue, alpha


    mImmediateContext->OMSetDepthStencilState(mDSOrdinary, sref);
    //mImmediateContext->RSSetState(mRSOrdinary);
    mImmediateContext->RSSetState(mRSWireframe);

    // set RT and clear it
    mImmediateContext->OMSetRenderTargets(1, &mRTSecondRTV, mDSSecondDSV);    
    mImmediateContext->ClearRenderTargetView(mRTSecondRTV, ClearColor);
    mImmediateContext->ClearDepthStencilView(mDSSecondDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
    UpdateViewport(swapChainDesc);

    // initial preparation
    mImmediateContext->IASetInputLayout(mLayoutPT);

    mImmediateContext->VSSetShader(mVertexShader, NULL, 0);
    mImmediateContext->PSSetShader(mPixelShader, NULL, 0);

    mImmediateContext->VSSetConstantBuffers(0, 1, &mCBChangesEveryFrame);
    mImmediateContext->PSSetConstantBuffers(0, 1, &mCBChangesEveryFrame);

    ID3D11ShaderResourceView* rv[3] = { mTextureRV, mWarpBufferSRV[0], mShadowMapSRV };
    mImmediateContext->PSSetShaderResources(0, 3, rv);
    mImmediateContext->VSSetShaderResources(0, 3, rv);


    _renderSceneObjects();


    rv[0] = NULL;
    rv[1] = NULL;
    rv[2] = NULL;
    mImmediateContext->PSSetShaderResources(0, 3, rv);
    mImmediateContext->VSSetShaderResources(0, 3, rv);


    D3DPERF_EndEvent();
}



void TestProject::_renderComputeWarpMaps()
{
    D3DPERF_BeginEvent(D3DCOLOR_RGBA(0, 255, 255, 0), L"Compute - warp maps");

    //clear RT
    mImmediateContext->OMSetRenderTargets(0, NULL, NULL);

    //----------------------------------------------------------------
    // do GPU computing 
    {
        //build buffer and other preparations
        {
            mDirLight.GetMVPMatrix(&mMemBufferForCSReprojection.matMVPLight);

            XMVECTOR det;
            XMMATRIX vmat = XMMatrixInverse(&det,
                XMMatrixTranspose(mainCamera.getProjMatrix()) *
                XMMatrixTranspose(mainCamera.getViewMatrix()));

            mMemBufferForCSReprojection.matMVPInv = vmat;

            mImmediateContext->UpdateSubresource(mCBforCS, 0, NULL, &mMemBufferForCSReprojection, 0, 0);

            // clear reproj buffer
            const UINT  values[4] = { 0, 0, 0, 0 };
            mImmediateContext->ClearUnorderedAccessViewUint(mReprojectionUAV, values);
        }

        D3DPERF_BeginEvent(D3DCOLOR_RGBA(0, 0, 255, 0), L"reprojection");

        ID3D11Buffer* csbuffers[1] = { mCBforCS };
        ID3D11UnorderedAccessView* aUAViews[1] = { mReprojectionUAV };
        ID3D11ShaderResourceView* srviews[1] = { mDSSecondSRV };

        mImmediateContext->CSSetShader(mComputeShaderReprojection, NULL, 0);

        mImmediateContext->CSSetConstantBuffers(0, 1, csbuffers);
        mImmediateContext->CSSetUnorderedAccessViews(0, 1, aUAViews, (UINT*)(&aUAViews));
        mImmediateContext->CSSetShaderResources(0, 1, srviews);

        // launch!
        int tG = ceil(1024 / 16.0f);
        mImmediateContext->Dispatch(tG, tG, 1); //numthreads(16,16,1)

        // unbind
        aUAViews[0] = NULL;
        mImmediateContext->CSSetUnorderedAccessViews(0, 1, aUAViews, (UINT*)(&aUAViews));
        srviews[0] = NULL;
        mImmediateContext->CSSetShaderResources(0, 1, srviews);
        csbuffers[0] = NULL;
        mImmediateContext->CSSetConstantBuffers(0, 1, csbuffers);


        if (false) //very very VERY slow, disable it for release
        {
            // visualize

            // copy to intermediate buffer
            mImmediateContext->CopyResource(mReprojectionTransferBuffer, mReprojectionBuffer);

            // copy to texture
            D3D11_MAPPED_SUBRESOURCE msSrc, msDest;
            mImmediateContext->Map(mReprojectionTransferBuffer, 0, D3D11_MAP_READ, 0, &msSrc);
            mImmediateContext->Map(mReprojectionVisualTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &msDest);

            memcpy(msDest.pData, msSrc.pData, sizeof(float) * 1024 * 1024);

            mImmediateContext->Unmap(mReprojectionVisualTexture, 0);
            mImmediateContext->Unmap(mReprojectionTransferBuffer, 0);
        }

        D3DPERF_EndEvent();

        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

        D3DPERF_BeginEvent(D3DCOLOR_RGBA(0, 0, 255, 0), L"importance calculation");

        aUAViews[0] = mWarpBufferUAV[0];
        srviews[0] = mReprojectionSRV;

        mImmediateContext->CSSetShader(mComputeShaderImportance, NULL, 0);

        mImmediateContext->CSSetUnorderedAccessViews(0, 1, aUAViews, (UINT*)(&aUAViews));
        mImmediateContext->CSSetShaderResources(0, 1, srviews);

        // launch!
        tG = ceil(1024 / 8.f);
        mImmediateContext->Dispatch(tG, 1, 1);

        srviews[0] = NULL;
        mImmediateContext->CSSetShaderResources(0, 1, srviews);

        D3DPERF_EndEvent();


        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

        D3DPERF_BeginEvent(D3DCOLOR_RGBA(0, 0, 255, 0), L"create warp maps");

        aUAViews[0] = mWarpBufferUAV[1];
        srviews[0] = mWarpBufferSRV[0];

        mImmediateContext->CSSetShader(mComputeShaderWarp, NULL, 0);

        mImmediateContext->CSSetUnorderedAccessViews(0, 1, aUAViews, (UINT*)(&aUAViews));
        mImmediateContext->CSSetShaderResources(0, 1, srviews);

        // launch!
        tG = ceil(1024 / 8.f);
        mImmediateContext->Dispatch(tG, 1, 1);

        srviews[0] = NULL;
        mImmediateContext->CSSetShaderResources(0, 1, srviews);

        D3DPERF_EndEvent();

        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

        D3DPERF_BeginEvent(D3DCOLOR_RGBA(0, 0, 255, 0), L"blur warp maps");

        aUAViews[0] = mWarpBufferUAV[0];
        srviews[0] = mWarpBufferSRV[1];

        mImmediateContext->CSSetShader(mComputeShaderBlurWarp, NULL, 0);

        mImmediateContext->CSSetUnorderedAccessViews(0, 1, aUAViews, (UINT*)(&aUAViews));
        mImmediateContext->CSSetShaderResources(0, 1, srviews);

        // launch!
        tG = ceil(1024 / 8.f);
        mImmediateContext->Dispatch(tG, 1, 1);

        D3DPERF_EndEvent();

        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        


        // Unbind
        aUAViews[0] = NULL;
        mImmediateContext->CSSetUnorderedAccessViews(0, 1, aUAViews, (UINT*)(&aUAViews));
        srviews[0] = NULL;
        mImmediateContext->CSSetShaderResources(0, 1, srviews);
    }

    D3DPERF_EndEvent();
}



void TestProject::_renderShadowMaps()
{
    UINT sref = 0;
    float ClearColor[4] = { 0.525f, 0.525f, 0.525f, 1.0f }; // red, green, blue, alpha


    mImmediateContext->RSSetState(mRSShadowMap);

    // initial preparation
    mImmediateContext->IASetInputLayout(mLayoutPT);

    // placeholder - just render from light's position
    mImmediateContext->OMSetRenderTargets(1, &mSMTempVisRTV, mShadowMapDSV);
    mImmediateContext->ClearRenderTargetView(mSMTempVisRTV, ClearColor);
    mImmediateContext->ClearDepthStencilView(mShadowMapDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
    UpdateViewport(1024, 1024);


    //-------------------------------------------------------------
    // now we render scene to shadowmap with RTW shader
    {
        D3DPERF_BeginEvent(D3DCOLOR_RGBA(0, 0, 255, 0), L"render to SM");

        mImmediateContext->VSSetShader(mVertexShaderRTW, NULL, 0);
        mImmediateContext->PSSetShader(mPixelShaderRTW, NULL, 0);

        ID3D11ShaderResourceView* vsSRVs[2] = { NULL, mWarpBufferSRV[0] };
        mImmediateContext->VSSetShaderResources(0, 2, vsSRVs);

        mImmediateContext->VSSetConstantBuffers(0, 1, &mCBChangesEveryFrame);

        mDirLight.GetProjectionMatrix(&cb.mProjection);
        mDirLight.GetTransformMatrix(&cb.mView);
        cb.mWorld = XMMatrixIdentity();

        _renderSceneObjects(false);

        vsSRVs[1] = NULL;
        mImmediateContext->VSSetShaderResources(0, 2, vsSRVs);

        D3DPERF_EndEvent();
    }
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

        ID3D11ShaderResourceView* srviews[4] = { mRTSecondSRV, mDSSecondSRV, mShadowMapSRV, mWarpBufferSRV[0] };
        mImmediateContext->PSSetShaderResources(0, 4, srviews);

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
    FUtil::InitVertexShader(mDevice, "TestProjectShader.fx", "VS_RTW", "vs_4_0", &pVSBlob, &mVertexShaderRTW);
	FUtil::InitVertexShader(mDevice, "BufferVisualizer.fx", "VS", "vs_4_0", &pVSBlob, &mVertexShaderBufferVis);

    ID3DBlob* pPSBlob = NULL;

    FUtil::InitPixelShader(mDevice, "TestProjectShader.fx", "PS", "ps_4_0", &pPSBlob, &mPixelShader);
	FUtil::InitPixelShader(mDevice, "TestProjectShader.fx", "PS_QUAD", "ps_4_0", &pPSBlob, &mPixelShaderQuad);
    FUtil::InitPixelShader(mDevice, "TestProjectShader.fx", "PS_RTW", "ps_4_0", &pPSBlob, &mPixelShaderRTW);
    FUtil::InitPixelShader(mDevice, "BufferVisualizer.fx", "PS", "ps_4_0", &pPSBlob, &mPixelShaderBufferVis);
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
    desc.CullMode = D3D11_CULL_NONE;
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


    //---------------------------------------
    // compute shader initialization

    LPCSTR version = "cs_5_0";

    ID3DBlob* pCSBlob = NULL;
    hr = FUtil::CompileShaderFromFile("Reprojection.hlsl", "CSMain", version, &pCSBlob);
    if (FAILED(hr))
    {
        FUtil::Log("Error: cannot compile compute shader for reprojection");
        return hr;
    }
    V_RETURN(mDevice->CreateComputeShader(pCSBlob->GetBufferPointer(), pCSBlob->GetBufferSize(), NULL, &mComputeShaderReprojection));
	SAFE_RELEASE(pCSBlob);

    hr = FUtil::CompileShaderFromFile("Importance.hlsl", "CalcImportance", version, &pCSBlob);
	if (FAILED(hr))
	{
		FUtil::Log("Error: cannot compile compute shader for importance");
		return hr;
	}
	V_RETURN(mDevice->CreateComputeShader(pCSBlob->GetBufferPointer(), pCSBlob->GetBufferSize(), NULL, &mComputeShaderImportance));

    hr = FUtil::CompileShaderFromFile("WarpCalculator.hlsl", "CalcWarp", version, &pCSBlob);
    if (FAILED(hr))
    {
        FUtil::Log("Error: cannot compile compute shader for importance");
        return hr;
    }
    V_RETURN(mDevice->CreateComputeShader(pCSBlob->GetBufferPointer(), pCSBlob->GetBufferSize(), NULL, &mComputeShaderWarp));

    hr = FUtil::CompileShaderFromFile("WarpCalculator.hlsl", "BlurWarp", version, &pCSBlob);
    if (FAILED(hr))
    {
        FUtil::Log("Error: cannot compile compute shader for importance");
        return hr;
    }
    V_RETURN(mDevice->CreateComputeShader(pCSBlob->GetBufferPointer(), pCSBlob->GetBufferSize(), NULL, &mComputeShaderBlurWarp));




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


        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        // textures for debug purposes only
        smDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        smDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        V_RETURN(mDevice->CreateTexture2D(&smDesc, NULL, &mSMTempVis));

        D3D11_RENDER_TARGET_VIEW_DESC DescRT;
        DescRT.Format = smDesc.Format;
        DescRT.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        DescRT.Texture2D.MipSlice = 0;
        V_RETURN(mDevice->CreateRenderTargetView(mSMTempVis, &DescRT, &mSMTempVisRTV));

        // Create the resource view
        D3D11_SHADER_RESOURCE_VIEW_DESC DescRV;
        DescRV.Format = smDesc.Format;
        DescRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        DescRV.Texture2D.MipLevels = 1;
        DescRV.Texture2D.MostDetailedMip = 0;
        V_RETURN(mDevice->CreateShaderResourceView(mSMTempVis, &DescRV, &mSMTempVisSRV));
	}


	// create reprojection texture and its views
	{
        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
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
        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

		// constant buffers
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;
		bd.ByteWidth = sizeof(CBforCSReprojection);
		hr = mDevice->CreateBuffer(&bd, NULL, &mCBforCS);
        V_RETURN(hr);

        bd.ByteWidth = sizeof(CBforDeferredPass);
        hr = mDevice->CreateBuffer(&bd, NULL, &mCBforDeferredPass);
        V_RETURN(hr);


		D3D11_BUFFER_DESC sbDesc;
		ZeroMemory(&sbDesc, sizeof(D3D11_BUFFER_DESC));
		sbDesc.Usage = D3D11_USAGE_DEFAULT;
		sbDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
		sbDesc.ByteWidth = 1024 * 1024 * sizeof(int);
		sbDesc.StructureByteStride = sizeof(int);
		sbDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

		int* arr = new int[1024 * 1024];
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
		sbSRVDesc.Buffer.ElementWidth = sizeof(int);
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


		//importance maps
		{
			ZeroMemory(&sbDesc, sizeof(D3D11_BUFFER_DESC));
			sbDesc.Usage = D3D11_USAGE_DEFAULT;
			sbDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
			sbDesc.ByteWidth = 1024 * sizeof(float) * 2;
			sbDesc.StructureByteStride = sizeof(float) * 2;
			sbDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

			float* arr = new float[1024*2];
			ZeroMemory(arr, sizeof(arr));
			InitData.pSysMem = arr;
            hr = mDevice->CreateBuffer(&sbDesc, &InitData, &mWarpBuffer[0]);
            hr = mDevice->CreateBuffer(&sbDesc, &InitData, &mWarpBuffer[1]);
			delete[] arr;
			V_RETURN(hr);

			sbSRVDesc.Buffer.NumElements = 1024;
            sbSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
            hr = mDevice->CreateShaderResourceView(mWarpBuffer[0], &sbSRVDesc, &mWarpBufferSRV[0]);
            hr = mDevice->CreateShaderResourceView(mWarpBuffer[1], &sbSRVDesc, &mWarpBufferSRV[1]);
			V_RETURN(hr);

			sbUAVDesc.Buffer.NumElements = 1024;
            sbUAVDesc.Format = DXGI_FORMAT_UNKNOWN;
            hr = mDevice->CreateUnorderedAccessView(mWarpBuffer[0], &sbUAVDesc, &mWarpBufferUAV[0]);
            hr = mDevice->CreateUnorderedAccessView(mWarpBuffer[1], &sbUAVDesc, &mWarpBufferUAV[1]);
            V_RETURN(hr);
		}

	}


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

	SAFE_RELEASE(mComputeShaderReprojection);
    SAFE_RELEASE(mCBforCS);

	SAFE_RELEASE(mReprojectionSRV);
	SAFE_RELEASE(mReprojectionUAV);
	SAFE_RELEASE(mReprojectionBuffer);


	return S_OK;
}

//------------------------------------------------------------------------------





