#pragma once

#include <d3d9.h>

namespace D3D9Hooks
{
	bool Install();
	void OnDirect3D9Created(IDirect3D9* d3d9);
}
