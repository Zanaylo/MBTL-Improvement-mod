#pragma once

namespace StageTable
{
	constexpr int kStockNumbers = 100;
	constexpr int kWideNumbers = 500;

	bool Initialize();

	bool Lifted();
	int Numbers();
	int ListEntries();

	const char* StatusText();
}
