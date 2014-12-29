/*
=======================================================================

=======================================================================
*/


#include "camera.h"


DXCamera::DXCamera() :
    mode(CameraMode::CM_Orbit)
{
    radius = 0;
    phi = 0;
    theta = 0;

    vAt = XMVectorSet(0,0,0,1);
    vUp = XMVectorSet(0,1,0,1);
    vEye = XMVectorSet(1,1,1,1);
}

DXCamera::~DXCamera()
{
}



void DXCamera::setProjectionParams(double fov, double aspect, double nearplane, double farplane)
{
    this->aspect = aspect;
    this->fov = fov;
    this->nearPlane = nearplane;
    this->farPlane = farplane;
}


void DXCamera::setOrbitParams( double radius, XMFLOAT3 target )
{
    this->radius = radius;

    phi = 0;
    theta = 0;

    vAt = XMVectorSet(target.x, target.y, target.z, 1);
    vUp = XMVectorSet(0,1,0,1);
}


void DXCamera::setFirstPersonParams()
{
    //TODO:
}



void DXCamera::FrameMove(XMFLOAT3 vOffset, XMFLOAT3 vMouse)
{
    // transform view depending on camera mode
    switch (mode)
    {

    case DXCamera::CM_FirstPerson:

        //TODO:
        //not implemented yet

        break;

    case DXCamera::CM_Orbit:

        //orbiting camera

        phi += (float)vMouse.x * 0.001f;
        theta += (float)vMouse.y * 0.001f;

        radius -= (float)vMouse.z * 0.005f;

        vAt = XMVector3TransformCoord(vAt, XMMatrixTranslation(vOffset.x, vOffset.y, vOffset.z));

        vEye = XMVectorSet(
            radius*cos(theta)*sin(phi),
            radius*sin(theta),
            radius*cos(theta)*cos(phi),
            0.0f );

        break;

    default:
        break;
    }
}



XMMATRIX DXCamera::getViewMatrix()
{
    XMMATRIX viewMat = XMMatrixLookAtLH( vEye, vAt, vUp );

    return viewMat;
}


XMMATRIX DXCamera::getProjMatrix()
{
    return XMMatrixPerspectiveFovLH( fov, aspect, nearPlane, farPlane );
}

XMVECTOR DXCamera::getForwardVector()
{
    XMVECTOR vec = vAt - vEye;
    vec.m128_f32[3] = 1.0f;
    return XMVector3Normalize(vec);
}

XMVECTOR DXCamera::getRightVector()
{
    XMVECTOR vec = XMVectorSet(1,0,0,1);
    XMVECTOR det;
    XMMATRIX invViewProj = XMMatrixInverse(&det, getViewMatrix() * getProjMatrix() );
    return XMVector3TransformCoord(vec, invViewProj);
}