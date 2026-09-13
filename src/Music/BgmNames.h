#pragma once

#include <cstddef>

namespace BgmNames
{
	const char* SceneOf(int id);

	void Label(int id, const char* file, char* out, size_t size);
	void Describe(int id, char* out, size_t size);
}
