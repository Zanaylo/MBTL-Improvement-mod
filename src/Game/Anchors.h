#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Anchors
{
	struct Anchor
	{
		std::string name;
		uintptr_t address;
		std::string note;
	};

	void Record(const char* name, uintptr_t address, const char* note = "");
	void Snapshot(std::vector<Anchor>& out);
}
