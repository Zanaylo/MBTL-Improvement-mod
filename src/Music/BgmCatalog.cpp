#include "Music/BgmCatalog.h"

#include "Music/BgmNames.h"
#include "Music/BgmTable.h"

#include <windows.h>

#include <cstring>

namespace {

namespace Music = GameOffsets::Music;

constexpr DWORD kRefreshMs = 1000;

BgmCatalog::Track g_tracks[Music::kSlotCount] = {};
int g_count = 0;
DWORD g_refreshedAt = 0;
bool g_stale = true;

}

void BgmCatalog::Refresh()
{
	const DWORD now = GetTickCount();

	if (!g_stale && now - g_refreshedAt < kRefreshMs)
		return;

	g_stale = false;
	g_refreshedAt = now;
	g_count = 0;

	for (int id = 0; id < Music::kSlotCount; ++id)
	{
		BgmTable::Slot slot = {};

		if (!BgmTable::Read(id, slot) || !slot.present)
			continue;

		Track& track = g_tracks[g_count++];

		track.id = id;
		track.loop = slot.loop;
		track.loopPosition = slot.loopPosition;
		strncpy_s(track.file, slot.file, _TRUNCATE);
		BgmNames::Label(id, slot.file, track.name, sizeof(track.name));
	}
}

void BgmCatalog::Invalidate()
{
	g_stale = true;
}

int BgmCatalog::Count()
{
	return g_count;
}

const BgmCatalog::Track& BgmCatalog::At(int index)
{
	return g_tracks[index];
}

const BgmCatalog::Track* BgmCatalog::Find(int id)
{
	for (int i = 0; i < g_count; ++i)
	{
		if (g_tracks[i].id == id)
			return &g_tracks[i];
	}

	return nullptr;
}
