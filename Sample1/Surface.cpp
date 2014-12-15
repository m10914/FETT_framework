/*
================================================================


================================================================
*/


#include "Surface.h"



FSurface::FSurface(int sizeX, int sizeY) : 
    SizeX(sizeX), SizeY(sizeY)
{

}

FSurface::~FSurface()
{
}



HRESULT FSurface::Init(LPD3D11Device device)
{
    return S_OK;
}

HRESULT FSurface::Render(LPD3DDeviceContext context)
{
    return S_OK;
}

HRESULT FSurface::Release()
{
    return S_OK;
}

