#include "Music/BgmShuffle.h"

#include "Core/logger.h"
#include "Music/BgmTable.h"
#include "Music/MusicIni.h"

#include <windows.h>

#include <cstdint>
#include <cstdlib>
#include <vector>

namespace {

namespace Music = GameOffsets::Music;

constexpr const char* kSection = "Randomizer";
constexpr const char* kTracksSection = "RandomizerTracks";
constexpr const char* kEnabledKey = "Enabled";
constexpr int8_t kUnset = -1;
constexpr int8_t kOut = 0;
constexpr int8_t kIn = 1;

volatile long g_enabled = 0;
volatile long g_drawnFor = BgmTable::kNoTrack;
volatile long g_drawn = BgmTable::kNoTrack;
int8_t g_inDraw[Music::kSlotCount] = {};
uint32_t g_seed = 0;

uint32_t NextRandom()
{
	if (g_seed == 0)
		g_seed = GetTickCount() | 1u;

	g_seed ^= g_seed << 13;
	g_seed ^= g_seed >> 17;
	g_seed ^= g_seed << 5;
	return g_seed;
}

void SaveTracks()
{
	std::vector<MusicIni::Entry> entries;

	for (int id = 0; id < Music::kSlotCount; ++id)
	{
		if (g_inDraw[id] == kUnset)
			continue;

		entries.emplace_back(MusicIni::IdKey(id), g_inDraw[id] == kIn ? "1" : "0");
	}

	MusicIni::WriteSection(kTracksSection, entries);
}

}

void BgmShuffle::Load()
{
	g_enabled = MusicIni::ReadInt(kSection, kEnabledKey, 0) != 0 ? 1 : 0;

	for (int id = 0; id < Music::kSlotCount; ++id)
		g_inDraw[id] = kUnset;

	for (const MusicIni::Entry& entry : MusicIni::ReadSection(kTracksSection))
	{
		const int id = atoi(entry.first.c_str());

		if (!BgmTable::IsValidId(id))
			continue;

		g_inDraw[id] = atoi(entry.second.c_str()) != 0 ? kIn : kOut;
	}
}

bool BgmShuffle::IsEnabled()
{
	return g_enabled != 0;
}

void BgmShuffle::SetEnabled(bool enabled)
{
	g_enabled = enabled ? 1 : 0;
	Redraw();
	MusicIni::WriteInt(kSection, kEnabledKey, enabled ? 1 : 0);
}

bool BgmShuffle::IsInDraw(int id, bool loops)
{
	if (!BgmTable::IsValidId(id))
		return false;

	return g_inDraw[id] == kUnset ? loops : g_inDraw[id] == kIn;
}

void BgmShuffle::SetInDraw(int id, bool inDraw)
{
	if (!BgmTable::IsValidId(id))
		return;

	g_inDraw[id] = inDraw ? kIn : kOut;
	SaveTracks();
}

void BgmShuffle::SetAllInDraw(bool inDraw)
{
	for (int id = 0; id < Music::kSlotCount; ++id)
		g_inDraw[id] = inDraw ? kIn : kOut;

	SaveTracks();
}

int BgmShuffle::Pick(int asked)
{
	if (g_enabled == 0)
		return BgmTable::kNoTrack;

	const int previous = static_cast<int>(g_drawn);

	if (asked == g_drawnFor && BgmTable::IsPresent(previous))
		return previous;

	int pool[Music::kSlotCount] = {};
	int size = 0;

	for (int id = 0; id < Music::kSlotCount; ++id)
	{
		BgmTable::Slot slot = {};

		if (!BgmTable::Read(id, slot) || !slot.present || !IsInDraw(id, slot.loop))
			continue;

		pool[size++] = id;
	}

	if (size == 0)
		return BgmTable::kNoTrack;

	int index = static_cast<int>(NextRandom() % static_cast<uint32_t>(size));

	if (size > 1 && pool[index] == previous)
		index = (index + 1) % size;

	InterlockedExchange(&g_drawnFor, asked);
	InterlockedExchange(&g_drawn, pool[index]);

	LOG("BgmShuffle: drew %d over %d from %d track(s)", pool[index], asked, size);
	return pool[index];
}

void BgmShuffle::Redraw()
{
	InterlockedExchange(&g_drawnFor, BgmTable::kNoTrack);
}
