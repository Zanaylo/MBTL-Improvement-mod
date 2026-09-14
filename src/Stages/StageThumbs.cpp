#include "Stages/StageThumbs.h"

#include "Core/logger.h"
#include "Core/utils.h"
#include "Game/FileOverlay.h"
#include "Game/GameOffsets.h"
#include "Game/ModFiles.h"
#include "Stages/GameStages.h"
#include "Stages/StageCards.h"
#include "Stages/StageImage.h"
#include "Stages/StageLibrary.h"
#include "Stages/StageReplacements.h"
#include "Stages/StageRevision.h"
#include "Stages/StageTable.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>

namespace {

namespace Cards = GameOffsets::Cards;

constexpr const char* kSheetKey = "grpdat\\csel\\stage_thumb01.dds";
constexpr const char* kThumbName = "thumbnail";
constexpr const char* kMagic = "DDS ";
constexpr size_t kMagicBytes = 4;
constexpr size_t kDdsHeader = 128;
constexpr size_t kFlagsAt = 8;
constexpr size_t kWidthAt = 16;
constexpr size_t kMipsAt = 28;
constexpr size_t kPixelFlagsAt = 80;
constexpr size_t kBitsAt = 88;
constexpr size_t kCapsAt = 108;
constexpr uint32_t kMipsFlag = 0x20000;
constexpr uint32_t kMipsCaps = 0x400008;
constexpr uint32_t kFourCcFlag = 0x4;
constexpr uint32_t kRgbFlag = 0x40;
constexpr uint32_t kBits = 32;
constexpr size_t kChannels = 4;
constexpr int kMostRows = 12;
constexpr int kOpaque = 255;

uint32_t Read32(const std::vector<uint8_t>& data, size_t at)
{
	uint32_t value = 0;
	std::memcpy(&value, data.data() + at, sizeof(value));

	return value;
}

void Write32(std::vector<uint8_t>& data, size_t at, uint32_t value)
{
	std::memcpy(data.data() + at, &value, sizeof(value));
}

bool SheetHeight(uint32_t& out)
{
	std::string path;
	FILE* file = nullptr;

	if (!ModFiles::Find(Cards::kSecondSheetFile, path) || fopen_s(&file, path.c_str(), "rb") != 0 || file == nullptr)
		return false;

	uint8_t header[Cards::kDdsHeaderBytes] = {};
	const bool read = fread(header, 1, sizeof(header), file) == sizeof(header);
	fclose(file);

	if (read)
		std::memcpy(&out, header + Cards::kDdsHeightAt, sizeof(out));

	return read;
}

int RowsOf(uint32_t height)
{
	return (std::max)(static_cast<int>(height / Cards::kCellHeight), Cards::kStockRows);
}

int RowsFor(int card)
{
	return (card - Cards::kPerSheet) / Cards::kPerRow + 1;
}

int FirstFreeCard()
{
	uint32_t height = 0;

	if (!SheetHeight(height))
		return Cards::kStockFirstFree;

	return Cards::kPerSheet + RowsOf(height) * Cards::kPerRow;
}

int BaseRows()
{
	uint32_t height = 0;

	return SheetHeight(height) ? RowsOf(height) : Cards::kStockRows;
}

int CardLimit()
{
	return StageCards::IsAvailable() ? Cards::kPerSheet + kMostRows * Cards::kPerRow - 1 : StageCards::kStockLastCard;
}

bool HasThumb(int number)
{
	return !StageImage::Find(number, kThumbName).empty();
}

void Candidates(std::vector<int>& out)
{
	out.clear();

	std::vector<StageLibrary::Entry> entries;
	StageLibrary::Snapshot(entries);

	for (const StageLibrary::Entry& entry : entries)
	{
		if (!entry.removed && entry.number < StageTable::Numbers() && !GameStages::Owns(entry.number) &&
			HasThumb(entry.number))
		{
			out.push_back(entry.number);
		}
	}

	std::vector<StageReplacements::Replacement> replaced;
	StageReplacements::Snapshot(replaced);

	for (const StageReplacements::Replacement& replacement : replaced)
	{
		if (HasThumb(replacement.number))
			out.push_back(replacement.number);
	}

	std::sort(out.begin(), out.end());
}

bool Paintable(const std::vector<uint8_t>& sheet)
{
	if (sheet.size() < kDdsHeader || std::memcmp(sheet.data(), kMagic, kMagicBytes) != 0)
		return false;

	const uint32_t pixelFlags = Read32(sheet, kPixelFlagsAt);
	const size_t width = Read32(sheet, kWidthAt);
	const size_t height = Read32(sheet, Cards::kDdsHeightAt);

	return (pixelFlags & kRgbFlag) != 0 && (pixelFlags & kFourCcFlag) == 0 && Read32(sheet, kBitsAt) == kBits &&
		width >= static_cast<size_t>(Cards::kPerRow) * Cards::kCellWidth && sheet.size() >= kDdsHeader + width * height * kChannels;
}

bool Grow(std::vector<uint8_t>& sheet, int rows)
{
	const size_t width = Read32(sheet, kWidthAt);
	const uint32_t height = Read32(sheet, Cards::kDdsHeightAt);
	uint32_t wanted = height;

	while (wanted < static_cast<uint32_t>(rows) * Cards::kCellHeight)
		wanted *= 2;

	sheet.resize(kDdsHeader + width * height * kChannels);
	Write32(sheet, kMipsAt, 1);
	Write32(sheet, kFlagsAt, Read32(sheet, kFlagsAt) & ~kMipsFlag);
	Write32(sheet, kCapsAt, Read32(sheet, kCapsAt) & ~kMipsCaps);

	if (wanted == height)
		return false;

	sheet.resize(kDdsHeader + width * wanted * kChannels, 0);
	Write32(sheet, Cards::kDdsHeightAt, wanted);
	return true;
}

uint8_t* PixelAt(std::vector<uint8_t>& sheet, size_t x, size_t y)
{
	return sheet.data() + kDdsHeader + (y * Read32(sheet, kWidthAt) + x) * kChannels;
}

std::vector<uint8_t> MaskOf(std::vector<uint8_t>& sheet)
{
	std::vector<uint8_t> mask(static_cast<size_t>(Cards::kCellWidth) * Cards::kCellHeight);
	bool lit = false;

	for (size_t y = 0; y < Cards::kCellHeight; ++y)
	{
		for (size_t x = 0; x < Cards::kCellWidth; ++x)
		{
			mask[y * Cards::kCellWidth + x] = PixelAt(sheet, x, y)[3];
			lit = lit || mask[y * Cards::kCellWidth + x] != 0;
		}
	}

	return lit ? mask : std::vector<uint8_t>();
}

void CopyCell(std::vector<uint8_t>& sheet, const StageImage::Bitmap& image, size_t originX, size_t originY)
{
	for (size_t y = 0; y < Cards::kCellHeight; ++y)
		std::memcpy(PixelAt(sheet, originX, originY + y), &image.bgra[y * Cards::kCellWidth * kChannels], Cards::kCellWidth * kChannels);
}

void CoverArt(std::vector<uint8_t>& sheet, const std::vector<uint8_t>& mask, const StageImage::Bitmap& image, size_t originX,
	size_t originY)
{
	const StageImage::Bitmap art = StageImage::Cover(image, Cards::kArtWidth, Cards::kArtHeight);

	for (size_t y = 0; y < Cards::kCellHeight; ++y)
		std::memset(PixelAt(sheet, originX, originY + y), 0, Cards::kCellWidth * kChannels);

	for (int row = 0; row < Cards::kArtHeight; ++row)
	{
		for (int column = 0; column < Cards::kArtWidth; ++column)
		{
			const size_t cellX = static_cast<size_t>(Cards::kArtX) + column;
			const size_t cellY = static_cast<size_t>(Cards::kArtY) + row;
			uint8_t* const out = PixelAt(sheet, originX + cellX, originY + cellY);

			std::memcpy(out, &art.bgra[(static_cast<size_t>(row) * Cards::kArtWidth + column) * kChannels], kChannels);

			if (!mask.empty())
				out[3] = static_cast<uint8_t>(out[3] * mask[cellY * Cards::kCellWidth + cellX] / kOpaque);
		}
	}
}

bool Paint(std::vector<uint8_t>& sheet, const std::vector<uint8_t>& mask, const StageThumbs::Card& card)
{
	StageImage::Bitmap image;

	if (!StageImage::Load(StageImage::Find(card.number, kThumbName), image))
		return false;

	const int local = card.card - Cards::kPerSheet;
	const size_t originX = static_cast<size_t>(local % Cards::kPerRow) * Cards::kCellWidth;
	const size_t originY = static_cast<size_t>(local / Cards::kPerRow) * Cards::kCellHeight;

	if (image.width == static_cast<int>(Cards::kCellWidth) && image.height == static_cast<int>(Cards::kCellHeight))
	{
		CopyCell(sheet, image, originX, originY);
		return true;
	}

	CoverArt(sheet, mask, image, originX, originY);
	return true;
}

class SheetOverlay : public IFileOverlay
{
public:
	bool Covers(const std::string& key) const override { return key == kSheetKey; }
	uint32_t Version() const override { return StageRevision::Current(); }

	bool Apply(const std::string&, std::vector<uint8_t>& content) const override
	{
		std::vector<StageThumbs::Card> cards;
		StageThumbs::Assign(cards);

		if (cards.empty())
			return false;

		if (!Paintable(content))
		{
			LOG("StageThumbs: stage_thumb01.dds is not an uncompressed 32-bit DDS, so no thumbnail is painted");
			return false;
		}

		const int rows = (std::max)(RowsOf(Read32(content, Cards::kDdsHeightAt)), RowsFor(cards.back().card));
		const bool grown = Grow(content, rows);
		const std::vector<uint8_t> mask = MaskOf(content);
		int painted = 0;

		for (const StageThumbs::Card& card : cards)
			painted += Paint(content, mask, card) ? 1 : 0;

		LOG("StageThumbs: %d of %u thumbnail(s) painted on stage_thumb01, cards %d to %d, sheet %ux%u", painted,
			static_cast<unsigned>(cards.size()), cards.front().card, cards.back().card, Read32(content, kWidthAt),
			Read32(content, Cards::kDdsHeightAt));
		return painted != 0 || grown;
	}
};

SheetOverlay g_overlay;

}

void StageThumbs::Register()
{
	ModFiles::AddOverlay(&g_overlay);
}

void StageThumbs::Assign(std::vector<Card>& out)
{
	out.clear();

	std::vector<int> numbers;
	Candidates(numbers);

	const int limit = CardLimit();
	int card = FirstFreeCard();

	for (int number : numbers)
	{
		if (card > limit)
			return;

		out.push_back({ number, card++ });
	}
}

int StageThumbs::CardIn(const std::vector<Card>& cards, int number)
{
	const auto found = std::find_if(cards.begin(), cards.end(), [number](const Card& card) { return card.number == number; });

	return found == cards.end() ? -1 : found->card;
}

int StageThumbs::LastCard()
{
	if (!StageCards::IsAvailable())
		return StageCards::kStockLastCard;

	std::vector<Card> cards;
	Assign(cards);

	const int rows = cards.empty() ? BaseRows() : (std::max)(BaseRows(), RowsFor(cards.back().card));
	return Cards::kPerSheet + rows * Cards::kPerRow - 1;
}
