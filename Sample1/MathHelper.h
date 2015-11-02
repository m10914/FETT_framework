/*
Some Math
*/


#pragma once


#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>
#include <xnamath.h>

#include <xmmintrin.h>



#define vf3(x) MathHelper::VectorFromFloat3(x)
#define vf4(x) MathHelper::VectorFromFloat4(x)
#define fv3(x) MathHelper::Float3FromVector(x)
#define fv4(x) MathHelper::Float4FromVector(x)


namespace MathHelper
{
	XMVECTOR VectorFromFloat4(XMFLOAT4 vec);
	XMVECTOR VectorFromFloat3(XMFLOAT3 vec);

    XMFLOAT3 Float3FromVector(XMVECTOR vec);
    XMFLOAT4 Float4FromVector(XMVECTOR vec);


};

