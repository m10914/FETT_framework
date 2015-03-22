/*
=======================================================================

Basic class for light sources
Includes FLightSource abstract class and it's basic implementation - directionalLight

=======================================================================
*/

#pragma once


#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>
#include <xnamath.h>

#include <xmmintrin.h>
#include "MathHelper.h"


class FLightSource
{
public:

	// V A R I A B L E S

	// TODO:



	// M E T H O D S

	// TODO:
};




/*
Directional light has no position - only direction, which affects every pixel.
*/
class FDirectionalLight //: public FLightSource
{
private:
	float phi = 0;
	float theta = 0;
	float radius = 55;

	XMVECTOR vAt;
	XMVECTOR vEye;
	XMVECTOR vUp;

protected:
	XMFLOAT3 dir;

public:
	XMFLOAT3 getDir() { return dir; }

	void FrameMove(XMFLOAT3 vectorAt, XMFLOAT3 axis);
	
	void GetTransformMatrix(XMMATRIX* mOut);
	void GetProjectionMatrix(XMMATRIX* mOut);
	void GetMVPMatrix(XMMATRIX* mOut);
};