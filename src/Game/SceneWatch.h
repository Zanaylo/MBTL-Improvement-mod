#pragma once

#include <cstdint>

namespace SceneWatch
{
	constexpr uint32_t kNone = 0xFFFFFFFF;

	void OnFrame();

	uint32_t Current();
	uint32_t First();
}
