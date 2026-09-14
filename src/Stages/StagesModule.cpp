#include "Stages/StagesModule.h"

#include "Core/logger.h"
#include "D3D9/DeviceHooks.h"
#include "Overlay/Debug/DebugSection.h"
#include "Stages/CharacterLight.h"
#include "Stages/HiddenStages.h"
#include "Stages/StageBloom.h"
#include "Stages/StageCards.h"
#include "Stages/StageImport.h"
#include "Stages/StageLibrary.h"
#include "Stages/StageOverlays.h"
#include "Stages/StageThumbs.h"
#include "Stages/StagesDebugSection.h"

namespace {

class FinishListener : public IDeviceListener
{
public:
	void OnPresent(IDirect3DDevice9*) override { StageImport::Update(); }
};

FinishListener g_listener;
StagesDebugSection g_debug;

}

void StagesModule::Install()
{
	HiddenStages::Load();
	StageCards::Install();
	StageLibrary::Load();
	StageOverlays::Register();
	StageThumbs::Register();
	StageBloom::Install();
	CharacterLight::Install();

	DeviceHooks::AddListener(&g_listener);
	DebugSections::Add(&g_debug);

	LOG("StagesModule: %d stage(s) installed, the stage list is extended when the game reads it", StageLibrary::Count());
}
