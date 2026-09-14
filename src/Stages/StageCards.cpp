#include "Stages/StageCards.h"

#include "Core/logger.h"
#include "Game/Anchors.h"
#include "Game/GameOffsets.h"
#include "Hooks/CodePatch.h"
#include "Hooks/ImageScanner.h"

#include <cstdio>
#include <cstring>
#include <vector>

namespace {

namespace Cards = GameOffsets::Cards;

struct Piece
{
	size_t at;
	const uint8_t* bytes;
	size_t count;
};

const Piece kPieces[] = {
	{ 0, Cards::kLoadFramed, sizeof(Cards::kLoadFramed) },
	{ 3, Cards::kDivide, sizeof(Cards::kDivide) },
	{ 11, Cards::kStoreQuotient, sizeof(Cards::kStoreQuotient) },
	{ 17, Cards::kLoadFramed, sizeof(Cards::kLoadFramed) },
	{ 20, Cards::kDivide, sizeof(Cards::kDivide) },
	{ 28, Cards::kStoreRemainder, sizeof(Cards::kStoreRemainder) },
	{ 31, Cards::kCompareQuotient, sizeof(Cards::kCompareQuotient) },
	{ 37, Cards::kClampJump, sizeof(Cards::kClampJump) },
	{ 40, Cards::kClearQuotient, sizeof(Cards::kClearQuotient) },
	{ 50, Cards::kLoadQuotient, sizeof(Cards::kLoadQuotient) },
	{ 56, Cards::kLoadFramed, sizeof(Cards::kLoadFramed) },
	{ 59, Cards::kFetchSheet, sizeof(Cards::kFetchSheet) },
};

constexpr size_t kIndexAt[] = { 2, 19, 30 };
constexpr size_t kQuotientAt[] = { 13, 33, 42, 52 };
constexpr size_t kClearedAt = 46;
constexpr size_t kSheetDispAt = 62;

constexpr uint8_t kNop = 0x90;
constexpr size_t kNewQuotientAt = 16;

bool g_patched = false;
char g_status[160] = "the stage picker keeps the game's own 42 cards";

void Refuse(const char* reason)
{
	sprintf_s(g_status, "the stage picker keeps the game's own 42 cards: %s", reason);
	LOG("StageCards: %s", g_status);
}

bool IsSplit(const uint8_t* at, uint32_t sheetDisp, uint8_t& index, uint32_t& quotient)
{
	for (const Piece& piece : kPieces)
	{
		if (std::memcmp(at + piece.at, piece.bytes, piece.count) != 0)
			return false;
	}

	index = at[kIndexAt[0]];
	quotient = ImageScanner::ReadDword(at + kQuotientAt[0]);

	for (size_t offset : kIndexAt)
	{
		if (at[offset] != index)
			return false;
	}

	for (size_t offset : kQuotientAt)
	{
		if (ImageScanner::ReadDword(at + offset) != quotient)
			return false;
	}

	return ImageScanner::ReadDword(at + kClearedAt) == 0 && ImageScanner::ReadDword(at + kSheetDispAt) == sheetDisp;
}

std::vector<uint8_t*> FindSplits(uint32_t sheetDisp, uint8_t& index, uint32_t& quotient)
{
	std::vector<uint8_t*> sites;
	const ImageSection code = ImageScanner::Code();

	for (size_t i = 0; i + Cards::kMatchLength <= code.size; ++i)
	{
		uint8_t* const at = code.begin + i;

		if (at[0] == Cards::kLoadFramed[0] && IsSplit(at, sheetDisp, index, quotient))
			sites.push_back(at);
	}

	return sites;
}

bool SheetSlot(const uint8_t* loader, size_t length, const char* anchor, uint32_t& out)
{
	const std::vector<uint8_t*> names = ImageScanner::FindString(anchor);
	const uint8_t* const after = names.size() == 1 ? ImageScanner::AfterPushOf(loader, length, names.front()) : nullptr;

	if (after == nullptr)
		return false;

	for (size_t i = 0; i < Cards::kStoreWindow && after + i + Cards::kStoreDispAt + sizeof(uint32_t) <= loader + length; ++i)
	{
		if (std::memcmp(after + i, Cards::kStoreSheet, sizeof(Cards::kStoreSheet)) != 0)
			continue;

		out = ImageScanner::ReadDword(after + i + Cards::kStoreDispAt);
		return true;
	}

	return false;
}

void BuildSplit(uint8_t index, uint32_t quotient, uint8_t* out)
{
	const uint8_t perSheet = static_cast<uint8_t>(Cards::kPerSheet);
	const uint8_t code[] = {
		0x8B, 0x45, index,
		0x31, 0xD2,
		0x83, 0xF8, perSheet,
		0x7C, 0x04,
		0x42,
		0x83, 0xE8, perSheet,
		0x89, 0x95, 0x00, 0x00, 0x00, 0x00,
		0x89, 0x45, index,
	};

	std::memset(out, kNop, Cards::kSplitLength);
	std::memcpy(out, code, sizeof(code));
	std::memcpy(out + kNewQuotientAt, &quotient, sizeof(quotient));
}

}

void StageCards::Install()
{
	const std::vector<uint8_t*> loaders = ImageScanner::FunctionsReferencing(ImageScanner::FindString(Cards::kSecondSheetAnchor));
	const uint8_t* const loader = loaders.size() == 1 ? loaders.front() : nullptr;
	const size_t length = loader ? ImageScanner::FunctionLength(loader) : 0;

	Anchors::Record("Stage card sheets loader", reinterpret_cast<uintptr_t>(loader), "stage cards");

	uint32_t first = 0;
	uint32_t second = 0;

	if (!loader || !SheetSlot(loader, length, Cards::kFirstSheetAnchor, first) ||
		!SheetSlot(loader, length, Cards::kSecondSheetAnchor, second) || second != first)
	{
		Refuse("the sheet loader was not recognised");
		return;
	}

	uint8_t index = 0;
	uint32_t quotient = 0;
	const std::vector<uint8_t*> sites = FindSplits(first, index, quotient);

	if (sites.size() != 1)
	{
		char reason[64] = {};
		sprintf_s(reason, "%u card split(s) found, expected exactly one", static_cast<unsigned>(sites.size()));
		Refuse(reason);
		return;
	}

	Anchors::Record("Stage card split", reinterpret_cast<uintptr_t>(sites.front()), "stage cards");

	uint8_t split[Cards::kSplitLength] = {};
	BuildSplit(index, quotient, split);

	if (!CodePatch::Write(sites.front(), split, sizeof(split)))
	{
		Refuse("the card split could not be written");
		return;
	}

	g_patched = true;
	sprintf_s(g_status, "every card past 20 is read down stage_thumb01, as many rows as it has");
	LOG("StageCards: %s", g_status);
}

bool StageCards::IsAvailable()
{
	return g_patched;
}

const char* StageCards::StatusText()
{
	return g_status;
}
