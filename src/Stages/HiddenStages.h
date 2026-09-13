#pragma once

#include <vector>

namespace HiddenStages
{
	void Load();

	bool Unlocked(int number);
	void SetUnlocked(int number, bool unlocked);

	void Snapshot(std::vector<int>& out);
}
