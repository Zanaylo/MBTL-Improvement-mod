#pragma once

#include <cstdint>
#include <vector>

namespace GameAssets
{
	bool IsAvailable();
	bool Read(const char* path, std::vector<uint8_t>& out);
}
