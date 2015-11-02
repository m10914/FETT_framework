

#pragma once
#pragma warning(disable:4005 4324)

#include <windows.h>

#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>

#include <DxErr.h>
#include <xnamath.h>
#include "dxapp.h"



struct __declspec(align(16)) VertexFormatPT
{
    XMFLOAT3 Pos;
    XMFLOAT2 Tex;
};


struct __declspec(align(16)) VertexFormatPNT
{
    XMFLOAT3 Pos;
    XMFLOAT2 Tex;
    XMFLOAT3 Normal;
};

struct __declspec(align(16)) VertexFormatPPTT
{
    XMFLOAT3 Pos;
    XMFLOAT3 Pos2;
    XMFLOAT3 Upvec;
    XMFLOAT3 Upvec2;
    XMFLOAT2 uv;
    XMFLOAT2 uv2;
};

class VertexFormatMgr
{
protected:
    static ID3D11InputLayout* mPTLayout;
    static ID3D11InputLayout* mPNTLayout;
    static ID3D11InputLayout* mPPTTLayout;

    static ID3DBlob* getShaderBlob(char* vsFuncType);

public:

    static ID3D11InputLayout* getPTLayout();
    static ID3D11InputLayout* getPNTLayout();
    static ID3D11InputLayout* getPPTTLayout();

    static void release();
};

