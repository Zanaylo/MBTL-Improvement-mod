#include "Core/Hotkeys.h"

#include "Core/KeyState.h"
#include "Core/Settings.h"
#include "Core/interfaces.h"

#include <cstring>

namespace {

constexpr const char* kSection = "Keybinds";
constexpr const char* kFunctionKey = "FunctionKey";

struct Entry
{
	const char* label;
	const char* key;
	KeyBind* bind;
};

const Entry kEntries[Hotkeys::Action_Count] = {
	{ "Open this window", "ToggleOverlay", &g_settings.toggleOverlayKey },
	{ "Hitbox viewer", "ToggleHitboxOverlay", &g_settings.toggleHitboxKey },
	{ "Frame meter", "ToggleFrameMeter", &g_settings.toggleFrameMeterKey },
	{ "Pause and resume", "FreezeFrame", &g_settings.freezeFrameKey },
	{ "Next frame", "StepForward", &g_settings.stepForwardKey },
	{ "Previous palette", "PreviousPalette", &g_settings.previousPaletteKey },
	{ "Next palette", "NextPalette", &g_settings.nextPaletteKey },
	{ "Debug window", "ToggleDebug", &g_settings.toggleDebugKey },
	{ "Restart the game", "RestartGame", &g_settings.restartGameKey },
	{ "Hide the HUD", "HideHud", &g_settings.hideHudKey },
	{ "Hide the characters", "HideCharacters", &g_settings.hideCharactersKey },
};

const KeyBind kNoBind;

bool Valid(Hotkeys::Action action)
{
	return action >= 0 && action < Hotkeys::Action_Count;
}

bool FunctionHeld()
{
	return KeyState::Held(g_settings.functionKey.key);
}

bool GatePasses(const KeyBind& bind)
{
	return bind.key != 0 && bind.function == FunctionHeld();
}

}

const char* Hotkeys::Label(Action action)
{
	return Valid(action) ? kEntries[action].label : "";
}

const KeyBind& Hotkeys::Bind(Action action)
{
	return Valid(action) ? *kEntries[action].bind : kNoBind;
}

void Hotkeys::SetBind(Action action, const KeyBind& bind)
{
	if (!Valid(action))
		return;

	*kEntries[action].bind = bind;
	Settings::SaveKey(kSection, kEntries[action].key, bind);
}

const KeyBind& Hotkeys::FunctionKey()
{
	return g_settings.functionKey;
}

void Hotkeys::SetFunctionKey(int virtualKey)
{
	g_settings.functionKey.key = virtualKey;
	g_settings.functionKey.function = false;
	Settings::SaveKey(kSection, kFunctionKey, g_settings.functionKey);
}

bool Hotkeys::Pressed(Action action)
{
	const KeyBind& bind = Bind(action);
	return GatePasses(bind) && KeyState::Pressed(bind.key);
}

bool Hotkeys::Repeating(Action action, unsigned delayMs, unsigned intervalMs)
{
	const KeyBind& bind = Bind(action);
	return GatePasses(bind) && KeyState::Repeating(bind.key, delayMs, intervalMs);
}

const char* Hotkeys::Describe(Action action)
{
	static char text[32] = {};

	const std::string formatted = KeyBinds::Format(Bind(action));
	strncpy_s(text, formatted.empty() ? "unbound" : formatted.c_str(), _TRUNCATE);
	return text;
}
