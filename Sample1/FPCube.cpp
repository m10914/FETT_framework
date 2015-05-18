
#include "FPCube.h"



FPCube::FPCube():
	mVertexBuffer(NULL),
	mIndexBuffer(NULL)
{

}


FPCube::~FPCube()
{

}




HRESULT FPCube::Init(LPD3D11Device device)
{
	HRESULT hr;

	// Create vertex buffer
	VertexFormatPT vertices[] =
	{
		{ XMFLOAT3( -1.0f, 1.0f, -1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3( 1.0f, 1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3( 1.0f, 1.0f, 1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f, 1.0f, 1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3( 1.0f, -1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3( 1.0f, -1.0f, 1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f, -1.0f, 1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT3( -1.0f, -1.0f, 1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3( -1.0f, 1.0f, -1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f, 1.0f, 1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT3( 1.0f, -1.0f, 1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3( 1.0f, -1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3( 1.0f, 1.0f, -1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT3( 1.0f, 1.0f, 1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3( 1.0f, -1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3( 1.0f, 1.0f, -1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f, 1.0f, -1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT3( -1.0f, -1.0f, 1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3( 1.0f, -1.0f, 1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3( 1.0f, 1.0f, 1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f, 1.0f, 1.0f ), XMFLOAT2( 0.0f, 1.0f ) },
	};

    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory( &InitData, sizeof(InitData) );

	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof( VertexFormatPT ) * 24;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	
	InitData.pSysMem = vertices;
	hr = device->CreateBuffer( &bd, &InitData, &mVertexBuffer );
	if( FAILED( hr ) )
		return hr;


	// Create index buffer
	// Create vertex buffer
	WORD indices[] =
	{
		3,1,0,
		2,1,3,

		6,4,5,
		7,4,6,

		11,9,8,
		10,9,11,

		14,12,13,
		15,12,14,

		19,17,16,
		18,17,19,

		22,20,21,
		23,20,22
	};

    bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof( WORD ) * 36;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;

	InitData.pSysMem = indices;
	hr = device->CreateBuffer( &bd, &InitData, &mIndexBuffer );

    numPrims = 36;

	return hr;
}


void FPCube::tesselate(float dist, LPD3D11Device device, LPD3DDeviceContext context)
{
    //release old constructions
    SAFE_RELEASE(mIndexBuffer);
    SAFE_RELEASE(mVertexBuffer);


    VertexFormatPT origVertices[] =
    {
        { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },

        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },

        { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },

        { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },

        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 1.0f) },

        { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
    };
    WORD origIndices[] =
    {
        3, 1, 0,
        2, 1, 3,

        6, 4, 5,
        7, 4, 6,

        11, 9, 8,
        10, 9, 11,

        14, 12, 13,
        15, 12, 14,

        19, 17, 16,
        18, 17, 19,

        22, 20, 21,
        23, 20, 22
    };

    std::vector<VertexFormatPT> vertices;
    std::vector<WORD> indices;
    std::map<UINT, WORD> newVertices;


    //1st - copy all vertices
    int numOrigVerts = sizeof(origVertices) / sizeof(VertexFormatPT);
    int numOrigIndices = sizeof(origIndices) / sizeof(WORD);

    int numInserted = 0;
    vertices.resize(numOrigVerts);
    std::memcpy(&vertices[0], origVertices, sizeof(origVertices));

    for (int i = 0; i < numOrigIndices; i+=3)
    {
        //let's watch each triangle
        WORD ind[3];
        for (int j = 0; j < 3; j++)
        {
            ind[j] = origIndices[i + j];
        }

        // invoke tesselation
        WORD* newIndPtr = NULL;
        int numInd = 0;
        tesselateTriangle(dist, vertices, indices, newVertices, ind, &newIndPtr, numInd);
        for (int j = 0; j < numInd * 3; j++)
            indices.push_back(newIndPtr[j]);

        delete[] newIndPtr;
    }


    //create
    HRESULT hr;

    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));

    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(VertexFormatPT) * vertices.size();
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    InitData.pSysMem = &vertices[0];
    hr = device->CreateBuffer(&bd, &InitData, &mVertexBuffer);

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(WORD) * indices.size();
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;

    InitData.pSysMem = &indices[0];
    hr = device->CreateBuffer(&bd, &InitData, &mIndexBuffer);

    numPrims = indices.size();
}



void FPCube::tesselateTriangle(
    float tessDist,
    std::vector<VertexFormatPT>& vertices,
    std::vector<WORD>& indices,
    std::map<UINT, WORD>& newVertices,
    WORD* triangleIndices,
    WORD** outTriangleIndices, int& outNumTriangles)
{
    //acquire triangle vertices
    VertexFormatPT trianglePoints[3];
    for (int i = 0; i < 3; i++)
        trianglePoints[i] = vertices[triangleIndices[i]];

    //determine whether we should tesselate or not
    bool bTesselate = false;
    for (int j = 0; j < 3; j++)
    {
        int prevInd = j > 0 ? j - 1 : 2;
        XMVECTOR side = XMVectorSet(
            trianglePoints[j].Pos.x - trianglePoints[prevInd].Pos.x,
            trianglePoints[j].Pos.y - trianglePoints[prevInd].Pos.y,
            trianglePoints[j].Pos.z - trianglePoints[prevInd].Pos.z,
            0);
        float len = XMVector4Length(side).m128_f32[0];

        if (len > tessDist)
        {
            bTesselate = true;

            break;
        }
    }

    //do subdivision
    if (bTesselate)
    {
        //insert new point - in the middle of each side
        // first, let's find out if vertex already been created by some other triangle
        UINT key;
        WORD newIndices[3];
        VertexFormatPT* createdVerts[3];
        XMVECTOR pt[3];

        for (int i = 0; i < 3; i++)
            pt[i] = MathHelper::VectorFromFloat3(trianglePoints[i].Pos);

        for (int i = 0; i < 3; i++)
        {
            int prevInd = i > 0 ? i - 1 : 2;
            key = KP(triangleIndices[i], triangleIndices[prevInd]);
            auto found = newVertices.find(key);
            if (found == newVertices.end())
            {
                //create new vertex
                XMVECTOR newPoint = XMVectorLerp(pt[i], pt[prevInd], 0.5);
                newIndices[i] = vertices.size();

                vertices.push_back({
                    XMFLOAT3(newPoint.m128_f32[0], newPoint.m128_f32[1], newPoint.m128_f32[2]),
                    XMFLOAT2(0.0f, 0.0f) });
                newVertices[key] = newIndices[i];
            }
            else
            {
                newIndices[i] = found->second;              
            }
            createdVerts[i] = &vertices[newIndices[i]];
        }


        //now we need to construct new triangles manually
        WORD triind[3];
        std::vector<WORD> resindices;
        resindices.clear();
        {
            triind[0] = triangleIndices[0];
            triind[1] = newIndices[1];
            triind[2] = newIndices[0];

            // invoke tesselation
            WORD* newIndPtr = NULL;
            int numInd = 0;
            tesselateTriangle(tessDist, vertices, indices, newVertices,
                triind, &newIndPtr, numInd);
            for (int j = 0; j < numInd * 3; j++)
                resindices.push_back(newIndPtr[j]);

            delete[] newIndPtr;
        }
        {
            triind[0] = newIndices[1];
            triind[1] = triangleIndices[1];
            triind[2] = newIndices[2];

            // invoke tesselation
            WORD* newIndPtr = NULL;
            int numInd = 0;
            tesselateTriangle(tessDist, vertices, indices, newVertices,
                triind, &newIndPtr, numInd);
            for (int j = 0; j < numInd * 3; j++)
                resindices.push_back(newIndPtr[j]);

            delete[] newIndPtr;
        }
        {
            triind[0] = newIndices[0];
            triind[1] = newIndices[2];
            triind[2] = triangleIndices[2];

            // invoke tesselation
            WORD* newIndPtr = NULL;
            int numInd = 0;
            tesselateTriangle(tessDist, vertices, indices, newVertices,
                triind, &newIndPtr, numInd);
            for (int j = 0; j < numInd * 3; j++)
                resindices.push_back(newIndPtr[j]);

            delete[] newIndPtr;
        }
        {
            triind[0] = newIndices[0];
            triind[1] = newIndices[1];
            triind[2] = newIndices[2];

            // invoke tesselation
            WORD* newIndPtr = NULL;
            int numInd = 0;
            tesselateTriangle(tessDist, vertices, indices, newVertices,
                triind, &newIndPtr, numInd);
            for (int j = 0; j < numInd * 3; j++)
                resindices.push_back(newIndPtr[j]);

            delete[] newIndPtr;
        }

        //just pass back input triangle
        int size = resindices.size();
        (*outTriangleIndices) = NULL;
        (*outTriangleIndices) = new WORD[size];
        memcpy((*outTriangleIndices), &resindices[0], size*sizeof(WORD));
        outNumTriangles = size / 3;
    }
    else
    {
        //just pass back input triangle
        (*outTriangleIndices) = NULL;
        (*outTriangleIndices) = new WORD[3];
        (*outTriangleIndices)[0] = triangleIndices[0];
        (*outTriangleIndices)[1] = triangleIndices[1];
        (*outTriangleIndices)[2] = triangleIndices[2];
        outNumTriangles = 1;
    }
}





HRESULT FPCube::Render(LPD3DDeviceContext context)
{
	// Set vertex buffer
	UINT stride = sizeof( VertexFormatPT );
	UINT offset = 0;
	context->IASetVertexBuffers( 0, 1, &mVertexBuffer, &stride, &offset );

	// Set index buffer
	context->IASetIndexBuffer( mIndexBuffer, DXGI_FORMAT_R16_UINT, 0 );

	// draw dat shit
	context->DrawIndexed( numPrims, 0, 0 );

	return S_OK;
}


HRESULT FPCube::Release()
{
	SAFE_RELEASE(mVertexBuffer);
	SAFE_RELEASE(mIndexBuffer);

	return S_OK;
}