#include "Training/PlayerState.h"

#include "Core/utils.h"
#include "Game/GameOffsets.h"
#include "Training/HitboxData.h"

namespace {

namespace Chara = GameOffsets::Meter::Chara;
namespace Codes = GameOffsets::Meter::Codes;
namespace Objects = GameOffsets::Objects;

constexpr int kCountdowns = 4;

bool RecordActive(uintptr_t record)
{
	int32_t time = 0;
	return TryRead(record + Chara::kRecordTime, time) && time > 0;
}

bool RecordByte(uintptr_t record, uint8_t& out)
{
	return RecordActive(record) && TryRead(record, out);
}

bool ReadFlag(uintptr_t address)
{
	uint8_t value = 0;
	return TryRead(address, value) && value != 0;
}

bool ReadFrameRecord(uintptr_t chara, uintptr_t& out)
{
	uint32_t frame = 0;
	uint32_t record = 0;

	if (!TryRead(chara + Objects::kFrame, frame) || frame == 0)
		return false;

	if (!TryRead(frame + Chara::kFrameRecord, record) || record == 0)
		return false;

	out = record;
	return true;
}

bool ReadActionable(uintptr_t chara, uintptr_t record)
{
	for (int i = 0; i < Chara::kMoveAbleRecords; ++i)
	{
		uint8_t value = 0;

		if (RecordByte(chara + Chara::kMoveAble + i * Chara::kRecordStride, value))
			return value != 0;
	}

	return ReadFlag(record + Chara::kFrameFree);
}

void ReadCancels(uintptr_t chara, uintptr_t record, PlayerState::State& out)
{
	TryRead(record + Chara::kFrameCancelNormal, out.cancelNormal);
	TryRead(record + Chara::kFrameCancelSpecial, out.cancelSpecial);

	uint16_t boost = 0;

	if (!RecordActive(chara + Chara::kCancelBoost) || !TryRead(chara + Chara::kCancelBoost, boost))
		return;

	const uint8_t normal = static_cast<uint8_t>(boost & 0xFF);
	const uint8_t special = static_cast<uint8_t>((boost >> 8) & 0xFF);

	if (normal != Chara::kCancelUnset)
		out.cancelNormal = normal;

	if (special != Chara::kCancelUnset)
		out.cancelSpecial = special;
}

uint8_t ReadStance(uintptr_t chara, uintptr_t record)
{
	uint8_t stance = 0;

	if (!RecordByte(chara + Chara::kStanceOverride, stance))
		TryRead(record + Chara::kFrameStance, stance);

	return stance;
}

uint16_t FrameInvuln(uint8_t kind)
{
	switch (kind)
	{
	case Chara::kInvulnBoth:
		return PlayerState::Invuln_Strike | PlayerState::Invuln_Throw;
	case Chara::kInvulnStrike:
		return PlayerState::Invuln_Strike;
	case Chara::kInvulnThrow:
		return PlayerState::Invuln_Throw;
	case Chara::kInvulnHighMid:
		return PlayerState::Invuln_HighMid;
	case Chara::kInvulnLowMid:
		return PlayerState::Invuln_LowMid;
	default:
		return 0;
	}
}

uint16_t AttributeInvuln(uint8_t attributes)
{
	uint16_t invuln = 0;

	invuln |= (attributes & Chara::kHitHead) != 0 ? PlayerState::Invuln_Head : 0;
	invuln |= (attributes & Chara::kHitBody) != 0 ? PlayerState::Invuln_Body : 0;
	invuln |= (attributes & Chara::kHitLegs) != 0 ? PlayerState::Invuln_Legs : 0;
	invuln |= (attributes & Chara::kHitFireBall) != 0 ? PlayerState::Invuln_Projectile : 0;
	invuln |= (attributes & Chara::kHitThrow) != 0 ? PlayerState::Invuln_Throw : 0;
	invuln |= (attributes & Chara::kHitAirDive) != 0 ? PlayerState::Invuln_Dive : 0;

	return invuln;
}

bool FullyInvulnerable(uintptr_t chara)
{
	uint8_t gate = 0;
	uint8_t level = 0;

	return TryRead(chara + Chara::kFullInvulnGate, gate) && gate == 0 &&
		TryRead(chara + Chara::kFullInvulnLevel, level) && level >= Chara::kFullInvulnLeast;
}

uint16_t ReadInvuln(uintptr_t chara, uintptr_t record, const PlayerState::State& state)
{
	if ((state.moveCode[3] & Codes::kAnten) != 0 || (state.moveCode[6] & Codes::kSousaiMuteki) != 0)
		return 0;

	if (FullyInvulnerable(chara))
		return PlayerState::Invuln_Full;

	uint8_t kind = 0;
	TryRead(record + Chara::kFrameInvuln, kind);

	uint16_t invuln = FrameInvuln(kind);

	uint8_t countdowns[kCountdowns] = {};

	if (TryReadMemory(countdowns, reinterpret_cast<const void*>(chara + Chara::kInvulnCountdowns), sizeof(countdowns)))
	{
		invuln |= (countdowns[0] | countdowns[2]) != 0 ? PlayerState::Invuln_Strike : 0;
		invuln |= (countdowns[1] | countdowns[3]) != 0 ? PlayerState::Invuln_Throw : 0;
	}

	uint8_t attributes = 0;

	if (RecordByte(chara + Chara::kHitCheck, attributes))
		invuln |= AttributeInvuln(attributes);

	return invuln;
}

int ReadAttackBoxes(uintptr_t object)
{
	uint32_t frame = 0;
	uint32_t flags = 0;
	uint8_t count = 0;

	if (!TryRead(object + Objects::kFrame, frame) || frame == 0)
		return 0;

	if (!TryRead(frame + Objects::kFrameAttackCount, count) || !TryRead(object + Objects::kExistFlags, flags))
		return 0;

	return (flags & Objects::kNoAttack) != 0 ? 0 : count;
}

bool ReadProjectileActive(uintptr_t chara)
{
	uint32_t owned = 0;

	if (!TryRead(chara + Chara::kOwnedObjects, owned) || owned == 0)
		return false;

	HitboxData::Object objects[HitboxData::kMaxObjects];
	const int count = HitboxData::Objects(objects, HitboxData::kMaxObjects);

	for (int i = 0; i < count; ++i)
	{
		uint32_t owner = 0;

		if (!objects[i].effect || !TryRead(objects[i].address + Chara::kOwner, owner) || owner != chara)
			continue;

		if (ReadAttackBoxes(objects[i].address) > 0)
			return true;
	}

	return false;
}

bool ReadMotion(uintptr_t chara, PlayerState::State& out)
{
	uint32_t pattern = 0;

	if (!TryRead(chara + Chara::kPattern, pattern) || !TryRead(chara + Chara::kMvCountFrame, out.mvCountFrame) ||
		!TryRead(chara + Chara::kCommand, out.command))
	{
		return false;
	}

	out.pattern = static_cast<uint16_t>(pattern);
	return TryReadMemory(out.moveCode, reinterpret_cast<const void*>(chara + Chara::kMoveCodes), sizeof(out.moveCode));
}

void ReadReaction(uintptr_t chara, PlayerState::State& out)
{
	int16_t hitstop = 0;
	uint32_t reaction = 0;
	int32_t stun = 0;

	TryRead(chara + Chara::kHitstop, hitstop);
	TryRead(chara + Chara::kReaction, reaction);
	TryRead(chara + Chara::kReactionStun, stun);

	out.hitstop = hitstop;
	out.inReaction = reaction != 0;
	out.stunTimer = stun > 0 ? stun : 0;
	out.inGuard = ReadFlag(chara + Chara::kInGuard);
}

}

bool PlayerState::Read(uintptr_t chara, State& out)
{
	out = State();

	uintptr_t record = 0;

	if (chara == 0 || !ReadFrameRecord(chara, record) || !ReadMotion(chara, out))
		return false;

	ReadReaction(chara, out);

	uint8_t armor = 0;

	out.shield = ReadFlag(chara + Chara::kShield);
	out.armor = RecordByte(chara + Chara::kArmor, armor) && armor != 0;

	out.actionable = ReadActionable(chara, record);
	ReadCancels(chara, record, out);

	out.stance = ReadStance(chara, record);
	out.running = ReadFlag(chara + Chara::kRunning);

	out.attackBoxes = ReadAttackBoxes(chara);
	out.projectileActive = ReadProjectileActive(chara);
	out.invuln = ReadInvuln(chara, record, out);
	return true;
}

bool PlayerState::IsAirborne(const State& state)
{
	return state.stance == Chara::kStanceAir;
}

bool PlayerState::IsAttacking(const State& state)
{
	return (state.moveCode[0] & Codes::kAttackBits) != 0;
}

bool PlayerState::IsTeching(const State& state)
{
	return (state.moveCode[0] & Codes::kRecovery) != 0;
}

bool PlayerState::CanLeaveFrameOnWhiff(const State& state)
{
	return state.actionable || state.cancelNormal == Chara::kCancelAlways || state.cancelSpecial == Chara::kCancelAlways;
}
