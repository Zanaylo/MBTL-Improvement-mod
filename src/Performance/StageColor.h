#pragma once

#include <cstdint>

namespace StageColor
{
	bool Install();
	bool IsAvailable();

	void SetEnabled(bool enabled);
	bool IsEnabled();

	void SetColor(uint32_t rgb);
	uint32_t GetClearColor();

	void Apply();

	const char* StatusText();
}
