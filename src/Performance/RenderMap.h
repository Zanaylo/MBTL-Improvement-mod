#pragma once

#include <cstdint>

struct RenderAddresses
{
	uint8_t* stageGate = nullptr;
	uintptr_t stageMultisample = 0;
	uintptr_t stageFxaa = 0;
};

namespace RenderMap
{
	bool Initialize();
	const RenderAddresses& Addresses();
}
