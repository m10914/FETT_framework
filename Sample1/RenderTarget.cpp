


#include "RenderTarget.h"
#include "dxapp.h"
#include "FUtil.h"


std::unordered_map<LPCSTR, RenderTarget*> RenderTarget::namedRenderTargets;



RenderTarget* RenderTarget::getByName(const LPCSTR& name)
{
    auto found = namedRenderTargets.find(name);
    if (found != namedRenderTargets.end())
        return found->second;
    else
        return nullptr;
}





RenderTarget::RenderTarget(const LPCSTR name, const XMFLOAT2& size)
{
    //first add this to global counter
    //TODO: add collision check
    namedRenderTargets[name] = this;

    //copy persistent parameters
    strcpy(this->name, name);
    this->size = size;
}

RenderTarget::~RenderTarget()
{
    namedRenderTargets.erase(this->name);

    //delete resources
    for (auto tex : textures)
    {
        if (tex.mRTSecondTex)
            SAFE_RELEASE(tex.mRTSecondTex);
        if (tex.mRTSecondRTV)
            SAFE_RELEASE(tex.mRTSecondRTV);
        if (tex.mRTSecondSRV)
            SAFE_RELEASE(tex.mRTSecondSRV);
    }


    if (mDSSecondTex)
        SAFE_RELEASE(mDSSecondTex);
    if (mDSSecondSRV)
        SAFE_RELEASE(mDSSecondSRV);
    if (mDSSecondDSV)
        SAFE_RELEASE(mDSSecondDSV);
}

ID3D11ShaderResourceView* RenderTarget::getTextureSRV(UINT index)
{
    if (textures.size() > index)
        return textures[index].mRTSecondSRV;

    return NULL;
}

ID3D11ShaderResourceView* RenderTarget::getDepthStencilSRV()
{
    return mDSSecondSRV;
}

ID3D11UnorderedAccessView* RenderTarget::getTextureUAV(UINT index)
{
    if (textures.size() <= index)
        return NULL;

    auto tex = textures[index];
    if (tex.mRTSecondUAV == NULL)
    {
        HRESULT hr;
        hr = GFXDEVICE->CreateUnorderedAccessView(tex.mRTSecondTex, NULL, &tex.mRTSecondUAV);
        if (FAILED(hr))
        {
            FUtil::Log("Failed to create unordered access view for render target.");
            return NULL;
        }
    }

    return tex.mRTSecondUAV;
}


void RenderTarget::activate()
{
    ID3D11RenderTargetView** views = new ID3D11RenderTargetView*[textures.size()];
    for (int i = 0; i < textures.size(); i++)
        views[i] = textures[i].mRTSecondRTV;

    GFXCONTEXT->OMSetRenderTargets(textures.size(), views, mDSSecondDSV);

    // force update viewport
    D3D11_VIEWPORT vp;
    ZeroMemory(&vp, sizeof(vp));
    vp.Width = size.x;
    vp.Height = size.y;
    vp.MinDepth = 0.0F;
    vp.MaxDepth = 1.0F;

    GFXCONTEXT->RSSetViewports(1, &vp);
}

void RenderTarget::deactivate()
{
    GFXCONTEXT->OMSetRenderTargets(0, NULL, NULL);
}

void RenderTarget::clear(XMFLOAT4 color)
{
    //TODO:: add clear by index option
    for (auto itr : textures)
        GFXCONTEXT->ClearRenderTargetView(itr.mRTSecondRTV, &color.x);
    
    if (mDSSecondDSV)
        GFXCONTEXT->ClearDepthStencilView(mDSSecondDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
}




void RenderTarget::appendTexture(UINT index, DXGI_FORMAT fmt)
{
    HRESULT hr;

    //TODO: add collisions

    _RTTex newTex;

    // create G-Buffer albedo texture
    D3D11_TEXTURE2D_DESC Desc;
    ZeroMemory(&Desc, sizeof(D3D11_TEXTURE2D_DESC));
    Desc.ArraySize = 1;
    Desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    Desc.Usage = D3D11_USAGE_DEFAULT;
    Desc.Format = fmt;
    Desc.Width = (UINT)size.x;
    Desc.Height = (UINT)size.y;
    Desc.MipLevels = 1;
    Desc.SampleDesc.Count = 1;
    hr = GFXDEVICE->CreateTexture2D(&Desc, NULL, &newTex.mRTSecondTex);
    if (FAILED(hr))
    {
        FUtil::Log("Error: cannot create %d texture", (int)index);
        return;
    }

    D3D11_RENDER_TARGET_VIEW_DESC DescRT;
    DescRT.Format = Desc.Format;
    DescRT.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    DescRT.Texture2D.MipSlice = 0;
    hr = GFXDEVICE->CreateRenderTargetView(newTex.mRTSecondTex, &DescRT, &newTex.mRTSecondRTV);
    if (FAILED(hr))
    {
        FUtil::Log("Error: cannot create %d texture RTV", (int)index);
        return;
    }

    // Create the resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC DescRV;
    DescRV.Format = Desc.Format;
    DescRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    DescRV.Texture2D.MipLevels = 1;
    DescRV.Texture2D.MostDetailedMip = 0;
    hr = GFXDEVICE->CreateShaderResourceView(newTex.mRTSecondTex, &DescRV, &newTex.mRTSecondSRV);
    if (FAILED(hr))
    {
        FUtil::Log("Error: cannot create %d texture SRV", (int)index);
        return;
    }

    textures.push_back(std::move(newTex));

    return;
}


void RenderTarget::appendDepthStencil(DXGI_FORMAT fmt)
{
    HRESULT hr;

    //TODO: check if not typeless
    if (fmt != DXGI_FORMAT_R32_TYPELESS)
    {
        FUtil::Log("Error: incorrect depth stencil format.");
        return;
    }

    // DEPTH_STENCIL
    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory(&descDepth, sizeof(descDepth));
    descDepth.Width = (UINT)size.x;
    descDepth.Height = (UINT)size.y;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = fmt;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    hr = GFXDEVICE->CreateTexture2D(&descDepth, NULL, &mDSSecondTex);
    if (FAILED(hr))
    {
        FUtil::Log("Error: cannot create depth stencil texture");
        return;
    }

    //TODO: add proper formats handling
    DXGI_FORMAT dsvFmt = DXGI_FORMAT_D32_FLOAT;
    DXGI_FORMAT srvFmt = DXGI_FORMAT_R32_FLOAT;

    // Create the depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory(&descDSV, sizeof(descDSV));
    descDSV.Format = dsvFmt;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    hr = GFXDEVICE->CreateDepthStencilView(mDSSecondTex, &descDSV, &mDSSecondDSV);
    if (FAILED(hr))
    {
        FUtil::Log("Error: cannot create depth stencil DSV");
        return;
    }
    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
    shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
    shaderResourceViewDesc.Texture2D.MipLevels = 1;
    shaderResourceViewDesc.Format = srvFmt;
    hr = GFXDEVICE->CreateShaderResourceView(mDSSecondTex, &shaderResourceViewDesc, &mDSSecondSRV);
    if (FAILED(hr))
    {
        FUtil::Log("Error: cannot create depth stencil SRV");
        return;
    }
}
