#include "Game/SceneMap.h"

#include "Core/logger.h"
#include "Game/Anchors.h"
#include "Game/GameOffsets.h"
#include "Hooks/ImageScanner.h"

#include <algorithm>
#include <cstring>
#include <map>
#include <vector>

namespace {

namespace Scenes = GameOffsets::Scenes;

SceneAddresses g_addresses;

struct Store
{
	size_t at = 0;
	uintptr_t address = 0;
	uint32_t value = 0;
};

bool ReadStore(const uint8_t* function, size_t length, size_t at, Store& out)
{
	if (at + Scenes::kStoreLength > length)
		return false;
	if (std::memcmp(function + at, Scenes::kStoreGlobal, sizeof(Scenes::kStoreGlobal)) != 0)
		return false;

	out.at = at;
	out.address = ImageScanner::ReadDword(function + at + Scenes::kStoreAddressAt);
	out.value = ImageScanner::ReadDword(function + at + Scenes::kStoreValueAt);
	return ImageScanner::InData(out.address);
}

bool ReadTitleRun(const uint8_t* function, size_t length, size_t at, Store run[Scenes::kTitleStores])
{
	for (size_t i = 0; i < Scenes::kTitleStores; ++i)
	{
		if (!ReadStore(function, length, at + i * Scenes::kStoreLength, run[i]))
			return false;
	}

	if (run[0].value != 0 || run[1].value != 0 || run[2].value != 0 || run[3].value == 0)
		return false;

	return run[1].address == run[0].address + sizeof(uint32_t) &&
		run[2].address == run[3].address + sizeof(uint32_t);
}

uintptr_t VoteForEntering(uintptr_t sceneId)
{
	uint8_t pattern[Scenes::kStoreAddressAt + sizeof(uint32_t)] = { Scenes::kStoreGlobal[0], Scenes::kStoreGlobal[1] };
	const auto address = static_cast<uint32_t>(sceneId);
	std::memcpy(pattern + Scenes::kStoreAddressAt, &address, sizeof(address));

	std::map<uintptr_t, int> votes;

	for (const uint8_t* site : ImageScanner::FindBytes(ImageScanner::Code(), pattern, sizeof(pattern)))
	{
		for (size_t i = Scenes::kStoreLength; i < Scenes::kEnteringWindow; ++i)
		{
			if (!ImageScanner::InCode(site + i, Scenes::kStoreLength))
				break;
			if (std::memcmp(site + i, Scenes::kStoreGlobal, sizeof(Scenes::kStoreGlobal)) != 0)
				continue;

			const uintptr_t candidate = ImageScanner::ReadDword(site + i + Scenes::kStoreAddressAt);

			if (candidate != sceneId && ImageScanner::InData(candidate))
				++votes[candidate];

			break;
		}
	}

	int best = 0;
	int second = 0;
	uintptr_t winner = 0;

	for (const std::pair<const uintptr_t, int>& vote : votes)
	{
		if (vote.second > best)
		{
			second = best;
			best = vote.second;
			winner = vote.first;
			continue;
		}

		if (vote.second > second)
			second = vote.second;
	}

	if (best < Scenes::kLeastEnteringVotes || best < second * Scenes::kEnteringMajority)
		return 0;

	return winner;
}

bool ReadTitleFlag(const uint8_t* function, size_t length, const Store& last, uintptr_t entering, int& flag)
{
	size_t at = last.at + Scenes::kStoreLength;

	if (at + Scenes::kJumpLength <= length && function[at] == Scenes::kJump)
	{
		const int32_t relative = static_cast<int32_t>(ImageScanner::ReadDword(function + at + 1));
		const uint8_t* const target = function + at + Scenes::kJumpLength + relative;

		if (target < function || target >= function + length)
			return false;

		at = static_cast<size_t>(target - function);
	}

	Store store;

	if (!ReadStore(function, length, at, store) || store.address != entering)
		return false;

	flag = static_cast<int>(store.value);
	return true;
}

void ResolveTitle()
{
	int matches = 0;
	SceneAddresses found;

	for (uint8_t* function : ImageScanner::FunctionsReferencing(ImageScanner::FindString(Scenes::kReturnTitleAnchor)))
	{
		const size_t length = ImageScanner::FunctionLength(function);

		for (size_t at = 0; at + Scenes::kTitleStores * Scenes::kStoreLength <= length; ++at)
		{
			Store run[Scenes::kTitleStores];

			if (!ReadTitleRun(function, length, at, run))
				continue;

			const uintptr_t entering = VoteForEntering(run[3].address);
			int flag = 0;

			if (entering == 0 || !ReadTitleFlag(function, length, run[3], entering, flag))
				continue;

			found.sceneId = run[3].address;
			found.sceneReturn = run[2].address;
			found.battleClear = run[0].address;
			found.manager = run[3].address - Scenes::kSceneId;
			found.entering = entering;
			found.titleScene = static_cast<int>(run[3].value);
			found.titleFlag = flag;
			++matches;
		}
	}

	if (matches != 1)
	{
		LOG("SceneMap: the way back to the title was found %d time(s), expected exactly one", matches);
		return;
	}

	g_addresses = found;
}

size_t JumpTableCases(uintptr_t table)
{
	size_t cases = 0;

	while (cases < Scenes::kMostSceneCases)
	{
		const auto slot = reinterpret_cast<const uint8_t*>(table + cases * sizeof(uint32_t));

		if (!ImageScanner::InData(reinterpret_cast<uintptr_t>(slot)) &&
			!ImageScanner::InCode(slot, sizeof(uint32_t)))
		{
			break;
		}

		const auto target = reinterpret_cast<const uint8_t*>(ImageScanner::ReadDword(slot));

		if (!ImageScanner::InCode(target, 1))
			break;

		++cases;
	}

	return cases;
}

bool SwitchesOverScenes(const uint8_t* function, size_t length)
{
	for (size_t i = 0; i + Scenes::kJumpTableLength <= length; ++i)
	{
		if (std::memcmp(function + i, Scenes::kJumpTable, sizeof(Scenes::kJumpTable)) != 0)
			continue;

		const uintptr_t table = ImageScanner::ReadDword(function + i + Scenes::kJumpTableAddressAt);

		if (JumpTableCases(table) >= Scenes::kLeastSceneCases)
			return true;
	}

	return false;
}

uint8_t* ResolveStep(uintptr_t sceneId)
{
	if (sceneId == 0)
		return nullptr;

	std::vector<uint8_t*> matches;

	for (uint8_t* function :
		ImageScanner::FunctionsReferencing({ reinterpret_cast<uint8_t*>(sceneId) }))
	{
		const size_t length = ImageScanner::FunctionLength(function);

		if (length != 0 && SwitchesOverScenes(function, length))
			matches.push_back(function);
	}

	if (matches.size() == 1)
		return matches.front();

	LOG("SceneMap: the scene step has %u candidate(s), expected exactly one", static_cast<unsigned>(matches.size()));
	return nullptr;
}

}

bool SceneMap::Initialize()
{
	if (!ImageScanner::Initialize())
		return false;

	ResolveTitle();
	g_addresses.step = ResolveStep(g_addresses.sceneId);

	Anchors::Record("Scene step", reinterpret_cast<uintptr_t>(g_addresses.step), "restart the game");
	Anchors::Record("Scene manager", g_addresses.manager, "the scene this launch started on");
	Anchors::Record("Scene entry flag", g_addresses.entering, "restart the game");

	LOG("SceneMap: the title is scene %d, reached with flag %d", g_addresses.titleScene, g_addresses.titleFlag);
	return g_addresses.step != nullptr && g_addresses.entering != 0;
}

const SceneAddresses& SceneMap::Addresses()
{
	return g_addresses;
}
