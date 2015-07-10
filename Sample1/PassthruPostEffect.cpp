

#include "PassthruPostEffect.h"


#include "RenderTarget.h"
#include "dxapp.h"



PassthruPostEffect::PassthruPostEffect()
    : PostEffect("FettEssential.fx", "PS_QUAD")
{

}


void PassthruPostEffect::updateConstants()
{
    // set constants here


    // set textures here
    RenderTarget* rt = RenderTarget::getByName("hbao");
    if (rt == nullptr) return;

    ID3D11ShaderResourceView* srviews[1] = { rt->getTextureSRV(0) };
    GFXCONTEXT->PSSetShaderResources(0, 1, srviews);
}
