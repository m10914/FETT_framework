


#pragma once
#include <windows.h>
#include <d3d11.h>

#include <xnamath.h>


class PostEffect
{
public:

    PostEffect(LPCSTR pixShader, LPCSTR entryPoint);
    ~PostEffect();

    // virtual methods
    void render();

    /// fullscreen by default
    static void renderQuad(
        XMFLOAT2 offset = XMFLOAT2( 0, 0 ), 
        XMFLOAT2 relativeSize = XMFLOAT2( 1, 1 ));

protected:

    ID3D11PixelShader*                  mPixelShader = NULL;
    
    static ID3D11VertexShader*          mVertexShaderQuad;
    static ID3D11InputLayout*           mLayoutPT;

    char pixShaderName[512];
    char entryPoint[256];

    virtual void updateConstants() = 0;

    static ID3D11VertexShader*          getQuadVertexShader();

};