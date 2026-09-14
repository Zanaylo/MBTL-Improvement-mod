#include "Performance/PotatoMode.h"

#include "Core/Settings.h"
#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Performance/EngineQuality.h"
#include "Performance/PresentSize.h"
#include "Performance/PumpWait.h"

namespace {

constexpr const char* kGraphics = "Graphics";

struct Preset
{
	const char* name;
	const char* description;
	int presentWidth;
	int presentHeight;
	bool sizeFromPotatoHeight;
	bool disableBackBufferAa;
	bool pumpWait;
	bool plainStage;
};

constexpr Preset kPresets[PotatoMode::Level_COUNT] = {
	{
		"Off",
		"No changes. The game draws at its own display setting.",
		0, 0, false, false, false, false,
	},
	{
		"Balanced",
		"Draws at 960x540 and stretches it up, turns off back buffer multisampling and uses the precise frame "
		"pause. In exclusive fullscreen the nearest monitor mode is used. Slightly soft.",
		960, 540, false, true, true, false,
	},
	{
		"Potato",
		"Draws at the size you pick below and stretches it up, and turns off the stage's multisampling and FXAA. "
		"Clearly soft, but the stage is still drawn.",
		0, 0, true, true, true, true,
	},
};

struct Snapshot
{
	bool disableBackBufferAa;
	bool pumpWait;
	bool plainStage;
	bool taken;
};

Snapshot g_before = {};

int ClampLevel(int level)
{
	if (level < PotatoMode::Level_Off)
		return PotatoMode::Level_Off;

	if (level >= PotatoMode::Level_COUNT)
		return PotatoMode::Level_COUNT - 1;

	return level;
}

void TakeSnapshot()
{
	if (g_before.taken)
		return;

	g_before.disableBackBufferAa = g_settings.disableBackBufferAa;
	g_before.pumpWait = g_settings.pumpWait;
	g_before.plainStage = g_settings.plainStage;
	g_before.taken = true;
}

void Adopt(bool disableBackBufferAa, bool pumpWait, bool plainStage)
{
	g_settings.disableBackBufferAa = disableBackBufferAa;
	g_settings.pumpWait = pumpWait;
	g_settings.plainStage = plainStage;
}

void Adopt(const Preset& preset)
{
	Adopt(preset.disableBackBufferAa, preset.pumpWait, preset.plainStage);
}

void RestoreOrShip()
{
	const bool taken = g_before.taken;
	g_before.taken = false;

	if (taken)
	{
		Adopt(g_before.disableBackBufferAa, g_before.pumpWait, g_before.plainStage);
		return;
	}

	Adopt(kPresets[PotatoMode::Level_Off]);
}

void Save()
{
	Settings::SaveInt(kGraphics, "PotatoMode", g_settings.potatoMode);
	Settings::SaveInt(kGraphics, "PotatoHeight", g_settings.potatoHeight);
	Settings::SaveInt(kGraphics, "PresentWidth", g_settings.presentWidth);
	Settings::SaveInt(kGraphics, "PresentHeight", g_settings.presentHeight);
	Settings::SaveBool(kGraphics, "DisableBackBufferAA", g_settings.disableBackBufferAa);
	Settings::SaveBool(kGraphics, "PlainStage", g_settings.plainStage);
	Settings::SaveBool("Video", "PumpWait", g_settings.pumpWait);
}

}

void PotatoMode::Apply(int level)
{
	const int clamped = ClampLevel(level);

	if (clamped == Level_Off)
		RestoreOrShip();
	else
	{
		TakeSnapshot();
		Adopt(kPresets[clamped]);
	}

	g_settings.potatoMode = clamped;

	PresentSize::Refresh();
	Save();

	PumpWait::Apply();
	EngineQuality::Apply();

	LOG("PotatoMode: %s", kPresets[clamped].name);
}

void PotatoMode::ApplySaved()
{
	const int level = GetLevel();

	if (level != Level_Off)
		Adopt(kPresets[level]);

	PresentSize::Refresh();
}

void PotatoMode::OnFrame()
{
	EngineQuality::OnFrame();
}

int PotatoMode::ClampHeight(int height)
{
	for (const int candidate : kHeights)
	{
		if (candidate == height)
			return height;
	}

	return 360;
}

void PotatoMode::SizeForHeight(int height, int& outWidth, int& outHeight)
{
	outHeight = ClampHeight(height);
	outWidth = (outHeight * 16 / 9) & ~1;
}

int PotatoMode::GetHeight()
{
	return ClampHeight(g_settings.potatoHeight);
}

void PotatoMode::SetHeight(int height)
{
	g_settings.potatoHeight = ClampHeight(height);

	if (GetLevel() != Level_Potato)
	{
		Settings::SaveInt(kGraphics, "PotatoHeight", g_settings.potatoHeight);
		return;
	}

	Apply(Level_Potato);
}

bool PotatoMode::GetPresentSize(int& outWidth, int& outHeight)
{
	const Preset& preset = kPresets[GetLevel()];

	if (preset.sizeFromPotatoHeight)
	{
		SizeForHeight(GetHeight(), outWidth, outHeight);
		return true;
	}

	outWidth = preset.presentWidth;
	outHeight = preset.presentHeight;

	return preset.presentWidth > 0 && preset.presentHeight > 0;
}

int PotatoMode::GetLevel()
{
	return ClampLevel(g_settings.potatoMode);
}

bool PotatoMode::IsActive()
{
	return GetLevel() != Level_Off;
}

const char* PotatoMode::GetLevelName(int level)
{
	return kPresets[ClampLevel(level)].name;
}

const char* PotatoMode::Describe(int level)
{
	return kPresets[ClampLevel(level)].description;
}
