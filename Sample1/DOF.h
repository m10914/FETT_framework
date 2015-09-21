/*
DOF

*/


#pragma once
#include <windows.h>
#include <d3d11.h>

#include <xnamath.h>




struct CBforDOF
{
    unsigned int rtWidth;
    unsigned int rtHeight;
    XMFLOAT2 oneOverRT;
    XMFLOAT4 blurQ;
};



class DOFEffect
{
public:
    DOFEffect();
    ~DOFEffect();


    //DO functions

    void doBlur(ID3D11ShaderResourceView* color, float blurQ);

    void doBokeh(
        ID3D11ShaderResourceView* color, 
        ID3D11ShaderResourceView* depth,
        int rtWidth, int rtHeight
        );

    void doMerge(ID3D11ShaderResourceView* blurredColor, ID3D11ShaderResourceView* bokehColor);



protected:

    // shaders
    ID3D11PixelShader*                  mPixelShader = NULL;
    ID3D11VertexShader*                 mVertexShader = NULL;

    ID3D11PixelShader*                  mFinPixelShader = NULL;
    ID3D11VertexShader*                 mFinVertexShader = NULL;

    ID3D11PixelShader*                  mBlurPixelShader = NULL;
    ID3D11VertexShader*                 mBlurVertexShader = NULL;


    // other stff

    ID3D11ShaderResourceView*           mBokehShapeTexSRV;

    CBforDOF  constBuffer;
    ID3D11Buffer*                       hwBuffer = NULL;

    ID3D11RasterizerState*              mRSCullNone = NULL;

    ID3D11SamplerState*                 mSamplerLinear = NULL;

    ID3D11BlendState*                   mBlendState = NULL;
};