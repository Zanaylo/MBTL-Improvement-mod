#pragma once

#include "Core/KeyBind.h"

namespace Hotkeys
{
	enum Action
	{
		Action_ToggleOverlay,
		Action_ToggleHitbox,
		Action_ToggleFrameMeter,
		Action_FreezeFrame,
		Action_StepForward,
		Action_PreviousPalette,
		Action_NextPalette,
		Action_ToggleDebug,
		Action_RestartGame,
		Action_HideHud,
		Action_HideCharacters,
		Action_Count,
	};

	const char* Label(Action action);
	const KeyBind& Bind(Action action);
	void SetBind(Action action, const KeyBind& bind);

	const KeyBind& FunctionKey();
	void SetFunctionKey(int virtualKey);

	bool Pressed(Action action);
	bool Repeating(Action action, unsigned delayMs, unsigned intervalMs);

	const char* Describe(Action action);
}
