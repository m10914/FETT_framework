/*
=======================================================================

Basic class for camera handling


=======================================================================
*/

#pragma once


#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>
#include <xnamath.h>

#include <xmmintrin.h>





class DXCamera
{
    enum CameraMode
    {
        CM_FirstPerson,
        CM_Orbit
    };


protected:

    CameraMode mode;

    float radius;
    float phi;
    float theta;

    XMVECTOR vAt;
    XMVECTOR vUp;
    XMVECTOR vEye;

    // projection
    float farPlane;
    float nearPlane;
    float fov;
    float aspect;

public:

    DXCamera();
    ~DXCamera();

    //set params
    void setOrbitParams( double radius, XMFLOAT3 target );
    void setProjectionParams(double fov, double aspect, double nearplane, double farplane);
    void setFirstPersonParams();

    XMMATRIX getViewMatrix();
    XMMATRIX getProjMatrix();

    void FrameMove(XMFLOAT3 vOffset, XMFLOAT3 vMouse);

    XMVECTOR getEye() { return vEye; }
    XMVECTOR getTarget() { return vAt; }

    void setMode(CameraMode md) { mode = md; };
    CameraMode getMode() { return mode; };

    float getNearPlane() { return nearPlane; };
    float getFarPlane() { return farPlane; };

};


