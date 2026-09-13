#pragma once

#include "D3D9/DeviceListener.h"

namespace FrameMeterHud
{
	IDeviceListener* Listener();

	bool IsVisible();
	void SetVisible(bool visible);
	void Toggle();

	bool FontFailed();
}
