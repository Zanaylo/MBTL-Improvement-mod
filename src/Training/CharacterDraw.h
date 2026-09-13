#pragma once

namespace CharacterDraw
{
	void Install();

	bool IsAvailable();
	bool IsHidden();
	void SetHidden(bool hidden);
	void Toggle();

	bool EffectsHidden();
	void SetEffectsHidden(bool hidden);

	const char* StatusText();
}
