#pragma once

namespace ColorPartTable
{
	constexpr int kParts = 6;
	constexpr int kColours = 256;

	bool Has(int chara, int sub);
	const char* Name(int chara, int sub, int part);
	int Entries(int chara, int sub, int part, unsigned char* out);
}
