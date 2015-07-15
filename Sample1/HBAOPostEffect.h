
#include "PostEffect.h"
#include "RenderTarget.h"



class HBAOConstBuffer
{
public:
    float FullResolution[2];
    float InvFullResolution[2];
    float AOResolution[2];
    float InvAOResolution[2];

    float FocalLen[2];
    float InvFocalLen[2];
    float UVToViewA[2];
    float UVToViewB[2];

    float R;
    float R2;
    float NegInvR2;
    float MaxRadiusPixels;

    float AngleBias;
    float TanAngleBias;
    float PowExponent;
    float Strength;

    float BlurDepthThreshold;
    float BlurFalloff;
    float LinA;
    float LinB;

    float nearFar[2];
    float somethingElse[2];
};





class HBAOPostEffect : public PostEffect
{
public:
    HBAOPostEffect();

protected:
    
    void initBuffers();
    void createRandomTexture();


    void updateConstants() override;
    void onPreRender() override;
    void onPostRender() override;


    HBAOConstBuffer csBuffer;
    ID3D11Buffer* hwBuffer = NULL;

    ID3D11SamplerState* mSamplerPointClamp = NULL;
    ID3D11SamplerState* mSamplerPointWrap = NULL;
    ID3D11SamplerState* mSamplerLinearClamp = NULL;
    
    ID3D11Texture2D*    mRandomTexture = NULL;
    ID3D11ShaderResourceView*   mRandomTextureSRV = NULL;

    RenderTarget*   hbaoRT = NULL;
    RenderTarget*   blurRT = NULL;

    ID3D11ComputeShader* blurShaderX = NULL;
    ID3D11ComputeShader* blurShaderY = NULL;


    class HBAOComposite : public PostEffect
    {
    public:

        HBAOComposite();

    protected:

        void updateConstants() override;
    };
    HBAOComposite* compositePFX = nullptr;
};

