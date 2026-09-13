#pragma once

#include <cstdint>

struct SceneAddresses
{
	uint8_t* step = nullptr;
	uint8_t* request = nullptr;
	uintptr_t manager = 0;
	uintptr_t entering = 0;
	int titleScene = -1;
	int titleFlag = -1;
	uintptr_t replayChecker = 0;
	uintptr_t replayCountdown = 0;
};

namespace SceneMap
{
	bool Initialize();
	const SceneAddresses& Addresses();
}
