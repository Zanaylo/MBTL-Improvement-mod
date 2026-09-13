#include "Palette/EffectPaint.h"

#include "Core/utils.h"
#include "Game/GameOffsets.h"
#include "Palette/PaletteControl.h"
#include "Palette/PaletteOwner.h"
#include "Palette/PalettePaint.h"

#include <cstring>

namespace {

namespace Palette = GameOffsets::Palette;

struct Entry
{
	uint8_t observed[3];
	bool seen;

	uint8_t edit[3];
	bool edited;

	uint8_t remote[3];
	bool hasRemote;

	uint8_t written[3];
	bool ours;
};

struct Player
{
	Entry entries[EffectPaint::kColours];

	uint8_t previewRgb[3];
	int previewExcept;
	uint8_t previewExceptRgb[3];
	bool previewing;

	uintptr_t owner;
	unsigned revision;
};

Player g_players[EffectPaint::kSeats] = {};

bool Valid(int player, int entry)
{
	return player >= 0 && player < EffectPaint::kSeats && entry > 0 && entry < EffectPaint::kColours;
}

bool ValidPlayer(int player)
{
	return player >= 0 && player < EffectPaint::kSeats;
}

const uint8_t* Wanted(const Player& player, int index, const uint8_t* worn)
{
	if (player.previewing)
		return index == player.previewExcept ? player.previewExceptRgb : player.previewRgb;

	const Entry& entry = player.entries[index];

	if (entry.hasRemote)
		return entry.remote;

	if (entry.edited)
		return entry.edit;

	return worn != nullptr ? worn + index * 4 : nullptr;
}

void ForgetOwner(Player& player, uintptr_t owner)
{
	player.owner = owner;

	for (Entry& entry : player.entries)
	{
		entry.seen = false;
		entry.ours = false;
	}
}

void Follow(int index)
{
	Player& player = g_players[index];
	PaletteOwner::Owner owner = {};

	if (!PaletteOwner::Read(index, owner))
	{
		if (player.owner != 0)
			ForgetOwner(player, 0);

		return;
	}

	if (owner.address != player.owner)
		ForgetOwner(player, owner.address);

	uint8_t buffer[EffectPaint::kBlockBytes] = {};
	const void* const where = reinterpret_cast<const void*>(owner.address + Palette::kOwnerEffectColours);

	if (!TryReadMemory(buffer, where, sizeof(buffer)))
		return;

	const bool allowed = PaletteControl::CanWear(index);
	const uint8_t* const worn = PalettePaint::GetWorn(index, 0);
	bool dirty = false;

	for (int i = 1; i < EffectPaint::kColours; ++i)
	{
		Entry& entry = player.entries[i];
		uint8_t* const now = buffer + i * 4;
		const bool stillOurs = entry.ours && std::memcmp(now, entry.written, 3) == 0;

		if (!stillOurs)
		{
			std::memcpy(entry.observed, now, 3);
			entry.seen = true;
		}

		const uint8_t* const rgb = allowed ? Wanted(player, i, worn) : nullptr;
		const uint8_t* const target = rgb != nullptr ? rgb : (stillOurs ? entry.observed : nullptr);

		entry.ours = rgb != nullptr;

		if (target == nullptr)
			continue;

		std::memcpy(entry.written, target, 3);

		if (std::memcmp(now, target, 3) == 0)
			continue;

		std::memcpy(now, target, 3);
		dirty = true;
	}

	if (dirty)
		TryWriteMemory(reinterpret_cast<void*>(owner.address + Palette::kOwnerEffectColours), buffer, sizeof(buffer));
}

}

bool EffectPaint::GetObserved(int player, int entry, uint8_t* outRgb)
{
	if (!Valid(player, entry) || !g_players[player].entries[entry].seen)
		return false;

	if (outRgb != nullptr)
		std::memcpy(outRgb, g_players[player].entries[entry].observed, 3);

	return true;
}

void EffectPaint::SetEntry(int player, int entry, const uint8_t* rgb)
{
	if (!Valid(player, entry) || rgb == nullptr)
		return;

	std::memcpy(g_players[player].entries[entry].edit, rgb, 3);
	g_players[player].entries[entry].edited = true;
	++g_players[player].revision;
}

void EffectPaint::ClearEntry(int player, int entry)
{
	if (!Valid(player, entry))
		return;

	g_players[player].entries[entry].edited = false;
	++g_players[player].revision;
}

void EffectPaint::Clear(int player)
{
	if (!ValidPlayer(player))
		return;

	for (Entry& entry : g_players[player].entries)
		entry.edited = false;

	++g_players[player].revision;
}

void EffectPaint::GetBlock(int player, uint8_t* block)
{
	if (!ValidPlayer(player) || block == nullptr)
		return;

	std::memset(block, 0, kBlockBytes);

	for (int entry = 1; entry < kColours; ++entry)
	{
		if (!g_players[player].entries[entry].edited)
			continue;

		std::memcpy(block + entry * 4, g_players[player].entries[entry].edit, 3);
		block[entry * 4 + 3] = 255;
	}
}

void EffectPaint::SetBlock(int player, const uint8_t* block)
{
	if (!ValidPlayer(player))
		return;

	for (int entry = 1; entry < kColours; ++entry)
	{
		const bool set = block != nullptr && block[entry * 4 + 3] == 255;

		g_players[player].entries[entry].edited = set;

		if (set)
			std::memcpy(g_players[player].entries[entry].edit, block + entry * 4, 3);
	}

	++g_players[player].revision;
}

void EffectPaint::SetRemote(int player, const uint8_t* block)
{
	if (!ValidPlayer(player))
		return;

	for (int entry = 1; entry < kColours; ++entry)
	{
		const bool set = block != nullptr && block[entry * 4 + 3] == 255;

		g_players[player].entries[entry].hasRemote = set;

		if (set)
			std::memcpy(g_players[player].entries[entry].remote, block + entry * 4, 3);
	}
}

void EffectPaint::ClearRemote(int player)
{
	if (!ValidPlayer(player))
		return;

	for (Entry& entry : g_players[player].entries)
		entry.hasRemote = false;
}

bool EffectPaint::HasRemote(int player)
{
	if (!ValidPlayer(player))
		return false;

	for (const Entry& entry : g_players[player].entries)
	{
		if (entry.hasRemote)
			return true;
	}

	return false;
}

bool EffectPaint::GetRemoteEntry(int player, int entry, uint8_t* outRgb)
{
	if (!Valid(player, entry) || !g_players[player].entries[entry].hasRemote)
		return false;

	if (outRgb != nullptr)
		std::memcpy(outRgb, g_players[player].entries[entry].remote, 3);

	return true;
}

unsigned EffectPaint::GetRevision(int player)
{
	return ValidPlayer(player) ? g_players[player].revision : 0;
}

bool EffectPaint::IsEdited(int player, int entry)
{
	return Valid(player, entry) && g_players[player].entries[entry].edited;
}

bool EffectPaint::GetEdit(int player, int entry, uint8_t* outRgb)
{
	if (!IsEdited(player, entry) || outRgb == nullptr)
		return false;

	std::memcpy(outRgb, g_players[player].entries[entry].edit, 3);
	return true;
}

int EffectPaint::GetEditedCount(int player)
{
	if (!ValidPlayer(player))
		return 0;

	int count = 0;

	for (const Entry& entry : g_players[player].entries)
		count += entry.edited ? 1 : 0;

	return count;
}

void EffectPaint::PreviewObserved(int player, const uint8_t* rgb, int except, const uint8_t* exceptRgb)
{
	if (!ValidPlayer(player) || rgb == nullptr)
		return;

	Player& entry = g_players[player];

	std::memcpy(entry.previewRgb, rgb, 3);
	std::memcpy(entry.previewExceptRgb, exceptRgb != nullptr ? exceptRgb : rgb, 3);
	entry.previewExcept = except;
	entry.previewing = true;
}

void EffectPaint::EndPreview(int player)
{
	if (ValidPlayer(player))
		g_players[player].previewing = false;
}

void EffectPaint::OnFrame()
{
	for (int player = 0; player < kSeats; ++player)
		Follow(player);
}
