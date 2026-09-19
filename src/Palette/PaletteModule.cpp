#include "Palette/PaletteModule.h"

#include "D3D9/DeviceHooks.h"
#include "Network/PaletteShare.h"
#include "Palette/EffectPaint.h"
#include "Palette/PaletteChoice.h"
#include "Palette/PaletteControl.h"
#include "Palette/PalettePaint.h"

namespace {

class PaletteListener final : public IDeviceListener
{
public:
	void OnPresent(IDirect3DDevice9*) override
	{
		PaletteControl::OnFrame();
		PaletteChoice::OnFrame();
		PaletteShare::OnFrame();
		PalettePaint::OnFrame();
		EffectPaint::OnFrame();
	}
};

PaletteListener g_listener;

}

void PaletteModule::Install()
{
	DeviceHooks::AddListener(&g_listener);
}
