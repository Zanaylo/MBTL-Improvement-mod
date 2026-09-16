#pragma once

#include <cstdint>

struct SceneAddresses
{
	uint8_t* step = nullptr;
	uintptr_t manager = 0;
	uintptr_t sceneId = 0;
	uintptr_t sceneReturn = 0;
	uintptr_t battleClear = 0;
	uintptr_t entering = 0;
	int titleScene = -1;
	int titleFlag = -1;
};

namespace SceneMap
{
	bool Initialize();
	const SceneAddresses& Addresses();
}
