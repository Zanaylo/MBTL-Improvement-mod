#include "Palette/PalettePaint.h"

#include "Core/logger.h"
#include "Palette/PaletteControl.h"
#include "Palette/PaletteOwner.h"
#include "Palette/PaletteTexture.h"

#include <cstring>

namespace {

struct Layer
{
	uint8_t pages[PalettePaint::kSubs][PalettePaint::kBytes];
	uint8_t mask;
};

struct Surface
{
	IDirect3DTexture9* texture;
	bool known;
	bool failed;
	uint32_t writtenMask;
	uint8_t pristine[PaletteTexture::kBytes];
	uint8_t written[PaletteTexture::kBytes];
};

struct Player
{
	Layer staged;
	Layer preview;
	Layer remote;

	uintptr_t owner;
	Surface own;
	Surface shared;

	unsigned revision;
};

Player g_players[PalettePaint::kSeats] = {};

bool Valid(int player, int sub)
{
	return player >= 0 && player < PalettePaint::kSeats && sub >= 0 && sub < PalettePaint::kSubs;
}

uint8_t Bit(int index)
{
	return static_cast<uint8_t>(1u << index);
}

const uint8_t* PageOf(const Layer& layer, int sub)
{
	return (layer.mask & Bit(sub)) != 0 ? layer.pages[sub] : nullptr;
}

const uint8_t* SourceFor(const Player& entry, int sub)
{
	const uint8_t* const preview = PageOf(entry.preview, sub);

	if (preview != nullptr)
		return preview;

	const uint8_t* const remote = PageOf(entry.remote, sub);
	return remote != nullptr ? remote : PageOf(entry.staged, sub);
}

void Forget(Surface& surface, IDirect3DTexture9* texture)
{
	std::memset(&surface, 0, sizeof(surface));
	surface.texture = texture;
}

void Blend(const uint8_t* source, const uint8_t* pristine, uint8_t* target)
{
	std::memcpy(target, pristine, 4);

	for (int i = 1; i < PalettePaint::kColours; ++i)
	{
		std::memcpy(target + i * 4, source + i * 4, 3);
		target[i * 4 + 3] = pristine[i * 4 + 3];
	}
}

void Sync(Surface& surface, IDirect3DTexture9* texture, const Player& entry, bool allowed)
{
	if (texture != surface.texture)
		Forget(surface, texture);

	static uint8_t current[PaletteTexture::kBytes];
	static uint8_t wanted[PaletteTexture::kBytes];

	if (texture == nullptr)
		return;

	if (!PaletteTexture::Read(texture, current))
	{
		if (!surface.failed)
			LOG("palette paint: the palette texture at 0x%p could not be read", texture);

		surface.failed = true;
		return;
	}

	std::memcpy(wanted, current, sizeof(wanted));

	uint32_t changed = 0;
	uint32_t ours = 0;

	for (int row = 0; row < PaletteTexture::kRows; ++row)
	{
		const size_t at = static_cast<size_t>(row) * PaletteTexture::kRowBytes;
		const bool stillOurs = (surface.writtenMask & (1u << row)) != 0 &&
			std::memcmp(current + at, surface.written + at, PaletteTexture::kRowBytes) == 0;

		if (!stillOurs)
			std::memcpy(surface.pristine + at, current + at, PaletteTexture::kRowBytes);

		const uint8_t* const source = allowed ? SourceFor(entry, row / 2) : nullptr;

		if (source == nullptr && !stillOurs)
			continue;

		if (source == nullptr)
			std::memcpy(wanted + at, surface.pristine + at, PaletteTexture::kRowBytes);
		else
			Blend(source, surface.pristine + at, wanted + at);

		if (std::memcmp(wanted + at, current + at, PaletteTexture::kRowBytes) != 0)
			changed |= 1u << row;

		if (source == nullptr)
			continue;

		std::memcpy(surface.written + at, wanted + at, PaletteTexture::kRowBytes);
		ours |= 1u << row;
	}

	surface.known = true;
	surface.writtenMask = ours;

	if (changed != 0)
		PaletteTexture::Write(texture, wanted, changed);
}

bool SharedElsewhere(int seat, uintptr_t shared)
{
	for (int other = 0; other < PalettePaint::kSeats; ++other)
	{
		PaletteOwner::Owner owner = {};

		if (other != seat && PaletteOwner::Read(other, owner) && owner.shared == shared)
			return true;
	}

	return false;
}

void Follow(int player)
{
	Player& entry = g_players[player];
	PaletteOwner::Owner owner = {};

	if (!PaletteOwner::Read(player, owner))
	{
		entry.owner = 0;
		Forget(entry.own, nullptr);
		Forget(entry.shared, nullptr);
		return;
	}

	const bool allowed = PaletteControl::CanWear(player);
	const bool sharedAlone = owner.shared != 0 && !SharedElsewhere(player, owner.shared);

	entry.owner = owner.address;
	Sync(entry.own, PaletteTexture::Own(owner.address), entry, allowed);
	Sync(entry.shared, sharedAlone ? PaletteTexture::Shared(owner.shared) : nullptr, entry, allowed);
}

void Put(Layer& layer, int sub, const uint8_t* colours)
{
	std::memcpy(layer.pages[sub], colours, PalettePaint::kBytes);
	layer.mask |= Bit(sub);
}

}

void PalettePaint::Stage(int player, int sub, const uint8_t* colours)
{
	if (!Valid(player, sub) || colours == nullptr)
		return;

	Put(g_players[player].staged, sub, colours);
	++g_players[player].revision;
}

void PalettePaint::Clear(int player)
{
	if (!Valid(player, 0))
		return;

	g_players[player].staged.mask = 0;
	g_players[player].preview.mask = 0;
	++g_players[player].revision;
}

bool PalettePaint::IsStaged(int player)
{
	return Valid(player, 0) && g_players[player].staged.mask != 0;
}

const uint8_t* PalettePaint::GetStaged(int player, int sub)
{
	return Valid(player, sub) ? PageOf(g_players[player].staged, sub) : nullptr;
}

void PalettePaint::Preview(int player, int sub, const uint8_t* colours)
{
	if (!Valid(player, sub) || colours == nullptr)
		return;

	g_players[player].preview.mask = 0;
	Put(g_players[player].preview, sub, colours);
}

void PalettePaint::EndPreview(int player)
{
	if (Valid(player, 0))
		g_players[player].preview.mask = 0;
}

void PalettePaint::StageRemote(int player, int sub, const uint8_t* colours)
{
	if (Valid(player, sub) && colours != nullptr)
		Put(g_players[player].remote, sub, colours);
}

void PalettePaint::ClearRemote(int player)
{
	if (Valid(player, 0))
		g_players[player].remote.mask = 0;
}

bool PalettePaint::HasRemote(int player)
{
	return Valid(player, 0) && g_players[player].remote.mask != 0;
}

const uint8_t* PalettePaint::GetRemote(int player, int sub)
{
	return Valid(player, sub) ? PageOf(g_players[player].remote, sub) : nullptr;
}

void PalettePaint::OnFrame()
{
	for (int player = 0; player < kSeats; ++player)
		Follow(player);
}

bool PalettePaint::IsPainting(int player)
{
	return Valid(player, 0) && g_players[player].own.writtenMask != 0;
}

bool PalettePaint::ReadGameColours(int player, int sub, uint8_t* rgba)
{
	if (!Valid(player, sub) || rgba == nullptr || !g_players[player].own.known)
		return false;

	std::memcpy(rgba, g_players[player].own.pristine + static_cast<size_t>(sub * 2) * PaletteTexture::kRowBytes, kBytes);
	return true;
}

const uint8_t* PalettePaint::GetWorn(int player, int sub)
{
	if (!Valid(player, sub) || !PaletteControl::CanWear(player))
		return nullptr;

	return SourceFor(g_players[player], sub);
}

unsigned PalettePaint::GetRevision(int player)
{
	return Valid(player, 0) ? g_players[player].revision : 0;
}
