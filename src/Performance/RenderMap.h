#pragma once

#include <cstdint>

struct RenderAddresses
{
	uint8_t* stageSkip = nullptr;
	uint8_t* stageSampleRead = nullptr;
	uintptr_t stageSamples = 0;
	uintptr_t stageFxaa = 0;
};

namespace RenderMap
{
	bool Initialize();
	const RenderAddresses& Addresses();
}
