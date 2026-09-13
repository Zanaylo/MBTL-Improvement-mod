#pragma once

#include <d3d9.h>

class IDrawFilter
{
public:
	virtual ~IDrawFilter() = default;

	virtual IDirect3DBaseTexture9* OnSetTexture(IDirect3DDevice9*, DWORD, IDirect3DBaseTexture9* texture) { return texture; }
	virtual D3DCOLOR OnClear(IDirect3DDevice9*, DWORD, DWORD, D3DCOLOR color) { return color; }
};
