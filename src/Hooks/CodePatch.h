#pragma once

#include <cstddef>
#include <cstdint>

namespace CodePatch
{
	bool Write(uint8_t* at, const void* bytes, size_t size);
}
