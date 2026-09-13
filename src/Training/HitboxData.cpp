#include "Training/HitboxData.h"

#include "Core/utils.h"
#include "Game/GameOffsets.h"
#include "Training/BattleMap.h"

namespace {

namespace Objects = GameOffsets::Objects;

constexpr int kMostBoxesPerList = 64;
constexpr uintptr_t kPointerBytes = sizeof(uint32_t);
constexpr uint8_t kActive = 1;

HitboxData::BoxKind NormalKind(int index)
{
	if (index == Objects::kPushBox)
		return HitboxData::BoxKind_Push;

	if (index <= Objects::kLastHurtBox)
		return HitboxData::BoxKind_Hurt;

	return index == Objects::kClashBox ? HitboxData::BoxKind_Clash : HitboxData::BoxKind_Special;
}

bool Inactive(HitboxData::BoxKind kind, uint32_t flags)
{
	switch (kind)
	{
	case HitboxData::BoxKind_Push:
		return (flags & Objects::kNoPush) != 0;
	case HitboxData::BoxKind_Hurt:
		return (flags & Objects::kNoHurt) != 0;
	case HitboxData::BoxKind_Attack:
		return (flags & Objects::kNoAttack) != 0;
	default:
		return false;
	}
}

bool ReadObject(uintptr_t address, int slot, bool effect, HitboxData::Object& out)
{
	uint8_t active = 0;
	uint32_t frame = 0;

	if (!TryRead(address + Objects::kActive, active) || active != kActive)
		return false;

	if (!TryRead(address + Objects::kFrame, frame) || frame == 0)
		return false;

	int32_t positionX = 0;
	int32_t positionY = 0;
	int32_t offsetX = 0;
	int32_t offsetY = 0;
	uint8_t facing = 0;
	uint32_t flags = 0;

	if (!TryRead(address + Objects::kPositionX, positionX) || !TryRead(address + Objects::kPositionY, positionY) ||
		!TryRead(address + Objects::kOffsetX, offsetX) || !TryRead(address + Objects::kOffsetY, offsetY) ||
		!TryRead(address + Objects::kFacing, facing) || !TryRead(address + Objects::kExistFlags, flags))
	{
		return false;
	}

	out.address = address;
	out.frame = frame;
	out.x = positionX + offsetX;
	out.y = positionY + offsetY;
	out.flags = flags;
	out.facingLeft = facing != 0;
	out.effect = effect;
	out.slot = slot;
	return true;
}

int Append(uint32_t list, int entries, bool attack, uint32_t flags, HitboxData::Box* out, int count, int capacity)
{
	if (list == 0 || entries > kMostBoxesPerList)
		return count;

	for (int i = 0; i < entries && count < capacity; ++i)
	{
		uint32_t pointer = 0;
		int16_t corners[4] = {};

		if (!TryRead(list + i * kPointerBytes, pointer) || pointer == 0)
			continue;

		if (!TryReadMemory(corners, reinterpret_cast<const void*>(static_cast<uintptr_t>(pointer)), sizeof(corners)))
			continue;

		HitboxData::Box& box = out[count++];
		box.x1 = corners[0];
		box.y1 = corners[1];
		box.x2 = corners[2];
		box.y2 = corners[3];
		box.index = i;
		box.kind = attack ? HitboxData::BoxKind_Attack : NormalKind(i);
		box.inactive = Inactive(box.kind, flags);
	}

	return count;
}

}

bool HitboxData::IsAvailable()
{
	const BattleAddresses& addresses = BattleMap::Addresses();
	return addresses.charaArray != 0 && addresses.charaStride != 0;
}

int HitboxData::Objects(Object* out, int capacity)
{
	const BattleAddresses& addresses = BattleMap::Addresses();

	if (!IsAvailable())
		return 0;

	int count = 0;

	for (int slot = 0; slot < Objects::kCharaSlots && count < capacity; ++slot)
		count += ReadObject(addresses.charaArray + slot * addresses.charaStride, slot, false, out[count]) ? 1 : 0;

	uint32_t effects = 0;

	if (addresses.effectList == 0 || !TryRead(addresses.effectList, effects) || effects > Objects::kMostEffects)
		return count;

	for (uint32_t i = 0; i < effects && count < capacity; ++i)
	{
		uint32_t pointer = 0;

		if (!TryRead(addresses.effectList + Objects::kEffectPointers + i * kPointerBytes, pointer) || pointer == 0)
			continue;

		count += ReadObject(pointer, -1, true, out[count]) ? 1 : 0;
	}

	return count;
}

int HitboxData::Boxes(const Object& object, Box* out, int capacity)
{
	uint8_t normalCount = 0;
	uint8_t attackCount = 0;
	uint32_t normal = 0;
	uint32_t attack = 0;

	if (!TryRead(object.frame + Objects::kFrameNormalCount, normalCount) ||
		!TryRead(object.frame + Objects::kFrameAttackCount, attackCount) ||
		!TryRead(object.frame + Objects::kFrameNormalBoxes, normal) ||
		!TryRead(object.frame + Objects::kFrameAttackBoxes, attack))
	{
		return 0;
	}

	const int count = Append(normal, normalCount, false, object.flags, out, 0, capacity);
	return Append(attack, attackCount, true, object.flags, out, count, capacity);
}
