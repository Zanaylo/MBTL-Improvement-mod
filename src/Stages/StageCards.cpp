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

bool g_patched = false;
char g_status[160] = "The stage picker uses only the game's 42 cards.";

void Refuse(const char* reason)
{
	sprintf_s(g_status, "The stage picker uses only the game's 42 cards: %s.", reason);
	LOG("StageCards: %s", g_status);
}

bool SheetSlot(const uint8_t* loader, size_t length, const char* anchor, uint32_t& out)
{
	const std::vector<uint8_t*> names = ImageScanner::FindString(anchor);

	if (names.size() != 1)
		return false;

	const auto name = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(names.front()));

	for (size_t i = 0; i + 1 + sizeof(uint32_t) <= length; ++i)
	{
		if (loader[i] != Cards::kLoadEcxName || ImageScanner::ReadDword(loader + i + 1) != name)
			continue;

		size_t at = i + 1 + sizeof(uint32_t);

		while (at < length && loader[at] != Cards::kCall)
			++at;

		for (size_t k = at + Cards::kCallLength; k + Cards::kStoreSheetLength <= length && k < at + Cards::kStoreWindow; ++k)
		{
			if (std::memcmp(loader + k, Cards::kStoreSheet, sizeof(Cards::kStoreSheet)) != 0)
				continue;

			out = ImageScanner::ReadDword(loader + k + Cards::kStoreSheetDispAt);
			return true;
		}

		return false;
	}

	return false;
}

uint8_t* FindSplit(uint32_t sheetDisp)
{
	uint8_t pattern[Cards::kClampLength] = {};
	std::memcpy(pattern, Cards::kClampAndFetch, sizeof(Cards::kClampAndFetch));
	std::memcpy(pattern + Cards::kFetchDispAt, &sheetDisp, sizeof(sheetDisp));

	const std::vector<uint8_t*> sites = ImageScanner::FindBytes(ImageScanner::Code(), pattern, sizeof(pattern));

	if (sites.size() != 1)
	{
		char reason[64] = {};
		sprintf_s(reason, "found %u card split(s), expected 1", static_cast<unsigned>(sites.size()));
		Refuse(reason);
		return nullptr;
	}

	uint8_t* const divide = sites.front() - sizeof(Cards::kDivide);

	if (!ImageScanner::InCode(divide, sizeof(Cards::kDivide)) ||
		std::memcmp(divide, Cards::kDivide, sizeof(Cards::kDivide)) != 0)
	{
		Refuse("the card split does not divide by 21 the way the mod knows");
		return nullptr;
	}

	return divide;
}

void BuildSplit(uint8_t* out)
{
	const uint8_t perSheet = static_cast<uint8_t>(Cards::kPerSheet);
	const uint8_t code[] = {
		0xF3, 0x0F, 0x10, 0x4B, 0x4C,
		0x0F, 0x57, 0xC0,
		0x33, 0xC9,
		0x83, 0xFF, perSheet,
		0x7C, 0x04,
		0x41,
		0x83, 0xEF, perSheet,
	};

	std::memset(out, Cards::kNop, sizeof(Cards::kDivide));
	std::memcpy(out, code, sizeof(code));
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
		!SheetSlot(loader, length, Cards::kSecondSheetAnchor, second) || second != first + sizeof(uint32_t))
	{
		Refuse("the card sheet code was not found");
		return;
	}

	uint8_t* const split = FindSplit(first);

	if (!split)
		return;

	Anchors::Record("Stage card split", reinterpret_cast<uintptr_t>(split), "stage cards");

	uint8_t patch[sizeof(Cards::kDivide)] = {};
	BuildSplit(patch);

	if (!CodePatch::Write(split, patch, sizeof(patch)))
	{
		Refuse("could not patch the card split");
		return;
	}

	g_patched = true;
	sprintf_s(g_status, "Cards past %d come from stage_thumb01, using all of its rows.", Cards::kPerSheet - 1);
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
