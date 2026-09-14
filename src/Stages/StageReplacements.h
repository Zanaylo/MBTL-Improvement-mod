#pragma once

#include <string>
#include <vector>

namespace StageReplacements
{
	struct Replacement
	{
		int number;
		bool renamed;
		std::string name;
	};

	void Snapshot(std::vector<Replacement>& out);
}
