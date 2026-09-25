#include "Palette/StockPalettes.h"

#include "Core/logger.h"
#include "Game/GameAssets.h"
#include "Game/GameOffsets.h"

#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace {

namespace Palette = GameOffsets::Palette;

struct Sub
{
	std::vector<uint8_t> rows;
	int count = 0;
};

struct Character
{
	bool loaded = false;
	Sub subs[Palette::kSubPalettes];
};

std::map<int, Character> g_characters;

std::string PathOf(int chara, int sub)
{
	char path[64] = {};

	if (sub > 0)
		sprintf_s(path, "data/chr%03d/chr%03d_p%d.pal", chara, chara, sub);
	else
		sprintf_s(path, "data/chr%03d/chr%03d.pal", chara, chara);

	return path;
}

int DistinctRows(const std::vector<uint8_t>& rows, int stored)
{
	int distinct = 1;

	for (int row = 1; row < stored; ++row)
	{
		if (std::memcmp(rows.data() + row * Palette::kPageBytes, rows.data(), Palette::kPageBytes) != 0)
			distinct = row + 1;
	}

	return distinct;
}

bool Parse(const std::vector<uint8_t>& data, int limit, Sub& out, size_t& outPages)
{
	if (data.size() < Palette::kFileHeader + Palette::kPageBytes)
		return false;

	const size_t pages = (data.size() - Palette::kFileHeader) / Palette::kPageBytes;
	const size_t keep = pages < static_cast<size_t>(limit) ? pages : static_cast<size_t>(limit);

	out.rows.assign(data.begin() + Palette::kFileHeader, data.begin() + Palette::kFileHeader + keep * Palette::kPageBytes);
	out.count = DistinctRows(out.rows, static_cast<int>(keep));
	outPages = pages;
	return true;
}

const Sub* Find(int chara, int sub)
{
	if (sub < 0 || sub >= Palette::kSubPalettes || !StockPalettes::Load(chara))
		return nullptr;

	const Sub& found = g_characters[chara].subs[sub];
	return found.count > 0 ? &found : nullptr;
}

}

bool StockPalettes::Load(int chara)
{
	if (chara < 0 || chara >= Palette::kMostCharas)
		return false;

	Character& character = g_characters[chara];

	if (character.loaded)
		return character.subs[0].count > 0;

	character.loaded = true;

	for (int sub = 0; sub < Palette::kSubPalettes; ++sub)
	{
		std::vector<uint8_t> data;
		size_t pages = 0;

		if (!GameAssets::Read(PathOf(chara, sub).c_str(), data) ||
			!Parse(data, Palette::kStockLimit, character.subs[sub], pages))
		{
			continue;
		}

		LOG("stock palettes: %s holds %u page(s), %d of them different",
			PathOf(chara, sub).c_str(), static_cast<unsigned>(pages), character.subs[sub].count);
	}

	if (character.subs[0].count == 0)
		LOG("stock palettes: %s could not be read", PathOf(chara, 0).c_str());

	return character.subs[0].count > 0;
}

bool StockPalettes::HasSub(int chara, int sub)
{
	return Find(chara, sub) != nullptr;
}

int StockPalettes::GetCount(int chara, int sub)
{
	const Sub* const found = Find(chara, sub);
	return found != nullptr ? found->count : 0;
}

const uint8_t* StockPalettes::GetRow(int chara, int sub, int colour)
{
	const Sub* const found = Find(chara, sub);

	if (found == nullptr || colour < 0 || colour >= found->count)
		return nullptr;

	return found->rows.data() + static_cast<size_t>(colour) * Palette::kPageBytes;
}
