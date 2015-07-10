
#pragma once

#include <windows.h>
#include <d3d11.h>

#include <vector>
#include <unordered_map>
#include <memory>
#include <xnamath.h>



struct _RTTex
{
    ID3D11Texture2D*					mRTSecondTex = NULL;
    ID3D11ShaderResourceView*			mRTSecondSRV = NULL;
    ID3D11RenderTargetView*				mRTSecondRTV = NULL;
    ID3D11UnorderedAccessView*          mRTSecondUAV = NULL;
};


class RenderTarget
{
public:

    static RenderTarget* getByName(const LPCSTR& name);

    ~RenderTarget();
    RenderTarget(const LPCSTR name, const XMFLOAT2& size);

    void appendTexture(UINT index, DXGI_FORMAT fmt);
    void appendDepthStencil(DXGI_FORMAT fmt);
    
    ID3D11ShaderResourceView* getTextureSRV(UINT index);
    ID3D11UnorderedAccessView* getTextureUAV(UINT index);
    ID3D11ShaderResourceView* getDepthStencilSRV();

    void activate();
    void deactivate();

    void clear(XMFLOAT4 color);

protected:

    //static functional
    static std::unordered_map<LPCSTR, RenderTarget*> namedRenderTargets;

    
    char name[512];
    XMFLOAT2 size;

    //depth-stencil
    ID3D11Texture2D*					mDSSecondTex = NULL;
    ID3D11ShaderResourceView*			mDSSecondSRV = NULL;
    ID3D11DepthStencilView*				mDSSecondDSV = NULL;

    //textures
    std::vector<_RTTex> textures;
};