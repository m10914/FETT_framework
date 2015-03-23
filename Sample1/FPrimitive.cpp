/*
==============================================================

==============================================================
*/



#include "FPrimitive.h"


FPrimitive::FPrimitive()
{
    scale = XMFLOAT3(1,1,1);
    position = XMFLOAT3(0,0,0);
    rotationEuler = XMFLOAT3(0,0,0);
}


XMMATRIX FPrimitive::getTransform()
{
    return
        XMMatrixScaling( scale.x, scale.y, scale.z )
        * XMMatrixRotationRollPitchYaw( rotationEuler.x, rotationEuler.y, rotationEuler.z )
        * XMMatrixTranslation(position.x, position.y, position.z);
}


void FPrimitive::writeTransform(CBMatrixSet& cbset)
{
    cbset.mWorld = XMMatrixTranspose( getTransform() );
	cbset.mvp = cbset.mProjection * cbset.mView * cbset.mWorld;
}