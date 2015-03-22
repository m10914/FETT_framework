/*
=======================================================================

=======================================================================
*/


#include "FLightSource.h"



//---------------------------------------------
// FDirectionalLight implementation


void FDirectionalLight::FrameMove(XMFLOAT3 vectorAt, XMFLOAT3 axis)
{
	
	phi += (float)axis.x * 0.001f;
	theta += (float)axis.y * 0.001f;

	radius -= (float)axis.z * 0.005f;

	vAt = MathHelper::VectorFromFloat3(vectorAt);

	vEye = XMVectorSet(
		radius*cos(theta)*sin(phi),
		radius*sin(theta),
		radius*cos(theta)*cos(phi),
		0.0f);

	vUp = XMVectorSet(0, 1, 0, 1);
}

void FDirectionalLight::GetMVPMatrix(XMMATRIX* mOut)
{
	XMMATRIX transformMatrix;
	GetTransformMatrix(&transformMatrix);

	// generate proj
	XMMATRIX projMat;
	GetProjectionMatrix(&projMat);

	*mOut = transformMatrix * projMat;
}

void FDirectionalLight::GetTransformMatrix(XMMATRIX* mOut)
{
	*mOut = XMMatrixTranspose(XMMatrixLookAtLH(vEye, vAt, vUp));
}

void FDirectionalLight::GetProjectionMatrix(XMMATRIX* mOut)
{
	*mOut = XMMatrixTranspose(XMMatrixOrthographicLH(70, 70, 0.01, 150));
}
