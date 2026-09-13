#include "Overlay/HotkeyActions.h"

#include "Core/Hotkeys.h"
#include "Core/interfaces.h"
#include "Game/GameRestart.h"
#include "Overlay/FrameMeterHud.h"
#include "Overlay/WindowContainer/WindowContainer.h"
#include "Palette/PaletteChoice.h"
#include "Training/BattleHud.h"
#include "Training/CharacterDraw.h"
#include "Training/FrameStepper.h"

namespace {

void RestartGame()
{
	GameRestart::SoftReset();
}

}

void HotkeyActions::Run(const WindowContainer& windows)
{
	if (Hotkeys::Pressed(Hotkeys::Action_ToggleDebug))
		windows.Toggle(WindowType_Debug);

	if (Hotkeys::Pressed(Hotkeys::Action_ToggleOverlay))
		windows.Toggle(WindowType_Main);

	if (Hotkeys::Pressed(Hotkeys::Action_ToggleHitbox))
		windows.Toggle(WindowType_HitboxOverlay);

	if (Hotkeys::Pressed(Hotkeys::Action_ToggleFrameMeter))
		FrameMeterHud::Toggle();

	if (Hotkeys::Pressed(Hotkeys::Action_FreezeFrame))
		FrameStepper::TogglePaused();

	if (Hotkeys::Pressed(Hotkeys::Action_NextPalette))
		PaletteChoice::Step(PaletteChoice::LocalPlayer(), 1);

	if (Hotkeys::Pressed(Hotkeys::Action_PreviousPalette))
		PaletteChoice::Step(PaletteChoice::LocalPlayer(), -1);

	if (Hotkeys::Pressed(Hotkeys::Action_RestartGame))
		RestartGame();

	if (Hotkeys::Pressed(Hotkeys::Action_HideHud))
		BattleHud::Toggle();

	if (Hotkeys::Pressed(Hotkeys::Action_HideCharacters))
		CharacterDraw::Toggle();

	const unsigned delay = static_cast<unsigned>(g_settings.stepRepeatDelayMs);
	const unsigned interval = static_cast<unsigned>(g_settings.stepRepeatIntervalMs);

	if (Hotkeys::Repeating(Hotkeys::Action_StepForward, delay, interval))
		FrameStepper::RequestStep(1);
}
