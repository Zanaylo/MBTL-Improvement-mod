#pragma once

#include <cstdint>

struct BattleAddresses
{
	uint8_t* battleStep = nullptr;
	uintptr_t battleInfo = 0;
	uintptr_t session = 0;
	uintptr_t pause = 0;
	uintptr_t charaArray = 0;
	uint32_t charaStride = 0;
	uintptr_t effectList = 0;
	uintptr_t camera = 0;
	uintptr_t teams = 0;
	uint32_t teamStride = 0;
	uintptr_t combos = 0;
	uint32_t comboStride = 0;
};

namespace BattleMap
{
	bool Initialize();
	const BattleAddresses& Addresses();
}
