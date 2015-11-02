


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

    XMFLOAT3 Float3FromVector(XMVECTOR vec)
    {
        return XMFLOAT3(
            XMVectorGetX(vec),
            XMVectorGetY(vec),
            XMVectorGetZ(vec)
            );
    }
    XMFLOAT4 Float4FromVector(XMVECTOR vec)
    {
        return XMFLOAT4(
            XMVectorGetX(vec),
            XMVectorGetY(vec),
            XMVectorGetZ(vec),
            XMVectorGetW(vec)
            );
    }

};