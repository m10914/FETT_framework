

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


class VertexFormatMgr
{
protected:
    static ID3D11InputLayout* mPTLayout;
    static ID3D11InputLayout* mPNTLayout;


public:

    static ID3D11InputLayout* getPTLayout(ID3D11Device* device);
    static ID3D11InputLayout* getPNTLayout(ID3D11Device* device);


    static ID3DBlob* getShaderBlob(char* vsFuncType);
    static void release();
};

