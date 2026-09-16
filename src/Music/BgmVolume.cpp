#include "Music/BgmVolume.h"

#include "Core/logger.h"
#include "Hooks/HookManager.h"
#include "Music/BgmTable.h"
#include "Music/MusicIni.h"

#include <cstdlib>
#include <string>
#include <vector>

namespace {

namespace Music = GameOffsets::Music;

using SetBgmVolume_t = void(__fastcall*)(int, int);

constexpr const char* kSection = "Volume";
constexpr int kEngineStep = Music::kFullVolume / BgmVolume::kFullPercent;

SetBgmVolume_t oSetBgmVolume = nullptr;
bool g_hooked = false;

volatile long g_percent[Music::kSlotCount] = {};
int g_original[Music::kSlotCount] = {};
bool g_written[Music::kSlotCount] = {};

int Clamp(int percent)
{
	if (percent < 0)
		return 0;

	if (percent > BgmVolume::kFullPercent)
		return BgmVolume::kFullPercent;

	return percent;
}

int SlotVolume(int id)
{
	int value = Music::kFullVolume;

	if (!BgmTable::ReadVolume(id, value))
		return Music::kFullVolume;

	return value;
}

void __fastcall HookedSetBgmVolume(int base, int track)
{
	const int id = BgmTable::CurrentId();

	if (track != Music::kFullVolume || !BgmTable::IsValidId(id))
	{
		oSetBgmVolume(base, track);
		return;
	}

	const int wanted = SlotVolume(id);

	if (wanted == Music::kFullVolume)
	{
		oSetBgmVolume(base, track);
		return;
	}

	BgmTable::SetTrackVolume(wanted);
	oSetBgmVolume(base, wanted);
}

}

void BgmVolume::Load()
{
	for (int id = 0; id < Music::kSlotCount; ++id)
		g_percent[id] = kFullPercent;

	for (const MusicIni::Entry& entry : MusicIni::ReadSection(kSection))
	{
		const int id = atoi(entry.first.c_str());

		if (!BgmTable::IsValidId(id))
			continue;

		g_percent[id] = Clamp(atoi(entry.second.c_str()));
	}

	LOG("BgmVolume: %d track(s) with a volume of their own", CustomCount());
}

void BgmVolume::Install(uint8_t* setVolume)
{
	if (setVolume == nullptr)
	{
		LOG("BgmVolume: SetBgmVolume was not found, so a volume set in the options brings a quieter track back to "
			"full until the next track");
		return;
	}

	g_hooked = HookManager::CreateHook(setVolume, reinterpret_cast<void*>(&HookedSetBgmVolume),
		reinterpret_cast<void**>(&oSetBgmVolume), "SetBgmVolume");

	if (g_hooked)
		return;

	oSetBgmVolume = reinterpret_cast<SetBgmVolume_t>(setVolume);
	LOG("BgmVolume: SetBgmVolume could not be hooked, so a volume set in the options resets a quieter track");
}

bool BgmVolume::IsHooked()
{
	return g_hooked;
}

int BgmVolume::Percent(int id)
{
	if (!BgmTable::IsValidId(id))
		return kFullPercent;

	return static_cast<int>(g_percent[id]);
}

void BgmVolume::SetPercent(int id, int percent)
{
	if (!BgmTable::IsValidId(id))
		return;

	g_percent[id] = Clamp(percent);
}

void BgmVolume::ResetAll()
{
	for (int id = 0; id < Music::kSlotCount; ++id)
		g_percent[id] = kFullPercent;

	Save();
}

void BgmVolume::Save()
{
	std::vector<MusicIni::Entry> entries;

	for (int id = 0; id < Music::kSlotCount; ++id)
	{
		if (g_percent[id] == kFullPercent)
			continue;

		entries.emplace_back(MusicIni::IdKey(id), std::to_string(g_percent[id]));
	}

	MusicIni::WriteSection(kSection, entries);
}

int BgmVolume::CustomCount()
{
	int custom = 0;

	for (int id = 0; id < Music::kSlotCount; ++id)
		custom += g_percent[id] == kFullPercent ? 0 : 1;

	return custom;
}

void BgmVolume::ApplyToSlot(int id)
{
	if (!BgmTable::IsValidId(id))
		return;

	const int percent = static_cast<int>(g_percent[id]);

	if (percent == kFullPercent && !g_written[id])
		return;

	if (percent == kFullPercent)
	{
		BgmTable::WriteVolume(id, g_original[id]);
		g_written[id] = false;
		return;
	}

	if (!g_written[id])
	{
		g_original[id] = SlotVolume(id);
		g_written[id] = true;
	}

	BgmTable::WriteVolume(id, percent * kEngineStep);
}

void BgmVolume::ApplyNow()
{
	const int id = BgmTable::CurrentId();

	if (!BgmTable::IsValidId(id) || oSetBgmVolume == nullptr)
		return;

	ApplyToSlot(id);

	const int wanted = SlotVolume(id);

	BgmTable::SetTrackVolume(wanted);
	oSetBgmVolume(BgmTable::BaseVolume(), wanted);
}
