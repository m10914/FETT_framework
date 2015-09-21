
#include "FPPlane.h"


FPPlane::FPPlane():
	mVertexBuffer(NULL),
	mIndexBuffer(NULL)
{

}


FPPlane::~FPPlane()
{

}




HRESULT FPPlane::Init(LPD3D11Device device)
{
	HRESULT hr;

	// Create vertex buffer
	VertexFormatPT vertices[] =
	{
		{ XMFLOAT3( -1.0f, 0.0f, -1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3( 1.0f, 0.0f, -1.0f ), XMFLOAT2( 50.0f, 0.0f ) },
		{ XMFLOAT3( 1.0f, 0.0f, 1.0f ), XMFLOAT2( 50.0f, 50.0f ) },
		{ XMFLOAT3( -1.0f, 0.0f, 1.0f ), XMFLOAT2( 0.0f, 50.0f ) },
	};

	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof( VertexFormatPT ) * 4;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory( &InitData, sizeof(InitData) );
	InitData.pSysMem = vertices;
	hr = device->CreateBuffer( &bd, &InitData, &mVertexBuffer );
	if( FAILED( hr ) )
		return hr;

	// create index buffer
	WORD indices[] =
	{
		3,1,0,
		2,1,3,
	};

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof( WORD ) * 6;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = indices;
	hr = device->CreateBuffer( &bd, &InitData, &mIndexBuffer );

	return hr;
}


HRESULT FPPlane::Render(LPD3DDeviceContext context)
{
	// Set vertex buffer
	UINT stride = sizeof( VertexFormatPT );
	UINT offset = 0;
	context->IASetVertexBuffers( 0, 1, &mVertexBuffer, &stride, &offset );

	// Set index buffer
	context->IASetIndexBuffer( mIndexBuffer, DXGI_FORMAT_R16_UINT, 0 );

	// draw dat shit
	context->DrawIndexed( 6, 0, 0 );

	return S_OK;
}


HRESULT FPPlane::Release()
{
	SAFE_RELEASE(mVertexBuffer);
	SAFE_RELEASE(mIndexBuffer);

	return S_OK;
}