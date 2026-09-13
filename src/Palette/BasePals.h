#pragma once

#include <cstddef>
#include <cstdint>

namespace BasePals
{
	bool Has(int chara, int sub);
	bool Get(int chara, int sub, const uint8_t*& outData, size_t& outSize);
}
