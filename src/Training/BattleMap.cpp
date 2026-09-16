#include "Training/BattleMap.h"

#include "Core/logger.h"
#include "Game/Anchors.h"
#include "Game/GameOffsets.h"
#include "Hooks/ImageScanner.h"

#include <algorithm>
#include <cstring>
#include <map>
#include <set>
#include <utility>
#include <vector>

namespace {

namespace Battle = GameOffsets::Battle;
namespace Objects = GameOffsets::Objects;
namespace CameraOffsets = GameOffsets::Camera;
namespace Meter = GameOffsets::Meter;

BattleAddresses g_addresses;

struct Indexed
{
	uint32_t stride = 0;
	uintptr_t base = 0;
};

uint8_t* Only(const std::vector<uint8_t*>& list, const char* what)
{
	if (list.size() == 1)
		return list.front();

	LOG("BattleMap: %s has %u candidate(s), expected exactly one", what, static_cast<unsigned>(list.size()));
	return nullptr;
}

uintptr_t OnlyValue(const std::set<uintptr_t>& values, const char* what)
{
	if (values.size() == 1)
		return *values.begin();

	LOG("BattleMap: %s has %u candidate(s), expected exactly one", what, static_cast<unsigned>(values.size()));
	return 0;
}

size_t ModRmLength(const uint8_t* modrm)
{
	const uint8_t mod = modrm[0] >> 6;
	const uint8_t rm = modrm[0] & 7;
	size_t length = 1;

	if (mod != 3 && rm == 4)
	{
		++length;

		if (mod == 0 && (modrm[1] & 7) == 5)
			length += 4;
	}

	if (mod == 1)
		return length + 1;

	if (mod == 2 || (mod == 0 && rm == 5))
		return length + 4;

	return length;
}

const uint8_t* NativeOf(const char* binding)
{
	const uint8_t* const native = ImageScanner::NativeFunction(binding);

	if (!native)
		LOG("BattleMap: the script native %s was not found", binding);

	return native;
}

uintptr_t AddedBase(const uint8_t* from)
{
	for (size_t k = 0; k < Objects::kStrideSearchWindow; ++k)
	{
		const uint8_t* const at = from + k;

		if (at[0] == Objects::kAddEax && ImageScanner::InData(ImageScanner::ReadDword(at + 1)))
			return ImageScanner::ReadDword(at + 1);

		if (at[0] == Objects::kAddGroup && (at[1] & 0xF8) == 0xC0 &&
			ImageScanner::InData(ImageScanner::ReadDword(at + 2)))
		{
			return ImageScanner::ReadDword(at + 2);
		}
	}

	return 0;
}

std::vector<Indexed> IndexedArrays(const uint8_t* function)
{
	std::vector<Indexed> found;

	if (!function)
		return found;

	const size_t length = ImageScanner::FunctionLength(function);

	for (size_t i = 0; i + Objects::kIndexerLength < length; ++i)
	{
		if (function[i] != Objects::kImul)
			continue;

		const uint8_t* const immediate = function + i + 1 + ModRmLength(function + i + 1);
		const uintptr_t base = AddedBase(immediate + sizeof(uint32_t));

		if (base == 0)
			continue;

		const Indexed entry = { ImageScanner::ReadDword(immediate), base };

		const bool seen = std::any_of(found.begin(), found.end(),
			[&entry](const Indexed& other) { return other.base == entry.base && other.stride == entry.stride; });

		if (!seen)
			found.push_back(entry);
	}

	return found;
}

void ResolveCharactersAndTeams()
{
	const std::vector<Indexed> arrays = IndexedArrays(NativeOf(Objects::kActivePlayerNative));

	if (arrays.size() != Objects::kIndexedArrays)
	{
		LOG("BattleMap: IsActivePlayer indexes %u array(s), expected %u", static_cast<unsigned>(arrays.size()),
			static_cast<unsigned>(Objects::kIndexedArrays));
		return;
	}

	if (arrays[0].stride <= arrays[1].stride)
	{
		LOG("BattleMap: the team record (0x%X) is not bigger than the character record (0x%X)", arrays[0].stride,
			arrays[1].stride);
		return;
	}

	g_addresses.teams = arrays[0].base;
	g_addresses.teamStride = arrays[0].stride;
	g_addresses.charaArray = arrays[1].base;
	g_addresses.charaStride = arrays[1].stride;
}

void ResolveCombos()
{
	std::vector<Indexed> arrays;

	for (const char* binding : Meter::kComboNatives)
	{
		for (const Indexed& entry : IndexedArrays(NativeOf(binding)))
		{
			const bool seen = std::any_of(arrays.begin(), arrays.end(),
				[&entry](const Indexed& other) { return other.base == entry.base && other.stride == entry.stride; });

			if (!seen)
				arrays.push_back(entry);
		}
	}

	if (arrays.size() != 1)
	{
		LOG("BattleMap: the combo records have %u candidate(s), expected exactly one",
			static_cast<unsigned>(arrays.size()));
		return;
	}

	g_addresses.combos = arrays.front().base + Meter::kComboRecordSkip;
	g_addresses.comboStride = arrays.front().stride;
}

uintptr_t ResolveBattleInfo()
{
	const uint8_t* const native = NativeOf(Battle::kTrainingNative);

	if (!native)
		return 0;

	const size_t length = ImageScanner::FunctionLength(native);
	std::set<uintptr_t> modes;

	for (size_t i = 0; i + Objects::kLoadEcxGlobalLength <= length; ++i)
	{
		if (std::memcmp(native + i, Objects::kLoadEcxGlobal, sizeof(Objects::kLoadEcxGlobal)) != 0)
			continue;

		const uintptr_t mode = ImageScanner::ReadDword(native + i + sizeof(Objects::kLoadEcxGlobal));

		if (ImageScanner::InData(mode))
			modes.insert(mode);
	}

	const uintptr_t mode = OnlyValue(modes, "the battle mode");

	if (mode == 0)
		return 0;

	const auto subMode = static_cast<uint32_t>(mode + (Battle::kSubMode - Battle::kMode));

	if (!ImageScanner::Contains(native, length, reinterpret_cast<const uint8_t*>(&subMode), sizeof(subMode)))
	{
		LOG("BattleMap: IsTrainingBattle does not read the sub-mode next to the mode");
		return 0;
	}

	return mode - Battle::kMode;
}

uintptr_t ResolveCamera()
{
	const uint8_t* const native = NativeOf(CameraOffsets::kPositionNative);

	if (!native)
		return 0;

	const size_t length = ImageScanner::FunctionLength(native);
	std::set<uintptr_t> globals;

	for (size_t i = 0; i + 1 + sizeof(uint32_t) <= length; ++i)
	{
		if (native[i] != CameraOffsets::kLoadEaxGlobal)
			continue;

		const uintptr_t value = ImageScanner::ReadDword(native + i + 1);

		if (ImageScanner::InData(value))
			globals.insert(value);
	}

	if (globals.size() != static_cast<size_t>(CameraOffsets::kElementCount))
	{
		LOG("BattleMap: the camera has %u element(s), expected %d", static_cast<unsigned>(globals.size()),
			CameraOffsets::kElementCount);
		return 0;
	}

	uintptr_t previous = 0;

	for (uintptr_t value : globals)
	{
		if (previous != 0 && value - previous != CameraOffsets::kElementBytes)
		{
			LOG("BattleMap: the camera elements are not %u bytes apart",
				static_cast<unsigned>(CameraOffsets::kElementBytes));
			return 0;
		}

		previous = value;
	}

	return *globals.begin() - CameraOffsets::kElementX;
}

uintptr_t ResolveEffectList()
{
	std::set<uintptr_t> lists;

	for (uint8_t* at : ImageScanner::FindBytes(ImageScanner::Code(), Objects::kEffectSpawnStore,
		sizeof(Objects::kEffectSpawnStore)))
	{
		if (at[-2] != Objects::kStoreByte || (at[-1] & 0xF8) != 0x80 || (at[-1] & 7) == 4)
			continue;

		for (size_t k = sizeof(Objects::kEffectSpawnStore); k + sizeof(Objects::kCountUp) + 4 < Objects::kEffectListWindow; ++k)
		{
			if (std::memcmp(at + k, Objects::kCountUp, sizeof(Objects::kCountUp)) != 0)
				continue;

			const uintptr_t list = ImageScanner::ReadDword(at + k + sizeof(Objects::kCountUp));

			if (ImageScanner::InData(list))
				lists.insert(list);

			break;
		}
	}

	return OnlyValue(lists, "effect list");
}

std::vector<uint8_t*> BattleTicks()
{
	const uint8_t* const create = Only(ImageScanner::FunctionsReferencing(ImageScanner::FindString(Battle::kStepAnchor)),
		"BattleProc Create");

	std::vector<uint8_t*> ticks;

	if (!create)
		return ticks;

	for (uint8_t* site : ImageScanner::CallersOf(create))
	{
		uint8_t* const function = ImageScanner::FunctionStart(site);

		if (function && std::find(ticks.begin(), ticks.end(), function) == ticks.end())
			ticks.push_back(function);
	}

	return ticks;
}

uint8_t* ResolveBattleUpdate(const uint8_t* create)
{
	if (!create)
		return nullptr;

	std::map<uint8_t*, int> votes;

	for (uint8_t* site : ImageScanner::CallersOf(create))
	{
		for (size_t i = Battle::kCallLength; i < Battle::kUpdateWindow; ++i)
		{
			uint8_t* const target = ImageScanner::CallTargetOf(site + i);

			if (!target)
				continue;

			++votes[target];
			break;
		}
	}

	const auto best = std::max_element(votes.begin(), votes.end(),
		[](const std::pair<uint8_t* const, int>& a, const std::pair<uint8_t* const, int>& b) { return a.second < b.second; });

	if (best != votes.end() && best->second >= Battle::kLeastUpdateVotes)
		return best->first;

	LOG("BattleMap: no consistent battle update after BattleProc Create (%d)", best == votes.end() ? 0 : best->second);
	return nullptr;
}

uintptr_t ResolveSession(const std::vector<uint8_t*>& ticks)
{
	std::map<uintptr_t, int> counts;

	for (uint8_t* tick : ticks)
	{
		const size_t length = ImageScanner::FunctionLength(tick);
		std::set<uintptr_t> seen;

		for (size_t i = 0; i + Battle::kNullTestLength <= length; ++i)
		{
			if (std::memcmp(tick + i, Battle::kLoadEcxGlobal, sizeof(Battle::kLoadEcxGlobal)) != 0)
				continue;
			if (std::memcmp(tick + i + Battle::kNullTestAt, Battle::kTestEcx, sizeof(Battle::kTestEcx)) != 0)
				continue;

			const uintptr_t global = ImageScanner::ReadDword(tick + i + sizeof(Battle::kLoadEcxGlobal));

			if (ImageScanner::InData(global))
				seen.insert(global);
		}

		for (uintptr_t global : seen)
			++counts[global];
	}

	std::set<uintptr_t> sessions;

	for (const std::pair<const uintptr_t, int>& entry : counts)
	{
		if (entry.second >= Battle::kLeastSessionTicks)
			sessions.insert(entry.first);
	}

	return OnlyValue(sessions, "the online session");
}

uintptr_t ResolvePause()
{
	const uint8_t* const printer = Only(ImageScanner::FunctionsReferencing(ImageScanner::FindString(Battle::kPauseAnchor)),
		"PLAYER %d PAUSE");

	if (!printer)
		return 0;

	std::map<uintptr_t, int> counts;

	for (uint8_t* caller : ImageScanner::CallerFunctions(printer))
	{
		const size_t length = ImageScanner::FunctionLength(caller);

		for (size_t i = 0; i + 1 + sizeof(uint32_t) <= length; ++i)
		{
			if (caller[i] != Battle::kLoadEcxImmediate)
				continue;

			const uintptr_t object = ImageScanner::ReadDword(caller + i + 1);

			if (ImageScanner::InData(object))
				++counts[object];
		}
	}

	int best = 0;
	int second = 0;
	uintptr_t winner = 0;

	for (const std::pair<const uintptr_t, int>& entry : counts)
	{
		if (entry.second > best)
		{
			second = best;
			best = entry.second;
			winner = entry.first;
			continue;
		}

		if (entry.second > second)
			second = entry.second;
	}

	if (best >= Battle::kPauseLeastLoads && best >= second * Battle::kPauseMajority)
		return winner;

	LOG("BattleMap: the pause controller loads no object often enough (%d)", best);
	return 0;
}

}

bool BattleMap::Initialize()
{
	if (!ImageScanner::Initialize())
		return false;

	const std::vector<uint8_t*> ticks = BattleTicks();
	const uint8_t* const create = ticks.empty()
		? nullptr
		: ImageScanner::FunctionsReferencing(ImageScanner::FindString(Battle::kStepAnchor)).front();

	g_addresses.battleUpdate = ResolveBattleUpdate(create);
	g_addresses.battleInfo = ResolveBattleInfo();
	g_addresses.session = ResolveSession(ticks);
	g_addresses.pause = ResolvePause();
	ResolveCharactersAndTeams();
	g_addresses.effectList = ResolveEffectList();
	g_addresses.camera = ResolveCamera();
	ResolveCombos();

	Anchors::Record("Battle update", reinterpret_cast<uintptr_t>(g_addresses.battleUpdate), "pause and next frame");
	Anchors::Record("Battle info", g_addresses.battleInfo, "mode and frame counter");
	Anchors::Record("GGPO session", g_addresses.session, "online check");
	Anchors::Record("Game pause", g_addresses.pause, "the game's own pause menu");
	Anchors::Record("Character array", g_addresses.charaArray, "hitboxes");
	Anchors::Record("Effect list", g_addresses.effectList, "projectile hitboxes");
	Anchors::Record("Camera", g_addresses.camera, "hitbox placement");
	Anchors::Record("Team records", g_addresses.teams, "frame meter players");
	Anchors::Record("Combo records", g_addresses.combos, "frame meter combo count");

	LOG("BattleMap: character stride 0x%X, team stride 0x%X, combo stride 0x%X", g_addresses.charaStride,
		g_addresses.teamStride, g_addresses.comboStride);

	return g_addresses.battleInfo != 0 && g_addresses.charaArray != 0;
}

const BattleAddresses& BattleMap::Addresses()
{
	return g_addresses;
}
