#pragma once

#include <cstdint>

namespace StageTable
{
	constexpr int kStockNumbers = 100;
	constexpr int kWideNumbers = 500;

	bool Initialize();

	bool Lifted();
	int Numbers();
	int ListEntries();

	uintptr_t RecordAt(int number);

	const char* StatusText();
}
