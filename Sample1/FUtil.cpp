


#include "FUtil.h"
#include "FPrimitive.h"


void FUtil::RenderPrimitive(FPrimitive* primitive, LPD3DDeviceContext context, CBMatrixSet& cbset, LPD3D11Buffer buffer)
{
    primitive->writeTransform( cbset );
    context->UpdateSubresource( buffer, 0, NULL, &cbset, 0, 0 );
    primitive->Render( context );
}


