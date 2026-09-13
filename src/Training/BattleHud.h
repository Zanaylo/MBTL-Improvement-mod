#pragma once

namespace BattleHud
{
	void Install();

	bool IsAvailable();
	bool IsHidden();
	void SetHidden(bool hidden);
	void Toggle();

	const char* StatusText();
}
