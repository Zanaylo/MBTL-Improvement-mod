#include "Palette/LivePalette.h"

#include "Palette/ColorPartTable.h"
#include "Palette/StockPalettes.h"

#include <cstring>

namespace {

int Luminance(const uint8_t* rgb)
{
	return (rgb[0] * 299 + rgb[1] * 587 + rgb[2] * 114) / 1000;
}

void Retint(uint8_t* palette, const unsigned char* entries, int count, const uint8_t* reference, const uint8_t* rgb)
{
	const int base = Luminance(reference);

	if (base <= 0)
		return;

	for (int i = 0; i < count; ++i)
	{
		uint8_t* const target = palette + entries[i] * 4;
		const int ratio = Luminance(target) * 255 / base;

		for (int c = 0; c < 3; ++c)
		{
			const int lit = rgb[c] * ratio / 255;
			target[c] = static_cast<uint8_t>(lit > 255 ? 255 : lit);
		}
	}
}

const uint8_t* BaseRow(const LivePalette::Colours& colours, bool haveStock)
{
	const uint8_t* const chosen = colours.stock[0] != LivePalette::kAsDrawn && haveStock
		? StockPalettes::GetRow(colours.chara, colours.sub, colours.stock[0]) : nullptr;

	if (chosen != nullptr)
		return chosen;

	if (colours.hasBaseline)
		return colours.baseline;

	return haveStock ? StockPalettes::GetRow(colours.chara, colours.sub, 0) : nullptr;
}

void ApplyPartStocks(const LivePalette::Colours& colours, bool haveStock, uint8_t* out)
{
	unsigned char entries[LivePalette::kColours] = {};

	for (int part = 1; part < LivePalette::kParts; ++part)
	{
		if (!haveStock || colours.stock[part] == LivePalette::kAsDrawn || colours.stock[part] == colours.stock[0])
			continue;

		const uint8_t* const row = StockPalettes::GetRow(colours.chara, colours.sub, colours.stock[part]);
		const int count = ColorPartTable::Entries(colours.chara, colours.sub, part, entries);

		for (int i = 0; row != nullptr && i < count; ++i)
			std::memcpy(out + entries[i] * 4, row + entries[i] * 4, 4);
	}
}

void ApplyPicks(const LivePalette::Colours& colours, uint8_t* out)
{
	unsigned char entries[LivePalette::kColours] = {};

	for (int part = 0; part < LivePalette::kParts; ++part)
	{
		uint8_t reference[3] = {};

		if (!colours.picked[part] || !LivePalette::Reference(out, colours.chara, colours.sub, part, reference))
			continue;

		const int count = ColorPartTable::Entries(colours.chara, colours.sub, part, entries);
		Retint(out, entries, count, reference, colours.pick[part]);
	}
}

}

void LivePalette::Reset(Colours& colours, int chara, int sub)
{
	const bool keepBaseline = colours.chara == chara && colours.sub == sub && colours.hasBaseline;

	uint8_t baseline[kBytes] = {};

	if (keepBaseline)
		std::memcpy(baseline, colours.baseline, sizeof(baseline));

	std::memset(&colours, 0, sizeof(colours));

	colours.chara = chara;
	colours.sub = sub;

	for (int& stock : colours.stock)
		stock = kAsDrawn;

	if (!keepBaseline)
		return;

	std::memcpy(colours.baseline, baseline, sizeof(baseline));
	colours.hasBaseline = true;
}

void LivePalette::SetBaseline(Colours& colours, const uint8_t* rgba)
{
	if (rgba == nullptr)
		return;

	std::memcpy(colours.baseline, rgba, kBytes);
	colours.hasBaseline = true;
}

bool LivePalette::IsCustom(const Colours& colours)
{
	for (int part = 0; part < kParts; ++part)
	{
		if (colours.picked[part] || colours.stock[part] != kAsDrawn)
			return true;
	}

	for (bool edited : colours.edited)
	{
		if (edited)
			return true;
	}

	return false;
}

bool LivePalette::Reference(const uint8_t* palette, int chara, int sub, int part, uint8_t* outRgb)
{
	unsigned char entries[kColours] = {};
	const int count = ColorPartTable::Entries(chara, sub, part, entries);

	int brightest = -1;

	for (int i = 0; i < count; ++i)
	{
		const uint8_t* const entry = palette + entries[i] * 4;

		if (Luminance(entry) <= brightest)
			continue;

		brightest = Luminance(entry);
		std::memcpy(outRgb, entry, 3);
	}

	return brightest >= 0;
}

bool LivePalette::Compose(const Colours& colours, uint8_t* out)
{
	if (out == nullptr)
		return false;

	const bool haveStock = StockPalettes::Load(colours.chara);
	const uint8_t* const base = BaseRow(colours, haveStock);

	if (base == nullptr)
		return false;

	std::memcpy(out, base, kBytes);

	ApplyPartStocks(colours, haveStock, out);
	ApplyPicks(colours, out);

	for (int index = 1; index < kColours; ++index)
	{
		if (colours.edited[index])
			std::memcpy(out + index * 4, colours.entry[index], 3);
	}

	return true;
}
