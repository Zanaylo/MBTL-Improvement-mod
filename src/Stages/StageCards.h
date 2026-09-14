#pragma once

namespace StageCards
{
	constexpr int kStockLastCard = 41;

	void Install();

	bool IsAvailable();
	const char* StatusText();
}
