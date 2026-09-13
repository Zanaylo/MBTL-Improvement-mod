#include "Stages/HiddenStages.h"

#include "Core/Settings.h"
#include "Core/logger.h"
#include "Stages/StageLibrary.h"
#include "Stages/StageRevision.h"

#include <windows.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace {

constexpr const char* kKey = "Unlocked";
constexpr DWORD kValueBytes = 512;

SRWLOCK g_lock = SRWLOCK_INIT;
std::vector<int> g_unlocked;

bool Holds(int number)
{
	return std::find(g_unlocked.begin(), g_unlocked.end(), number) != g_unlocked.end();
}

void Save()
{
	std::string text;

	for (int number : g_unlocked)
	{
		char item[16] = {};
		sprintf_s(item, "%s%d", text.empty() ? "" : ",", number);
		text += item;
	}

	Settings::SaveString(StageLibrary::kSection, kKey, text.empty() ? nullptr : text.c_str());
}

}

void HiddenStages::Load()
{
	char value[kValueBytes] = {};
	GetPrivateProfileStringA(StageLibrary::kSection, kKey, "", value, kValueBytes, Settings::IniPath().c_str());

	std::vector<int> unlocked;

	for (const char* at = value; *at != '\0'; ++at)
	{
		if (isdigit(static_cast<unsigned char>(*at)) == 0)
			continue;

		unlocked.push_back(atoi(at));

		while (isdigit(static_cast<unsigned char>(at[1])) != 0)
			++at;
	}

	AcquireSRWLockExclusive(&g_lock);
	g_unlocked.swap(unlocked);
	ReleaseSRWLockExclusive(&g_lock);
}

bool HiddenStages::Unlocked(int number)
{
	AcquireSRWLockShared(&g_lock);
	const bool unlocked = Holds(number);
	ReleaseSRWLockShared(&g_lock);

	return unlocked;
}

void HiddenStages::SetUnlocked(int number, bool unlocked)
{
	AcquireSRWLockExclusive(&g_lock);

	const bool changed = Holds(number) != unlocked;

	if (changed && unlocked)
		g_unlocked.push_back(number);

	if (changed && !unlocked)
		g_unlocked.erase(std::remove(g_unlocked.begin(), g_unlocked.end(), number), g_unlocked.end());

	if (changed)
		Save();

	ReleaseSRWLockExclusive(&g_lock);

	if (!changed)
		return;

	StageRevision::Bump();
	LOG("HiddenStages: stage %d is %s", number, unlocked ? "unlocked" : "hidden again");
}

void HiddenStages::Snapshot(std::vector<int>& out)
{
	AcquireSRWLockShared(&g_lock);
	out = g_unlocked;
	ReleaseSRWLockShared(&g_lock);
}
