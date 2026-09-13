#include "Palette/PaletteOwner.h"

#include "Core/utils.h"
#include "Game/GameOffsets.h"
#include "Training/BattleMap.h"

namespace {

namespace Palette = GameOffsets::Palette;
namespace Objects = GameOffsets::Objects;
namespace Chara = GameOffsets::Meter::Chara;

constexpr uint8_t kSlotActive = 1;

struct Slot
{
	PaletteOwner::Owner owner;
	int32_t tag;
};

bool ReadSlot(int seat, Slot& out)
{
	const BattleAddresses& addresses = BattleMap::Addresses();

	if (seat < 0 || seat >= PaletteOwner::kSeats || addresses.charaArray == 0 || addresses.charaStride == 0)
		return false;

	const uintptr_t chara = addresses.charaArray + seat * addresses.charaStride;
	uint8_t active = 0;
	uint8_t side = 0xFF;
	uint32_t link = 0;

	if (!TryRead(chara + Objects::kActive, active) || active != kSlotActive)
		return false;

	if (!TryRead(chara + Chara::kSide, side) || side != PaletteOwner::SideOf(seat))
		return false;

	if (!TryRead(chara + Palette::kCharaOwner, link) || link <= Palette::kOwnerFromLink)
		return false;

	const uintptr_t owner = link - Palette::kOwnerFromLink;
	int32_t id = -1;
	uint32_t shared = 0;
	int32_t colour = 0;

	if (!TryRead(owner + Palette::kOwnerChara, id) || !TryRead(owner + Palette::kOwnerPaletteChara, out.tag) ||
		!TryRead(owner + Palette::kOwnerShared, shared))
	{
		return false;
	}

	if (id < 0 || id >= Palette::kMostCharas)
		return false;

	TryRead(owner + Palette::kOwnerColour, colour);
	out.owner = { owner, shared, id, colour };
	return true;
}

}

bool PaletteOwner::Read(int seat, Owner& out)
{
	out = {};

	Slot slot = {};

	if (!ReadSlot(seat, slot))
		return false;

	if (MemberOf(seat) > 0)
	{
		Slot main = {};

		if (slot.tag < 0 || !ReadSlot(SideOf(seat), main) || main.tag != slot.tag)
			return false;
	}

	out = slot.owner;
	return true;
}

int PaletteOwner::CharaNumber(int seat)
{
	Owner owner = {};
	return Read(seat, owner) ? owner.chara : -1;
}

bool PaletteOwner::HasPartner(int side)
{
	return CharaNumber(SeatOf(side, 1)) >= 0;
}
