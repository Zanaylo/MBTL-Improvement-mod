#include "Performance/RenderMap.h"

#include "Core/logger.h"
#include "Game/Anchors.h"
#include "Game/GameOffsets.h"
#include "Hooks/ImageScanner.h"

#include <algorithm>
#include <cstring>
#include <set>
#include <vector>

namespace {

namespace Render = GameOffsets::Render;

RenderAddresses g_addresses;

uintptr_t ByteGetterValue(const uint8_t* function)
{
	if (!ImageScanner::InCode(function, Render::kByteGetterLength))
		return 0;

	if (std::memcmp(function, Render::kByteGetterHead, sizeof(Render::kByteGetterHead)) != 0 ||
		std::memcmp(function + Render::kByteGetterTailAt, Render::kByteGetterTail, sizeof(Render::kByteGetterTail)) != 0)
	{
		return 0;
	}

	const uintptr_t value = ImageScanner::ReadDword(function + sizeof(Render::kByteGetterHead));
	return ImageScanner::InData(value) ? value : 0;
}

int GlobalCompares(const uint8_t* function, size_t length)
{
	int count = 0;

	for (size_t i = 0; i + Render::kGlobalCompareLength <= length; ++i)
	{
		const bool compare = std::memcmp(function + i, Render::kGlobalCompare, sizeof(Render::kGlobalCompare)) == 0 &&
			function[i + Render::kGlobalCompareLength - 1] == 0;

		count += compare ? 1 : 0;
	}

	return count;
}

bool IsStageGate(const uint8_t* function)
{
	const size_t length = ImageScanner::FunctionLength(function);

	if (length == 0 || length > Render::kGateMaxLength)
		return false;

	return GlobalCompares(function, length) == Render::kGateCompares &&
		ImageScanner::Contains(function, length, Render::kReturnTrue, sizeof(Render::kReturnTrue));
}

uint8_t* ResolveStageDraw()
{
	const std::vector<uint8_t*> specular = ImageScanner::FunctionsReferencing(ImageScanner::FindString(Render::kStageSpecularAnchor));
	std::vector<uint8_t*> matches;

	for (uint8_t* function : ImageScanner::FunctionsReferencing(ImageScanner::FindString(Render::kStageDrawAnchor)))
	{
		if (std::find(specular.begin(), specular.end(), function) == specular.end())
			continue;

		const std::vector<uint8_t*> calls = ImageScanner::CallSequence(function);

		if (!calls.empty() && IsStageGate(calls.front()))
			matches.push_back(function);
	}

	if (matches.size() == 1)
		return matches.front();

	LOG("RenderMap: the stage draw has %u candidate(s), expected exactly one", static_cast<unsigned>(matches.size()));
	return nullptr;
}

uintptr_t ResolveFxaa(const uint8_t* stageDraw)
{
	std::set<uintptr_t> values;

	for (uint8_t* target : ImageScanner::CallTargets(stageDraw))
	{
		const uintptr_t value = ByteGetterValue(target);

		if (value)
			values.insert(value);
	}

	if (values.size() == 1)
		return *values.begin();

	LOG("RenderMap: the stage FXAA switch has %u candidate(s), expected exactly one", static_cast<unsigned>(values.size()));
	return 0;
}

uintptr_t ResolveMultisample()
{
	std::vector<uint8_t*> functions;

	for (uint8_t* site : ImageScanner::FindBytes(ImageScanner::Code(), Render::kStretchRectCall, sizeof(Render::kStretchRectCall)))
	{
		uint8_t* const start = ImageScanner::FunctionStart(site);

		if (start && std::find(functions.begin(), functions.end(), start) == functions.end())
			functions.push_back(start);
	}

	if (functions.size() != 1)
	{
		LOG("RenderMap: the stage resolve has %u candidate(s), expected exactly one", static_cast<unsigned>(functions.size()));
		return 0;
	}

	const std::vector<uint8_t*> calls = ImageScanner::CallSequence(functions.front());
	return calls.empty() ? 0 : ByteGetterValue(calls.front());
}

}

bool RenderMap::Initialize()
{
	if (!ImageScanner::Initialize())
		return false;

	const uint8_t* const stageDraw = ResolveStageDraw();

	g_addresses.stageGate = stageDraw ? ImageScanner::CallSequence(stageDraw).front() : nullptr;
	g_addresses.stageFxaa = stageDraw ? ResolveFxaa(stageDraw) : 0;
	g_addresses.stageMultisample = ResolveMultisample();

	Anchors::Record("Stage ready check", reinterpret_cast<uintptr_t>(g_addresses.stageGate), "empty stage");
	Anchors::Record("Stage multisampling", g_addresses.stageMultisample, "POTATO MODE plain stage");
	Anchors::Record("Stage FXAA", g_addresses.stageFxaa, "POTATO MODE plain stage");

	return g_addresses.stageGate != nullptr;
}

const RenderAddresses& RenderMap::Addresses()
{
	return g_addresses;
}
