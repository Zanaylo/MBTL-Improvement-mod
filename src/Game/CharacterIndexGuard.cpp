#include "Game/CharacterIndexGuard.h"

#include "Core/logger.h"
#include "Game/Anchors.h"
#include "Game/AssertSites.h"
#include "Game/GameOffsets.h"
#include "Hooks/CodePatch.h"
#include "Hooks/ImageScanner.h"

#include <algorithm>
#include <cstring>
#include <cwchar>
#include <iterator>
#include <vector>

namespace {

namespace Asserts = GameOffsets::Asserts;

constexpr size_t kPlaceBytes = 128;

bool IsCharacterCheck(const AssertSite& site)
{
	if (site.bound < 0)
		return false;

	return std::any_of(std::begin(Asserts::kCharacterChecks), std::end(Asserts::kCharacterChecks),
		[&site](const Asserts::CharacterCheck& check)
		{
			return AssertSites::FileIs(site.file, check.file) && std::wcscmp(site.expression, check.expression) == 0;
		});
}

std::vector<uint8_t*> UnlockedColourGetters(const std::vector<AssertSite>& sites)
{
	std::vector<uint8_t*> getters;

	for (const AssertSite& site : sites)
	{
		if (!AssertSites::FileIs(site.file, Asserts::kSaveDataFile) ||
			std::wcscmp(site.expression, Asserts::kUnlockedColourCheck) != 0)
		{
			continue;
		}

		uint8_t* const function = ImageScanner::FunctionStart(site.block);

		if (function && ImageScanner::ReturnsWith(function, Asserts::kColourGetterStackBytes))
			getters.push_back(function);
	}

	return getters;
}

bool Patch(const AssertSite& site, const uint8_t* exit, uint32_t result)
{
	uint8_t bytes[Asserts::kLongestBlock];
	std::memset(bytes, Asserts::kFill, sizeof(bytes));

	const auto distance = static_cast<int32_t>(exit - (site.block + Asserts::kPatchLength));
	bytes[0] = Asserts::kMoveEax;
	std::memcpy(bytes + 1, &result, sizeof(result));
	bytes[Asserts::kJumpOpcodeAt] = Asserts::kJump;
	std::memcpy(bytes + Asserts::kJumpOpcodeAt + 1, &distance, sizeof(distance));

	return CodePatch::Write(site.block, bytes, site.length);
}

bool Guard(const AssertSite& site, const std::vector<uint8_t*>& colourGetters)
{
	char place[kPlaceBytes] = {};
	AssertSites::Describe(site.file, site.line, place, sizeof(place));

	uint8_t* const function = ImageScanner::FunctionStart(site.block);
	const uint8_t* const exit = function ? ImageScanner::Epilogue(function) : nullptr;

	if (!exit || static_cast<size_t>(site.block - function) > Asserts::kEntryWindow ||
		site.length < Asserts::kPatchLength || site.length > Asserts::kLongestBlock)
	{
		LOG("CharacterIndexGuard: %s is not a plain entry check, left alone", place);
		return false;
	}

	const bool unlocked = std::find(colourGetters.begin(), colourGetters.end(), function) != colourGetters.end();
	const uint32_t result = unlocked ? 1 : 0;

	if (!Patch(site, exit, result))
	{
		LOG("CharacterIndexGuard: %s could not be patched", place);
		return false;
	}

	Anchors::Record(place, reinterpret_cast<uintptr_t>(site.block),
		unlocked ? "added characters read as unlocked" : "added characters return 0");
	LOG("CharacterIndexGuard: %s returns %u for character numbers from %d", place, result, site.bound);
	return true;
}

}

void CharacterIndexGuard::Install()
{
	const std::vector<AssertSite> sites = AssertSites::Find();

	if (sites.empty())
	{
		LOG("CharacterIndexGuard: no assert sites were found, nothing is guarded");
		return;
	}

	const std::vector<uint8_t*> colourGetters = UnlockedColourGetters(sites);
	size_t matched = 0;
	size_t guarded = 0;

	for (const AssertSite& site : sites)
	{
		if (!IsCharacterCheck(site))
			continue;

		++matched;
		guarded += Guard(site, colourGetters) ? 1 : 0;
	}

	LOG("CharacterIndexGuard: %u of %u character checks guarded (%u assert sites, %u colour getter)",
		static_cast<unsigned>(guarded), static_cast<unsigned>(matched),
		static_cast<unsigned>(sites.size()), static_cast<unsigned>(colourGetters.size()));
}
