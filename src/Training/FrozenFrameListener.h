#pragma once

#include "D3D9/DeviceListener.h"

class FrozenFrameListener : public IDeviceListener
{
public:
	void OnDeviceReady(IDirect3DDevice9* device, const D3DPRESENT_PARAMETERS& parameters) override;
	void OnDeviceLost() override;
	void OnPresent(IDirect3DDevice9* device) override;
	Profiler::Section ProfileSection() const override { return Profiler::Section_PresentFrozenFrame; }
};
