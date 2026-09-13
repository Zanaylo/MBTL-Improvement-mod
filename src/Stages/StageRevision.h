#pragma once

#include <cstdint>

namespace StageRevision
{
	void Bump();

	uint32_t Current();
	bool Changed();
}
