#pragma once

#include <cstdint>

namespace BgmVolume
{
	inline constexpr int kFullPercent = 100;

	void Load();
	void Install(uint8_t* setVolume);
	bool IsHooked();

	int Percent(int id);
	void SetPercent(int id, int percent);
	void ResetAll();
	void Save();
	int CustomCount();

	void ApplyToSlot(int id);
	void ApplyNow();
}
