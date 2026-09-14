#include "Performance/Improvements.h"

#include "Core/Settings.h"
#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Performance/PresentSize.h"

namespace {

struct Step
{
	const char* name;
	const char* description;
	int width;
	int height;
};

constexpr Step kSteps[Improvements::Level_COUNT] = {
	{
		"Off",
		"Uses the game's own display setting.",
		0, 0,
	},
	{
		"1080p",
		"1920x1080. Best for a 1080p screen: one pixel per screen pixel, and the overlay stays sharp.",
		1920, 1080,
	},
	{
		"1440p",
		"2560x1440, shrunk to fit your window. Sharper HUD and menus. On a 1080p window the overlay "
		"gets a little soft.",
		2560, 1440,
	},
	{
		"4K",
		"3840x2160. The sharpest, and the most GPU work. On a 1080p window it shrinks exactly 2:1, "
		"the cleanest result.",
		3840, 2160,
	},
};

int ClampLevel(int level)
{
	if (level < Improvements::Level_Off)
		return Improvements::Level_Off;

	if (level >= Improvements::Level_COUNT)
		return Improvements::Level_COUNT - 1;

	return level;
}

}

void Improvements::Apply(int level)
{
	g_settings.supersample = ClampLevel(level);

	Settings::SaveInt("Graphics", "Supersample", g_settings.supersample);
	PresentSize::Refresh();

	LOG("improvements %s", kSteps[g_settings.supersample].name);
}

int Improvements::GetLevel()
{
	return ClampLevel(g_settings.supersample);
}

bool Improvements::GetPresentSize(int& outWidth, int& outHeight)
{
	const Step& step = kSteps[GetLevel()];

	outWidth = step.width;
	outHeight = step.height;

	return step.width > 0 && step.height > 0;
}

const char* Improvements::GetLevelName(int level)
{
	return kSteps[ClampLevel(level)].name;
}

const char* Improvements::Describe(int level)
{
	return kSteps[ClampLevel(level)].description;
}
