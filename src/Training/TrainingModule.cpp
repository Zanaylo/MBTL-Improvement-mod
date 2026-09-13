#include "Training/TrainingModule.h"

#include "D3D9/DeviceHooks.h"
#include "Overlay/Debug/DebugSection.h"
#include "Training/BattleHud.h"
#include "Training/BattleMap.h"
#include "Training/CharacterDraw.h"
#include "Training/FrameMeter.h"
#include "Training/FrameStepper.h"
#include "Training/FrozenFrameListener.h"
#include "Training/TrainingDebugSection.h"

namespace {

FrozenFrameListener g_frozenFrame;
TrainingDebugSection g_debug;

}

void TrainingModule::Install()
{
	BattleMap::Initialize();
	FrameStepper::Initialize();
	FrameStepper::AddTickListener(FrameMeter::Listener());
	BattleHud::Install();
	CharacterDraw::Install();

	DeviceHooks::AddListener(&g_frozenFrame);
	DebugSections::Add(&g_debug);
}
