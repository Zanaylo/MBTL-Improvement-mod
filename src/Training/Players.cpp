#include "Training/Players.h"

#include "Core/utils.h"
#include "Game/GameOffsets.h"
#include "Training/BattleMap.h"

namespace {

namespace Meter = GameOffsets::Meter;
namespace Objects = GameOffsets::Objects;

constexpr uint8_t kSlotActive = 1;

}

uintptr_t Players::Chara(int team)
{
	const BattleAddresses& addresses = BattleMap::Addresses();

	if (team < 0 || team >= kTeams || !addresses.teams || !addresses.teamStride || !addresses.charaArray ||
		!addresses.charaStride)
	{
		return 0;
	}

	const uintptr_t record = addresses.teams + team * addresses.teamStride;
	uint32_t slot = 0;
	uint32_t point = 0;

	if (!TryRead(record + Meter::kTeamSlot, slot) || !TryRead(record + Meter::kTeamPoint, point))
		return 0;

	const uint32_t index = slot + 2 * point;

	if (index >= static_cast<uint32_t>(Objects::kCharaSlots))
		return 0;

	const uintptr_t chara = addresses.charaArray + index * addresses.charaStride;
	uint8_t active = 0;
	uint32_t objectId = 1;
	uint8_t side = 0xFF;

	if (!TryRead(chara + Objects::kActive, active) || active != kSlotActive)
		return 0;

	if (!TryRead(chara + Meter::Chara::kObjectId, objectId) || objectId != 0)
		return 0;

	return TryRead(chara + Meter::Chara::kSide, side) && side == team ? chara : 0;
}
