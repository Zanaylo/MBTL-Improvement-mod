#include "Network/NetworkModule.h"

#include "D3D9/DeviceHooks.h"
#include "Network/NetplayTick.h"
#include "Network/PaletteShare.h"

namespace {

class NetworkListener final : public IDeviceListener
{
public:
	void OnPresent(IDirect3DDevice9*) override
	{
		NetplayTick::Update();
	}
};

NetworkListener g_listener;

}

void NetworkModule::Install()
{
	PaletteShare::Initialize();
	NetplayTick::Install();

	DeviceHooks::AddListener(&g_listener);
}

void NetworkModule::Shutdown()
{
	NetplayTick::Shutdown();
}
