#include "Game/SceneMap.h"

#include "Core/logger.h"
#include "Game/Anchors.h"
#include "Game/GameOffsets.h"
#include "Hooks/ImageScanner.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace {

namespace Scenes = GameOffsets::Scenes;

SceneAddresses g_addresses;

uint8_t* ResolveStep()
{
	const std::vector<uint8_t*> steps =
		ImageScanner::FunctionsReferencing(ImageScanner::FindWideString(Scenes::kStepAnchor));

	if (steps.size() != 1)
	{
		LOG("SceneMap: the scene step has %u candidate(s), expected exactly one", static_cast<unsigned>(steps.size()));
		return nullptr;
	}

	const std::vector<uint8_t*> sites = ImageScanner::CallersOf(steps.front());

	if (sites.size() != 1 ||
		std::memcmp(sites.front() + Scenes::kCallLength, Scenes::kOneArgumentCleanup, sizeof(Scenes::kOneArgumentCleanup)) != 0)
	{
		LOG("SceneMap: the scene step is not a one-argument cdecl with a single caller");
		return nullptr;
	}

	return steps.front();
}

bool ReadRequest(const uint8_t* at, SceneAddresses& out)
{
	if (at[0] != Scenes::kPushByte ||
		std::memcmp(at + Scenes::kMovEcxEaxAt, Scenes::kMovEcxEax, sizeof(Scenes::kMovEcxEax)) != 0)
	{
		return false;
	}

	const uint8_t* const getter = ImageScanner::CallTargetOf(at + Scenes::kGetterCallAt);
	uint8_t* const request = ImageScanner::CallTargetOf(at + Scenes::kRequestCallAt);
	const uintptr_t manager = getter ? ImageScanner::GetterValue(getter) : 0;

	if (manager == 0 || request == nullptr || !ImageScanner::ReturnsWith(request, Scenes::kRequestStackBytes))
		return false;

	out.manager = manager;
	out.request = request;
	out.titleFlag = static_cast<int8_t>(at[1]);
	return true;
}

bool RequestWithin(const uint8_t* from, const uint8_t* end, SceneAddresses& out)
{
	for (const uint8_t* at = from; at + Scenes::kRequestSequenceLength <= end && at < from + Scenes::kRequestWindow; ++at)
	{
		if (ReadRequest(at, out))
			return true;
	}

	return false;
}

bool RequestIn(const uint8_t* function, const std::vector<uint8_t*>& strings, SceneAddresses& out)
{
	const size_t length = ImageScanner::FunctionLength(function);

	for (const uint8_t* text : strings)
	{
		const uint8_t* const after = ImageScanner::AfterPushOf(function, length, text);

		if (after != nullptr && RequestWithin(after, function + length, out))
			return true;
	}

	return false;
}

void ResolveRequest()
{
	const std::vector<uint8_t*> strings = ImageScanner::FindString(Scenes::kReturnTitleAnchor);
	SceneAddresses found;
	int matches = 0;

	for (uint8_t* function : ImageScanner::FunctionsReferencing(strings))
		matches += RequestIn(function, strings, found) ? 1 : 0;

	if (matches != 1)
	{
		LOG("SceneMap: a scene request follows ReturnTitle in %d function(s), expected exactly one", matches);
		return;
	}

	g_addresses.manager = found.manager;
	g_addresses.request = found.request;
	g_addresses.titleFlag = found.titleFlag;
}

uintptr_t ResolveEntering(const uint8_t* request)
{
	std::vector<uintptr_t> globals;

	for (uint8_t* target : ImageScanner::CallTargets(request))
	{
		if (!ImageScanner::InCode(target, Scenes::kEnteringSetterLength) ||
			std::memcmp(target, Scenes::kEnteringSetterHead, sizeof(Scenes::kEnteringSetterHead)) != 0 ||
			std::memcmp(target + Scenes::kEnteringSetterTailAt, Scenes::kEnteringSetterTail,
				sizeof(Scenes::kEnteringSetterTail)) != 0)
		{
			continue;
		}

		globals.push_back(ImageScanner::ReadDword(target + sizeof(Scenes::kEnteringSetterHead)));
	}

	if (globals.size() == 1 && ImageScanner::InData(globals.front()))
		return globals.front();

	LOG("SceneMap: the scene entry flag has %u candidate(s), expected exactly one", static_cast<unsigned>(globals.size()));
	return 0;
}

bool SetsSceneId(const uint8_t* setter)
{
	const size_t length = (std::min)(ImageScanner::FunctionLength(setter), Scenes::kSceneSetterWindow);

	return length != 0 && ImageScanner::Contains(setter, length, Scenes::kSceneStore, sizeof(Scenes::kSceneStore));
}

int ResolveTitleScene(const uint8_t* request)
{
	const size_t length = ImageScanner::FunctionLength(request);
	std::vector<int> scenes;

	for (size_t i = 0; i + Scenes::kSceneSetSequenceLength <= length; ++i)
	{
		if (request[i] != Scenes::kPushByte ||
			std::memcmp(request + i + Scenes::kLoadThisAt, Scenes::kLoadThis, sizeof(Scenes::kLoadThis)) != 0)
		{
			continue;
		}

		const uint8_t* const setter = ImageScanner::CallTargetOf(request + i + Scenes::kSceneSetCallAt);

		if (setter != nullptr && SetsSceneId(setter))
			scenes.push_back(static_cast<int8_t>(request[i + 1]));
	}

	if (scenes.size() == 1)
		return scenes.front();

	LOG("SceneMap: the title scene has %u candidate(s), expected exactly one", static_cast<unsigned>(scenes.size()));
	return -1;
}

bool ReadCountdown(const uint8_t* at, uintptr_t& object, uintptr_t& offset)
{
	if (!ImageScanner::InCode(at, Scenes::kCountdownSequenceLength) ||
		at[Scenes::kCountdownLimitAt] != Scenes::kCountdownLimit || at[Scenes::kJumpAboveAt] != Scenes::kJumpAbove ||
		at[Scenes::kJumpShortAt] != Scenes::kJumpShort || at[Scenes::kLoadEcxAt] != Scenes::kLoadEcx ||
		at[Scenes::kCountdownCallAt] != Scenes::kCall)
	{
		return false;
	}

	object = ImageScanner::ReadDword(at + Scenes::kCountdownObjectAt);
	offset = ImageScanner::ReadDword(at + Scenes::kCountdownOffsetAt);
	return ImageScanner::InData(object);
}

void ResolveReplayCountdown()
{
	uintptr_t object = 0;
	uintptr_t offset = 0;
	int matches = 0;

	for (const uint8_t* at : ImageScanner::FindBytes(ImageScanner::Code(), Scenes::kCountdownCompare,
		sizeof(Scenes::kCountdownCompare)))
	{
		uintptr_t candidateObject = 0;
		uintptr_t candidateOffset = 0;

		if (!ReadCountdown(at, candidateObject, candidateOffset))
			continue;

		object = candidateObject;
		offset = candidateOffset;
		++matches;
	}

	if (matches != 1)
	{
		LOG("SceneMap: the replay check countdown has %d candidate(s), expected exactly one", matches);
		return;
	}

	g_addresses.replayChecker = object;
	g_addresses.replayCountdown = offset;
}

}

bool SceneMap::Initialize()
{
	if (!ImageScanner::Initialize())
		return false;

	g_addresses.step = ResolveStep();
	ResolveRequest();
	ResolveReplayCountdown();

	if (g_addresses.request != nullptr)
	{
		g_addresses.entering = ResolveEntering(g_addresses.request);
		g_addresses.titleScene = ResolveTitleScene(g_addresses.request);
	}

	Anchors::Record("Scene step", reinterpret_cast<uintptr_t>(g_addresses.step), "restart the game");
	Anchors::Record("Back to the title", reinterpret_cast<uintptr_t>(g_addresses.request), "restart the game");
	Anchors::Record("Scene manager", g_addresses.manager, "the scene this launch started on");
	Anchors::Record("Scene entry flag", g_addresses.entering, "restart the game");
	Anchors::Record("Replay check", g_addresses.replayChecker, "the loading screen after a restart");

	LOG("SceneMap: the title is scene %d, reached with flag %d; replay check countdown at +0x%X",
		g_addresses.titleScene, g_addresses.titleFlag, static_cast<unsigned>(g_addresses.replayCountdown));
	return g_addresses.step != nullptr && g_addresses.request != nullptr && g_addresses.entering != 0 &&
		g_addresses.titleScene >= 0 && g_addresses.replayChecker != 0;
}

const SceneAddresses& SceneMap::Addresses()
{
	return g_addresses;
}
