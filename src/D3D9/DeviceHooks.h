#pragma once

#include "D3D9/DeviceListener.h"
#include "D3D9/DrawFilter.h"

#include <d3d9.h>

namespace DeviceHooks
{
	void AddListener(IDeviceListener* listener);
	void AddFilter(IDrawFilter* filter);

	bool Install(IDirect3DDevice9* device, const D3DPRESENT_PARAMETERS& parameters, HWND window);
	bool IsInstalled();

	IDirect3DDevice9* Device();
	HWND Window();
	const D3DPRESENT_PARAMETERS& GetPresentParameters();

	unsigned long PresentCount();
	float OverlayScale();
}
