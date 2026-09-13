#pragma once

#include <cstdint>

namespace StockPalettes
{
	bool Load(int chara);

	bool HasSub(int chara, int sub);
	int GetCount(int chara, int sub);

	const uint8_t* GetRow(int chara, int sub, int colour);
}
