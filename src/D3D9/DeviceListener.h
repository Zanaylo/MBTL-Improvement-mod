#pragma once

#include "Core/Profiler.h"

#include <d3d9.h>

class IDeviceListener
{
public:
	virtual ~IDeviceListener() = default;

	virtual void OnDeviceReady(IDirect3DDevice9*, const D3DPRESENT_PARAMETERS&) {}
	virtual void OnDeviceLost() {}
	virtual void OnPresent(IDirect3DDevice9*) {}

	virtual Profiler::Section ProfileSection() const { return Profiler::Section_PresentTasks; }
};
