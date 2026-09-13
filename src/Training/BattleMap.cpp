#include "Training/BattleMap.h"

#include "Core/logger.h"
#include "Game/Anchors.h"
#include "Game/GameOffsets.h"
#include "Hooks/ImageScanner.h"

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

constexpr uint8_t kLoadEcx = 0xB9;
constexpr uint8_t kCall = 0xE8;
constexpr size_t kLoadLength = 5;
constexpr size_t kLoadAndCallLength = 10;
constexpr uint8_t kImul = 0x69;
constexpr uint8_t kAddEax = 0x05;
constexpr uint8_t kAddGroup = 0x81;
constexpr uint8_t kMovByte = 0xC6;
constexpr uint8_t kCompareGroup = 0x83;
constexpr uint8_t kAbsoluteOperand = 0x3D;
constexpr size_t kCompareLength = 7;

BattleAddresses g_addresses;

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

uint8_t* ResolveBattleStep()
{
	const uint8_t* const create = Only(ImageScanner::FunctionsReferencing(ImageScanner::FindString(Battle::kStepAnchor)),
		"BattleProc Create");

	if (!create)
		return nullptr;

	std::vector<uint8_t*> matches;

	for (uint8_t* caller : ImageScanner::CallerFunctions(create))
	{
		const size_t length = ImageScanner::FunctionLength(caller);

		if (length == 0 || length > Battle::kStepMaxLength)
			continue;

		if (std::memcmp(caller, Battle::kStepPrologue, sizeof(Battle::kStepPrologue)) != 0)
			continue;

		matches.push_back(caller);
	}

	return Only(matches, "BattleStep");
}

uintptr_t ObjectBehindNative(const char* binding)
{
	const uint8_t* const native = ImageScanner::NativeFunction(binding);

	if (!native)
	{
		LOG("BattleMap: the script native %s was not found", binding);
		return 0;
	}

	const std::vector<uint8_t*> calls = ImageScanner::CallSequence(native);
	return calls.empty() ? 0 : ImageScanner::GetterValue(calls.front());
}

uintptr_t ResolveSession()
{
	const uint8_t* const user = Only(ImageScanner::FunctionsReferencing(ImageScanner::FindWideString(Battle::kSessionAnchor)),
		"p_session");

	if (!user)
		return 0;

	const uint8_t* const caller = Only(ImageScanner::CallerFunctions(user), "p_session caller");

	if (!caller)
		return 0;

	for (size_t i = 0; i < Battle::kSessionCompareWindow; ++i)
	{
		const uint8_t* const at = caller + i;

		if (at[0] == kCompareGroup && at[1] == kAbsoluteOperand && at[kCompareLength - 1] == 0)
			return ImageScanner::ReadDword(at + 2);
	}

	LOG("BattleMap: the session check has no compare against a global");
	return 0;
}

uintptr_t ResolvePause()
{
	const uint8_t* const printer = Only(ImageScanner::FunctionsReferencing(ImageScanner::FindString(Battle::kPauseAnchor)),
		"PLAYER %d PAUSE");

	if (!printer)
		return 0;

	uintptr_t best = 0;
	int bestCount = 0;

	for (uint8_t* caller : ImageScanner::CallerFunctions(printer))
	{
		std::map<uintptr_t, int> counts;
		const size_t length = ImageScanner::FunctionLength(caller);

		for (size_t i = 0; i + 5 <= length; ++i)
		{
			if (caller[i] == kLoadEcx)
				++counts[ImageScanner::ReadDword(caller + i + 1)];
		}

		for (const std::pair<const uintptr_t, int>& entry : counts)
		{
			if (entry.second <= bestCount)
				continue;

			best = entry.first;
			bestCount = entry.second;
		}
	}

	if (bestCount >= Battle::kPauseLeastLoads)
		return best;

	LOG("BattleMap: the pause controller loads no object often enough (%d)", bestCount);
	return 0;
}

uintptr_t AddedBase(const uint8_t* from)
{
	for (size_t k = 0; k < Objects::kStrideSearchWindow; ++k)
	{
		const uint8_t* const at = from + k;

		if (at[0] == kAddEax && ImageScanner::InData(ImageScanner::ReadDword(at + 1)))
			return ImageScanner::ReadDword(at + 1);

		if (at[0] == kAddGroup && (at[1] & 0xF8) == 0xC0 && ImageScanner::InData(ImageScanner::ReadDword(at + 2)))
			return ImageScanner::ReadDword(at + 2);
	}

	return 0;
}

uintptr_t ResolveCharaArray(uint32_t& stride)
{
	const uint8_t* const native = ImageScanner::NativeFunction(Objects::kActivePlayerNative);

	if (!native)
	{
		LOG("BattleMap: the script native %s was not found", Objects::kActivePlayerNative);
		return 0;
	}

	const size_t length = ImageScanner::FunctionLength(native);
	std::set<std::pair<uint32_t, uintptr_t>> pairs;

	for (size_t i = 0; i + 1 + 6 + 4 + Objects::kStrideSearchWindow + 6 < length; ++i)
	{
		if (native[i] != kImul)
			continue;

		const uint8_t* const immediate = native + i + 1 + ModRmLength(native + i + 1);
		const uint32_t value = ImageScanner::ReadDword(immediate);

		if (value < Objects::kLeastStride || value > Objects::kMostStride)
			continue;

		const uintptr_t base = AddedBase(immediate + 4);

		if (base)
			pairs.insert({ value, base });
	}

	if (pairs.size() != 1)
	{
		LOG("BattleMap: the character array has %u candidate(s), expected exactly one", static_cast<unsigned>(pairs.size()));
		return 0;
	}

	stride = pairs.begin()->first;
	return pairs.begin()->second;
}

uintptr_t ResolveEffectList()
{
	std::set<uintptr_t> lists;
	const size_t storeLength = sizeof(Objects::kEffectSpawnStore);

	for (uint8_t* at : ImageScanner::FindBytes(ImageScanner::Code(), Objects::kEffectSpawnStore, storeLength))
	{
		if (at[-2] != kMovByte || (at[-1] & 0xF8) != 0x80 || (at[-1] & 7) == 4)
			continue;

		for (size_t k = storeLength; k + 2 < Objects::kEffectListWindow; ++k)
		{
			if (at[k] != kLoadEcx || !ImageScanner::InData(ImageScanner::ReadDword(at + k + 1)))
				continue;

			lists.insert(ImageScanner::ReadDword(at + k + 1));
			break;
		}
	}

	return OnlyValue(lists, "effect list");
}

uintptr_t ResolveCamera()
{
	const uint8_t* const native = ImageScanner::NativeFunction(CameraOffsets::kPositionNative);

	if (!native)
	{
		LOG("BattleMap: the script native %s was not found", CameraOffsets::kPositionNative);
		return 0;
	}

	std::set<uintptr_t> cameras;

	for (uint8_t* target : ImageScanner::CallTargets(native))
	{
		const std::vector<uint8_t*> inner = ImageScanner::CallSequence(target);

		if (inner.empty())
			continue;

		const uintptr_t value = ImageScanner::GetterValue(inner.front());

		if (value)
			cameras.insert(value);
	}

	return OnlyValue(cameras, "camera");
}

uintptr_t ResolveTeams(uint32_t& stride)
{
	const uint8_t* const native = ImageScanner::NativeFunction(Meter::kTeamNative);

	if (!native)
	{
		LOG("BattleMap: the script native %s was not found", Meter::kTeamNative);
		return 0;
	}

	std::set<std::pair<uint32_t, uintptr_t>> found;

	for (uint8_t* target : ImageScanner::CallTargets(native))
	{
		if (!ImageScanner::InCode(target, Meter::kTeamIndexerLength))
			continue;

		if (std::memcmp(target, Meter::kTeamIndexerHead, sizeof(Meter::kTeamIndexerHead)) != 0 ||
			target[Meter::kTeamIndexerAddAt] != Meter::kTeamIndexerAdd ||
			std::memcmp(target + Meter::kTeamIndexerTailAt, Meter::kTeamIndexerTail, sizeof(Meter::kTeamIndexerTail)) != 0)
		{
			continue;
		}

		found.insert({ ImageScanner::ReadDword(target + Meter::kTeamIndexerStrideAt),
			ImageScanner::ReadDword(target + Meter::kTeamIndexerBaseAt) });
	}

	if (found.size() != 1)
	{
		LOG("BattleMap: the team records have %u candidate(s), expected exactly one", static_cast<unsigned>(found.size()));
		return 0;
	}

	stride = found.begin()->first;
	return found.begin()->second;
}

const uint8_t* FindInFunction(const uint8_t* function, const uint8_t* bytes, size_t count)
{
	const size_t length = ImageScanner::FunctionLength(function);

	for (size_t i = 0; i + count + 4 <= length; ++i)
	{
		if (std::memcmp(function + i, bytes, count) == 0)
			return function + i + count;
	}

	return nullptr;
}

bool ReadComboIndexer(const uint8_t* function, uint32_t& stride, uint32_t& offset)
{
	if (!ImageScanner::InCode(function, 1))
		return false;

	const uint8_t* const strideAt = FindInFunction(function, Meter::kComboStride, sizeof(Meter::kComboStride));
	const uint8_t* const offsetAt = FindInFunction(function, Meter::kComboOffset, sizeof(Meter::kComboOffset));

	if (!strideAt || !offsetAt)
		return false;

	stride = ImageScanner::ReadDword(strideAt);
	offset = ImageScanner::ReadDword(offsetAt);
	return true;
}

void CollectComboRecords(const uint8_t* native, std::set<std::pair<uint32_t, uintptr_t>>& found)
{
	const size_t length = ImageScanner::FunctionLength(native);

	for (size_t i = 0; i + kLoadAndCallLength <= length; ++i)
	{
		if (native[i] != kLoadEcx || native[i + kLoadLength] != kCall)
			continue;

		const uintptr_t base = ImageScanner::ReadDword(native + i + 1);

		if (!ImageScanner::InData(base))
			continue;

		const int32_t displacement = static_cast<int32_t>(ImageScanner::ReadDword(native + i + kLoadLength + 1));
		const uint8_t* const callee = native + i + kLoadAndCallLength + displacement;

		uint32_t stride = 0;
		uint32_t offset = 0;

		if (ReadComboIndexer(callee, stride, offset))
			found.insert({ stride, base + offset + Meter::kComboRecordSkip });
	}
}

uintptr_t ResolveCombos(uint32_t& stride)
{
	std::set<std::pair<uint32_t, uintptr_t>> found;

	for (const char* binding : Meter::kComboNatives)
	{
		const uint8_t* const native = ImageScanner::NativeFunction(binding);

		if (!native)
		{
			LOG("BattleMap: the script native %s was not found", binding);
			continue;
		}

		CollectComboRecords(native, found);
	}

	if (found.size() != 1)
	{
		LOG("BattleMap: the combo records have %u candidate(s), expected exactly one", static_cast<unsigned>(found.size()));
		return 0;
	}

	stride = found.begin()->first;
	return found.begin()->second;
}

}

bool BattleMap::Initialize()
{
	if (!ImageScanner::Initialize())
		return false;

	g_addresses.battleStep = ResolveBattleStep();
	g_addresses.battleInfo = ObjectBehindNative(Battle::kTrainingNative);
	g_addresses.session = ResolveSession();
	g_addresses.pause = ResolvePause();
	g_addresses.charaArray = ResolveCharaArray(g_addresses.charaStride);
	g_addresses.effectList = ResolveEffectList();
	g_addresses.camera = ResolveCamera();
	g_addresses.teams = ResolveTeams(g_addresses.teamStride);
	g_addresses.combos = ResolveCombos(g_addresses.comboStride);

	Anchors::Record("BattleStep", reinterpret_cast<uintptr_t>(g_addresses.battleStep), "pause and next frame");
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
	return g_addresses.battleStep != nullptr;
}

const BattleAddresses& BattleMap::Addresses()
{
	return g_addresses;
}
