


#include "MathHelper.h"



namespace MathHelper
{
	XMVECTOR VectorFromFloat3(XMFLOAT3 vec)
	{
		return XMVectorSet(vec.x, vec.y, vec.z, 1);
	}

	XMVECTOR VectorFromFloat4(XMFLOAT4 vec)
	{
		return XMVectorSet(vec.x, vec.y, vec.z, vec.w);
	}



};