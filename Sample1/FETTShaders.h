//TODO TODO TODO!!!


#pragma once
#include "dxapp.h"


#define SAFE_RELEASE(x)		if (x) { (x)->Release();		(x) = NULL; }	//!< Safe D3D-style release
#define SAFE_DELETE_ARRAY(p) { if (p) { delete[] (p);   (p)=NULL; } }
#define SAFE_DELETE(p) { if (p) { delete (p);   (p)=NULL; } }



// These classes will be improved to handle all shaders functionality without external
// knowledge (as it is now)

struct FETTPixelShader
{

    ~FETTPixelShader()
    {
        SAFE_RELEASE(shader);
    }

    ID3D11PixelShader** getPP() { return &shader; }
    ID3D11PixelShader* getP() { return shader; }

private:
    ID3D11PixelShader* shader = NULL;
};



struct FETTVertexShader
{
    ID3D11VertexShader** getPP() { return &shader; }
    ID3D11VertexShader* getP() { return shader; }

    ~FETTVertexShader()
    {
        SAFE_RELEASE(shader);
    }

private:
    ID3D11VertexShader* shader = NULL;
};



struct FETTGeometryShader
{
    ID3D11GeometryShader** getPP() { return &shader; }
    ID3D11GeometryShader* getP() { return shader; }

    ~FETTGeometryShader()
    {
        SAFE_RELEASE(shader);
    }

private:
    ID3D11GeometryShader* shader = NULL;
};