/*
================================================================


================================================================
*/


#include "Surface.h"
#include "FUtil.h"


void OceanDescription::update(double appTime)
{
    // update perlin_move
    float mul = (float)appTime * PerlinSpeed;
    this->PerlinMove = XMFLOAT2(mul*wind_dir.x, -mul*wind_dir.y);
}

void OceanDescription::set(
    float iPerlinSize,
    float iPerlinSpeed,
    XMFLOAT3	iPerlinAmplitude,
    XMFLOAT3	iPerlinOctave,
    XMFLOAT3	iPerlinGradient,
    int idmap_dim,
    float ipatch_length,
    float itime_scale,
    float iwave_amplitude,
    XMFLOAT2 iwind_dir,
    float iwind_speed,
    float iwind_dependency,
    float ichoppy_scale)
{
    PerlinSize = iPerlinSize;
    PerlinSpeed = iPerlinSpeed;
    PerlinAmplitude = iPerlinAmplitude;
    PerlinOctave = iPerlinOctave;
    PerlinGradient = iPerlinGradient;
    dmap_dim = idmap_dim;
    patch_length = ipatch_length;
    time_scale = itime_scale;
    wave_amplitude = iwave_amplitude;
    wind_dir = iwind_dir;
    wind_speed = iwind_speed;
    wind_dependency = iwind_dependency;
    choppy_scale = ichoppy_scale;

    this->update(0);
}



FSurface::FSurface() : 
    SizeX(GRID_DIMENSION), SizeY(GRID_DIMENSION)
{
    numOfPositions = 0;
}

FSurface::~FSurface()
{
}



void FSurface::initBuffer(LPD3D11Device device)
{
    HRESULT hr;


    //----------------------------------------------------
    // vertex buffer

    // none - being created on compute shader

    //---------------------------------------------------------
    // index buffer

    int totalInd = 6 * (SizeX-1) * (SizeY-1);
    WORD *indices = new WORD[totalInd];
    int i = 0;
    {
        for(int v=0; v < (SizeY-1); v++){
            for(int u=0; u < (SizeX-1); u++){

                // face 1 |/
                indices[i++]	= v*SizeX + u;
                indices[i++]	= (v+1)*SizeX + u;
                indices[i++]	= v*SizeX + u + 1;

                // face 2 /|
                indices[i++]	= (v+1)*SizeX + u;
                indices[i++]	= (v+1)*SizeX + u + 1;
                indices[i++]	= v*SizeX + u + 1;
            }
        }
    }

    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.MiscFlags = 0;
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof( WORD ) * totalInd;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = indices;
    hr = device->CreateBuffer( &bd, &InitData, &mIndexBuffer );

    delete [] indices;

    return;
}


XMVECTOR FSurface::calcWorldPosOfCorner(XMFLOAT2 uv, XMMATRIX* matrix)
{
    // this is hacky.. this does take care of the homogenous coordinates in a correct way, 
    // but only when the plane lies at y=0
    XMVECTOR origin = XMVectorSet(uv.x, uv.y, -1.0f, 1.0f);
    XMVECTOR direction = XMVectorSet(uv.x, uv.y, 1.0f, 1.0f);

    origin = XMVector4Transform( origin, *matrix );
    direction = XMVector4Transform( direction, *matrix );
    direction -= origin;    

    float l = -origin.m128_f32[1] / direction.m128_f32[1];	// assumes the plane is y=0

    XMVECTOR worldPos = origin + direction*l;    
    return worldPos;
}


void FSurface::Update(double appTime, double deltaTime)
{
    mOceanSimulator->updateDisplacementMap((float)appTime);
}

bool FSurface::fillConstantBuffer(CBForCS& buffrer, double appTime)
{
    XMMATRIX viewProjInverted;
    XMVECTOR det;
    bVisible = getProjectedPointsMatrix(viewProjInverted);
    //viewProjInverted = XMMatrixInverse( &det, viewProjInverted );

    if (bVisible)
    {
        //-------------------------------------------------------

        XMVECTOR corner0, corner1, corner2, corner3;
        corner0 = calcWorldPosOfCorner(XMFLOAT2(0, 0), &viewProjInverted);
        corner1 = calcWorldPosOfCorner(XMFLOAT2(1, 0), &viewProjInverted);
        corner2 = calcWorldPosOfCorner(XMFLOAT2(0, 1), &viewProjInverted);
        corner3 = calcWorldPosOfCorner(XMFLOAT2(1, 1), &viewProjInverted);

        float du = 1.0f / float(SizeX);
        float dv = 1.0f / float(SizeY);

        // change the buffer

        buffrer.dTexcoord = XMFLOAT4(du, dv, 0, 0);
        buffrer.vCorner0 = XMFLOAT4(corner0.m128_f32[0], corner0.m128_f32[1], corner0.m128_f32[2], corner0.m128_f32[3]);
        buffrer.vCorner1 = XMFLOAT4(corner1.m128_f32[0], corner1.m128_f32[1], corner1.m128_f32[2], corner1.m128_f32[3]);
        buffrer.vCorner2 = XMFLOAT4(corner2.m128_f32[0], corner2.m128_f32[1], corner2.m128_f32[2], corner2.m128_f32[3]);
        buffrer.vCorner3 = XMFLOAT4(corner3.m128_f32[0], corner3.m128_f32[1], corner3.m128_f32[2], corner3.m128_f32[3]);

        // set world to zero

        XMMATRIX mat = XMMatrixTranslation(position.x, position.y, position.z);
        buffrer.worldMatrix = XMMatrixTranspose(mat);

        // set perlin params
        buffrer.PerlinAmplitude = XMFLOAT4(pOceanDesc->PerlinAmplitude.x, pOceanDesc->PerlinAmplitude.y, pOceanDesc->PerlinAmplitude.z, 0);
        buffrer.PerlinGradient = XMFLOAT4(pOceanDesc->PerlinGradient.x, pOceanDesc->PerlinGradient.y, pOceanDesc->PerlinGradient.z, 0);
        buffrer.PerlinOctave = XMFLOAT4(pOceanDesc->PerlinOctave.x, pOceanDesc->PerlinOctave.y, pOceanDesc->PerlinOctave.z, 0);
        buffrer.PerlinSize = XMFLOAT4(pOceanDesc->PerlinSize, 0, 0, 0);
        buffrer.PerlinMovement = XMFLOAT4(pOceanDesc->PerlinMove.x, pOceanDesc->PerlinMove.x, 0, 0);
    }

    return bVisible;
}



bool FSurface::getProjectedPointsMatrix(XMMATRIX& outMatrix)
{
    //------------------------
    // get displacement zone

    static float amplitude = 0.5f;// .5f;
    static float paramElevation = 1.0f;
    static XMVECTOR normal = XMVectorSet(0, 1, 0, 0);

    XMVECTOR camPos = mCamera->getEye() -XMVectorSet(0, position.y, 0, 0);
    XMVECTOR camDir = mCamera->getForwardVector();
    XMVECTOR camAt = mCamera->getTarget() -XMVectorSet(0, position.y, 0, 0);
    XMMATRIX camView = XMMatrixLookAtLH(camPos, camAt, XMVectorSet(0, 1, 0, 1));
    XMMATRIX camProj = mCamera->getProjMatrix();


    XMVECTOR curPos = XMVectorSet( 0, 0, 0, 1);
    XMVECTOR upper = curPos + normal * XMVectorSet(amplitude, amplitude, amplitude, 1);   
    XMVECTOR lower = curPos - normal * XMVectorSet(amplitude, amplitude, amplitude, 1); 

    plane = XMPlaneFromPointNormal( curPos, normal );
    upperPlane = XMPlaneFromPointNormal( upper, normal );
    lowerPlane = XMPlaneFromPointNormal( lower, normal );

    //-------------------------
    // 

    float x_min, y_min, x_max, y_max;
    XMVECTOR frustum[8], proj_points[24];
    int n_points = 0;
    int cube[] =
    {	0,1,	0,2,	2,3,	1,3,
        0,4,	2,6,	3,7,	1,5,
        4,6,	4,5,	5,7,	6,7 };

    //get inv_view_proj
    XMVECTOR determinant;
    
    XMMATRIX viewProjInverted = camView * camProj;
        
    viewProjInverted = XMMatrixInverse( &determinant, viewProjInverted );
    
    frustum[0] = XMVector3TransformCoord(XMVectorSet(-1,-1,-1, 1), viewProjInverted );
    frustum[1] = XMVector3TransformCoord( XMVectorSet(+1,-1,-1, 1), viewProjInverted );
    frustum[2] = XMVector3TransformCoord( XMVectorSet(-1,+1,-1, 1), viewProjInverted );
    frustum[3] = XMVector3TransformCoord( XMVectorSet(+1,+1,-1, 1), viewProjInverted );
    frustum[4] = XMVector3TransformCoord( XMVectorSet(-1,-1,+1, 1), viewProjInverted );
    frustum[5] = XMVector3TransformCoord( XMVectorSet(+1,-1,+1, 1), viewProjInverted );
    frustum[6] = XMVector3TransformCoord( XMVectorSet(-1,+1,+1, 1), viewProjInverted );
    frustum[7] = XMVector3TransformCoord( XMVectorSet(+1,+1,+1, 1), viewProjInverted );


    // check intersections with upper_bound and lower_bound	
    for(int i=0; i<12; i++){
        int src=cube[i*2], dst=cube[i*2+1];
        if (
            (upperPlane.m128_f32[0]*frustum[src].m128_f32[0] +
                upperPlane.m128_f32[1]*frustum[src].m128_f32[1] +
                upperPlane.m128_f32[2]*frustum[src].m128_f32[2] + upperPlane.m128_f32[3]*1.0f)/
            (upperPlane.m128_f32[0]*frustum[dst].m128_f32[0] +
                upperPlane.m128_f32[1]*frustum[dst].m128_f32[1] +
                upperPlane.m128_f32[2]*frustum[dst].m128_f32[2] + upperPlane.m128_f32[3]*1.0f) < 0)
        {			
            proj_points[n_points++] = XMPlaneIntersectLine( upperPlane, frustum[src], frustum[dst] );		
        }
        if (
            (lowerPlane.m128_f32[0]*frustum[src].m128_f32[0] +
                lowerPlane.m128_f32[1]*frustum[src].m128_f32[1] +
                lowerPlane.m128_f32[2]*frustum[src].m128_f32[2] + lowerPlane.m128_f32[3]*1.0f)/
            (lowerPlane.m128_f32[0]*frustum[dst].m128_f32[0] +
                lowerPlane.m128_f32[1]*frustum[dst].m128_f32[1] +
                lowerPlane.m128_f32[2]*frustum[dst].m128_f32[2] + lowerPlane.m128_f32[3]*1.0f) < 0)
        {			
            proj_points[n_points++] = XMPlaneIntersectLine( lowerPlane, frustum[src], frustum[dst] );		
        }
    }
    // check if any of the frustums vertices lie between the upper_bound and lower_bound planes
    {
        for(int i=0; i<8; i++)
        {		
            if(
                (upperPlane.m128_f32[0]*frustum[i].m128_f32[0] + 
                    upperPlane.m128_f32[1]*frustum[i].m128_f32[1] + 
                    upperPlane.m128_f32[2]*frustum[i].m128_f32[2] + upperPlane.m128_f32[3]*1.0f) / 
                (lowerPlane.m128_f32[0]*frustum[i].m128_f32[0] +
                    lowerPlane.m128_f32[1]*frustum[i].m128_f32[1] + 
                    lowerPlane.m128_f32[2]*frustum[i].m128_f32[2] + lowerPlane.m128_f32[3]*1.0f) < 0)
            {			
                proj_points[n_points++] = frustum[i];
            }		
        }	
    }


    XMMATRIX projCamView, projCamProj;
    XMVECTOR projCamPos = camPos;
    XMVECTOR camFwd = camDir;

    // create project camera
    float height_in_plane = ( lowerPlane.m128_f32[0]*projCamPos.m128_f32[0] +
        lowerPlane.m128_f32[1]*projCamPos.m128_f32[1] +
        lowerPlane.m128_f32[2]*projCamPos.m128_f32[2] );

    {
        XMVECTOR aimpoint, aimpoint2;		
        XMVECTOR dif;

        if (height_in_plane < amplitude + paramElevation)
        {	
            float multiplier = amplitude + paramElevation - height_in_plane;
            dif = XMVectorSet(lowerPlane.m128_f32[0]*multiplier, lowerPlane.m128_f32[1]*multiplier, lowerPlane.m128_f32[2]*multiplier, 0);
            projCamPos += dif;
        } 
        
        // aim the projector at the point where the camera view-vector intersects the plane
        // if the camera is aimed away from the plane, mirror it's view-vector against the plane
        if( (XMPlaneDotNormal(plane, camFwd).m128_f32[0] < 0.0f) ^  
            (XMPlaneDotCoord(plane, camPos).m128_f32[0] < 0.0f)
            )
        {
            aimpoint = XMPlaneIntersectLine( plane, camPos, camAt );
            if(XMVectorIsNaN(aimpoint).m128_f32[0] != 0)
                aimpoint = XMVectorSet(0,0,0,0); //parallel
        }
        else
        {
            //flipped = rendering_camera->forward - 2*normal*D3DXVec3Dot(&(rendering_camera->forward),&normal);
            XMVECTOR flipped = camFwd - 2 * normal * XMVector3Dot(camFwd, normal);
            aimpoint = XMPlaneIntersectLine( plane, camPos, camPos + flipped );
            if(XMVectorIsNaN(aimpoint).m128_f32[0] != 0)
                aimpoint = XMVectorSet(0,0,0,0); //parallel
        }

        // force the point the camera is looking at in a plane, and have the projector look at it
        // works well against horizon, even when camera is looking upwards
        // doesn't work straight down/up
        float af = fabs(XMPlaneDotNormal(plane, camFwd).m128_f32[0]);
        
        //aimpoint2 = (rendering_camera->position + 10.0f * rendering_camera->forward);
        //aimpoint2 = aimpoint2 - normal*D3DXVec3Dot(&aimpoint2,&normal);
        aimpoint2 = camPos + 10.0f * camFwd;
        aimpoint2 = aimpoint2 - normal * XMVector3Dot(aimpoint2, normal);

        // fade between aimpoint & aimpoint2 depending on view angle

        aimpoint = aimpoint*af + aimpoint2*(1.0f - af);
        //aimpoint = aimpoint2;

        XMVECTOR projCamFwd = aimpoint - projCamPos;

        // attach proj cam view
        projCamView = XMMatrixLookAtLH(
            projCamPos,
            projCamPos + projCamFwd,
            XMVectorSet(0,1,0,1) );
        projCamProj = camProj;

        projectorWorldViewInverted = XMMatrixInverse( &determinant, projCamView * projCamProj );
    }


    for(int i=0; i<n_points; i++)
    {
        // project the point onto the surface plane
        proj_points[i] = proj_points[i] -normal*XMVector3Dot(proj_points[i], normal);
        //proj_points[i].m128_f32[1] = position.y;
        proj_points[i].m128_f32[3] = 1.0f;
    }
    
    //------------------ TEST ONLY
    for(int i = 0; i < n_points; i++)
    {
        positions[i] = XMFLOAT3(proj_points[i].m128_f32[0], proj_points[i].m128_f32[1], proj_points[i].m128_f32[2]);
    }
    numOfPositions = n_points;


    XMMATRIX ccombo = projCamView * projCamProj;
    for(int i=0; i < n_points; i++)
    {
        proj_points[i] = XMVector3TransformCoord( proj_points[i], ccombo );
    }

    if (n_points > 0)
    {
        x_min = proj_points[0].m128_f32[0];
        x_max = proj_points[0].m128_f32[0];
        y_min = proj_points[0].m128_f32[1];
        y_max = proj_points[0].m128_f32[1];
        for(int i=1; i<n_points; i++)
        {
            if (proj_points[i].m128_f32[0] > x_max) x_max = proj_points[i].m128_f32[0];
            if (proj_points[i].m128_f32[0] < x_min) x_min = proj_points[i].m128_f32[0];
            if (proj_points[i].m128_f32[1] > y_max) y_max = proj_points[i].m128_f32[1];
            if (proj_points[i].m128_f32[1] < y_min) y_min = proj_points[i].m128_f32[1];
        }		

        //FUtil::Log("x (%.2f; %.2f), y: (%.2f; %.2f)\n", x_min, x_max, y_min, y_max);

        // build the packing matrix that spreads the grid across the "projection window"
        XMMATRIX pack;
        pack = XMMatrixSet(
            x_max-x_min,	0,				0,		x_min,
            0,				y_max-y_min,	0,		y_min,
            0,				0,				1,		0,	
            0,				0,				0,		1 );
        pack = XMMatrixTranspose(pack);

        outMatrix = pack * projectorWorldViewInverted;

        return true;
    } 

    return false;
}




HRESULT FSurface::Init(LPD3D11Device device, OceanDescription* desc)
{
    this->Init(device);

    //store ocean description
    this->pOceanDesc = desc;

    initBuffer(device);
    initOcean(device);

    return S_OK;
}
void FSurface::initOcean(LPD3D11Device device)
{
    // NVidia part
    {
        // Create ocean simulating object (copy from desc)
        OceanParameter ocean_param;

        ocean_param.dmap_dim = pOceanDesc->dmap_dim;
        ocean_param.patch_length = pOceanDesc->patch_length;
        ocean_param.time_scale = pOceanDesc->time_scale;
        ocean_param.wave_amplitude = pOceanDesc->wave_amplitude;
        ocean_param.wind_dir = D3DXVECTOR2(pOceanDesc->wind_dir.x, pOceanDesc->wind_dir.y);
        ocean_param.wind_speed = pOceanDesc->wind_speed;
        ocean_param.wind_dependency = pOceanDesc->wind_dependency;
        ocean_param.choppy_scale = pOceanDesc->choppy_scale;

        mOceanSimulator = new OceanSimulator(ocean_param, device);

        // Update the simulation for the first time.
        mOceanSimulator->updateDisplacementMap(0);
    }



    DWORD* buffer = new DWORD[FRESNEL_TEX_SIZE];
    for (int i = 0; i < FRESNEL_TEX_SIZE; i++)
    {
        float cos_a = i / (FLOAT)FRESNEL_TEX_SIZE;
        // Using water's refraction index 1.33
        DWORD fresnel = (DWORD)(D3DXFresnelTerm(cos_a, 1.33f) * 255);

        DWORD sky_blend = (DWORD)(powf(1 / (1 + cos_a), 16) * 255);

        buffer[i] = (sky_blend << 8) | fresnel;
    }

    D3D11_TEXTURE1D_DESC tex_desc;
    tex_desc.Width = FRESNEL_TEX_SIZE;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    tex_desc.Usage = D3D11_USAGE_IMMUTABLE;
    tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    tex_desc.CPUAccessFlags = 0;
    tex_desc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA init_data;
    init_data.pSysMem = buffer;
    init_data.SysMemPitch = 0;
    init_data.SysMemSlicePitch = 0;

    device->CreateTexture1D(&tex_desc, &init_data, &pFresnelTexture);

    SAFE_DELETE_ARRAY(buffer);

    // Create shader resource
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
    srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
    srv_desc.Texture1D.MipLevels = 1;
    srv_desc.Texture1D.MostDetailedMip = 0;

    device->CreateShaderResourceView(pFresnelTexture, &srv_desc, &pFresnelSRV);


    //load pre-generated perlin texture
    D3DX11CreateShaderResourceViewFromFile(device, "perlin_noise.dds", NULL, NULL, &pPerlinSRV, NULL);

}





HRESULT FSurface::Render(LPD3DDeviceContext context)
{
    // Set vertex buffer
    UINT stride = sizeof( VertexFormatPNT );
    UINT offset = 0;

    // Null vertex buffer - actual vertices info will be counted in compute shader
    context->IASetVertexBuffers(0, 0, NULL, &offset, &offset);

    // Set index buffer
    context->IASetIndexBuffer( mIndexBuffer, DXGI_FORMAT_R16_UINT, 0 );

    // Set primitive topology
    context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    // draw dat shit
    context->DrawIndexed( 6 * (SizeX-1) * (SizeY-1), 0, 0 );
    
    return S_OK;
}

HRESULT FSurface::Release()
{
    SAFE_RELEASE(mIndexBuffer);

    SAFE_DELETE(mOceanSimulator);

    return S_OK;
}

