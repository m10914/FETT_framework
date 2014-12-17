/*
================================================================


================================================================
*/


#include "Surface.h"
#include "FUtil.h"


FSurface::FSurface(int sizeX, int sizeY) : 
    SizeX(sizeX), SizeY(sizeY)
{
}

FSurface::~FSurface()
{
}



void FSurface::initBuffer(LPD3D11Device device)
{
    HRESULT hr;


    //----------------------------------------------------
    // vertex buffer

    // Create vertex buffer
    int gridX = SizeX + 1;
    int gridY = SizeY + 1;
    int total = gridX * gridY;

    //alloc
    VertexFormatPNT* vertices = new VertexFormatPNT[total];

    static double size = 10.0;

    for(int i = 0; i < gridX; i++)
        for(int j = 0; j < gridY; j++)
        {
            vertices[i + j*gridX].Pos = XMFLOAT3(
                (float)(i-gridX*0.5f)*(size/gridX),
                0,
                (float)(j-gridY*0.5f)*(size/gridY)
                );
            vertices[i + j*gridX].Normal = XMFLOAT3(0, 1, 0);
            vertices[i + j*gridX].Tex = XMFLOAT2(0, 0);
        }

    D3D11_BUFFER_DESC bd;
    ZeroMemory( &bd, sizeof(bd) );
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof( VertexFormatPNT ) * total;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory( &InitData, sizeof(InitData) );
    InitData.pSysMem = vertices;
    hr = device->CreateBuffer( &bd, &InitData, &mVertexBuffer );
    if( FAILED( hr ) )
        return;

    //free
    delete [] vertices;


    //---------------------------------------------------------
    // index buffer

    int totalInd = 6 * SizeX * SizeY;
    WORD *indices = new WORD[totalInd];
    int i = 0;
    {
        for(int v=0; v < SizeY; v++){
            for(int u=0; u < SizeX; u++){

                // face 1 |/
                indices[i++]	= v*gridX + u;
                indices[i++]	= (v+1)*gridX + u;
                indices[i++]	= v*gridX + u + 1;

                // face 2 /|
                indices[i++]	= (v+1)*gridX + u;
                indices[i++]	= (v+1)*gridX + u + 1;
                indices[i++]	= v*gridX + u + 1;
            }
        }
    }

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof( WORD ) * totalInd;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;
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

void FSurface::recreateBuffer(LPD3DDeviceContext context)
{
    XMMATRIX viewProjInverted;
    XMVECTOR det;
    bVisible = getProjectedPointsMatrix(viewProjInverted);
    viewProjInverted = XMMatrixInverse( &det, viewProjInverted );

    if(bVisible)
    {
        //change the buffer
        VertexFormatPNT* verts = NULL;
        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT hr = context->Map( mVertexBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &mapped);
        if(hr == E_FAIL)
        {
            bVisible = false;
            return;
        }
        verts = (VertexFormatPNT*)mapped.pData;

        //-------------------------------------------------------

        XMVECTOR corner0, corner1, corner2, corner3;
        corner0 = calcWorldPosOfCorner( XMFLOAT2(0,0), &viewProjInverted);
        corner1 = calcWorldPosOfCorner( XMFLOAT2(1,0), &viewProjInverted);
        corner2 = calcWorldPosOfCorner( XMFLOAT2(0,1), &viewProjInverted);
        corner3 = calcWorldPosOfCorner( XMFLOAT2(1,1), &viewProjInverted);

        float du = 1.0f/float(SizeX-1);
        float dv = 1.0f/float(SizeY-1);
        float u, v = 0.0f;

        XMFLOAT4 result;
        int i=0;
        for(int iv=0; iv < SizeY; iv++)
        {
            u = 0.0f;		
            for(int iu=0; iu < SizeX; iu++)
            {						
                result.x = (1.0f-v)*( (1.0f-u)*corner0.m128_f32[0] + u*corner1.m128_f32[0] ) + v*( (1.0f-u)*corner2.m128_f32[0] + u*corner3.m128_f32[0] );				
                result.z = (1.0f-v)*( (1.0f-u)*corner0.m128_f32[2] + u*corner1.m128_f32[2] ) + v*( (1.0f-u)*corner2.m128_f32[2] + u*corner3.m128_f32[2] );				
                result.w = (1.0f-v)*( (1.0f-u)*corner0.m128_f32[3] + u*corner1.m128_f32[3] ) + v*( (1.0f-u)*corner2.m128_f32[3] + u*corner3.m128_f32[3] );	

                float divide = 1.0f/result.w;				
                result.x *= divide;
                result.z *= divide;

                verts[i].Pos.x = result.x;
                verts[i].Pos.z = result.z;
                verts[i].Pos.y = 0;

                i++;
                u += du;
            }
            v += dv;			
        }

        //-------------------------------------------------------

        context->Unmap( mVertexBuffer, NULL );
    }
}



bool FSurface::getProjectedPointsMatrix(XMMATRIX& outMatrix)
{
    //------------------------
    // get displacement zone

    static float amplitude = 5;
    static XMVECTOR normal = XMVectorSet(0, 1, 0, 1);

    XMVECTOR curPos = XMVectorSet( position.x, position.y, position.z, 1);
    XMVECTOR upper = curPos + normal * XMVectorSet(amplitude, amplitude, amplitude, 1);   
    XMVECTOR lower = curPos - normal * XMVectorSet(amplitude, amplitude, amplitude, 1); 

    plane = XMPlaneFromPointNormal( curPos, normal );
    upperPlane = XMPlaneFromPointNormal( upper, normal );
    lowerPlane = XMPlaneFromPointNormal( lower, normal );

    //-------------------------
    // 

    float x_min, y_min, x_max, y_max;
    XMVECTOR frustum[8], proj_points[24];
    int n_points=0;
    int cube[] =
    {	0,1,	0,2,	2,3,	1,3,
        0,4,	2,6,	3,7,	1,5,
        4,6,	4,5,	5,7,	6,7 };

    //get inv_view_proj
    XMVECTOR determinant;
    XMMATRIX viewProjInverted = mCamera->getViewMatrix() * mCamera->getProjMatrix();
    viewProjInverted = XMMatrixInverse( &determinant, viewProjInverted );

    frustum[0] = XMVector4Transform( XMVectorSet(-1,-1,-1, 1), viewProjInverted );
    frustum[1] = XMVector4Transform( XMVectorSet(+1,-1,-1, 1), viewProjInverted );
    frustum[2] = XMVector4Transform( XMVectorSet(-1,+1,-1, 1), viewProjInverted );
    frustum[3] = XMVector4Transform( XMVectorSet(+1,+1,-1, 1), viewProjInverted );
    frustum[4] = XMVector4Transform( XMVectorSet(-1,-1,+1, 1), viewProjInverted );
    frustum[5] = XMVector4Transform( XMVectorSet(+1,-1,+1, 1), viewProjInverted );
    frustum[6] = XMVector4Transform( XMVectorSet(-1,+1,+1, 1), viewProjInverted );
    frustum[7] = XMVector4Transform( XMVectorSet(+1,+1,+1, 1), viewProjInverted );

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
    XMVECTOR projCamPos = mCamera->getEye();
    XMVECTOR camFwd = mCamera->getForwardVector();
    static float paramElevation = 7.0f;
    static float paramStrength = 1.0f;

    // create project camera
    float height_in_plane = ( lowerPlane.m128_f32[0]*projCamPos.m128_f32[0] +
        lowerPlane.m128_f32[1]*projCamPos.m128_f32[1] +
        lowerPlane.m128_f32[2]*projCamPos.m128_f32[2] );

    {
        XMVECTOR aimpoint, aimpoint2;		
        XMVECTOR dif;

        if (height_in_plane < paramStrength + paramElevation)
        {	
            float multiplier = paramStrength + paramElevation - height_in_plane;
            dif = XMVectorSet(lowerPlane.m128_f32[0]*multiplier, lowerPlane.m128_f32[1]*multiplier, lowerPlane.m128_f32[2]*multiplier, 0);
            projCamPos += dif;
        } 
        
        // aim the projector at the point where the camera view-vector intersects the plane
        // if the camera is aimed away from the plane, mirror it's view-vector against the plane
        if( (XMPlaneDotNormal(plane, camFwd).m128_f32[0] < 0.0f) ^  
            (XMPlaneDotCoord(plane, mCamera->getEye()).m128_f32[0] < 0.0f)
            )
        {
            aimpoint = XMPlaneIntersectLine( plane, mCamera->getEye(), mCamera->getTarget() );
        }
        else
        {
            //flipped = rendering_camera->forward - 2*normal*D3DXVec3Dot(&(rendering_camera->forward),&normal);
            XMVECTOR flipped = camFwd - 2 * normal * XMVector3Dot(camFwd, normal);
            aimpoint = XMPlaneIntersectLine( plane, mCamera->getEye(), mCamera->getEye() + flipped );
        }

        // force the point the camera is looking at in a plane, and have the projector look at it
        // works well against horizon, even when camera is looking upwards
        // doesn't work straight down/up
        float af = fabs(XMPlaneDotNormal(plane, camFwd).m128_f32[0]);
        
        //aimpoint2 = (rendering_camera->position + 10.0f * rendering_camera->forward);
        //aimpoint2 = aimpoint2 - normal*D3DXVec3Dot(&aimpoint2,&normal);
        aimpoint2 = mCamera->getEye() + 10.0f * camFwd;
        aimpoint2 = aimpoint2 - normal * XMVector3Dot( aimpoint2, normal );

        // fade between aimpoint & aimpoint2 depending on view angle

        aimpoint = aimpoint*af + aimpoint2*(1.0f-af);
        //aimpoint = aimpoint2;

        XMVECTOR projCamFwd = aimpoint - projCamPos;

        // attach proj cam view
        projCamView = XMMatrixLookAtLH( projCamPos, projCamPos + projCamFwd, XMVectorSet(0,1,0,1) );
        projCamProj = mCamera->getProjMatrix();

        projectorWorldViewInverted = XMMatrixInverse( &determinant, projCamView * projCamProj );
    }


    for(int i=0; i<n_points; i++)
    {
        // project the point onto the surface plane
        proj_points[i] = proj_points[i] - normal*XMVector3Dot(proj_points[i], normal);	
    }
    
    for(int i=0; i<n_points; i++)
    {
        XMMATRIX ccombo = projCamView * projCamProj;
        proj_points[i] = XMVector4Transform( proj_points[i], ccombo );
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


        // build the packing matrix that spreads the grid across the "projection window"
        XMMATRIX pack;
        XMMatrixSet(
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




HRESULT FSurface::Init(LPD3D11Device device)
{
    initBuffer(device);

    return S_OK;
}

HRESULT FSurface::Render(LPD3DDeviceContext context)
{
    //recreate buffer
    //temporary measure, needs to be done on GPU completely
    recreateBuffer(context);
    if(!bVisible) return S_OK;


    // Set vertex buffer
    UINT stride = sizeof( VertexFormatPNT );
    UINT offset = 0;
    context->IASetVertexBuffers( 0, 1, &mVertexBuffer, &stride, &offset );

    // Set index buffer
    context->IASetIndexBuffer( mIndexBuffer, DXGI_FORMAT_R16_UINT, 0 );

    // Set primitive topology
    context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    // draw dat shit
    context->DrawIndexed( 6 * SizeX * SizeY, 0, 0 );
    
    return S_OK;
}

HRESULT FSurface::Release()
{
    SAFE_RELEASE(mVertexBuffer);
    SAFE_RELEASE(mIndexBuffer);

    return S_OK;
}

