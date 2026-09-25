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

bool Matches(const uint8_t* at, const uint8_t* bytes, size_t count)
{
	return std::memcmp(at, bytes, count) == 0;
}

int GlobalCompares(const uint8_t* function, size_t length)
{
	int count = 0;

	for (size_t i = 0; i + Render::kGlobalCompareLength <= length; ++i)
	{
		const bool compare = Matches(function + i, Render::kGlobalCompare, sizeof(Render::kGlobalCompare)) &&
			function[i + Render::kGlobalCompareLength - 1] == 0;

		count += compare ? 1 : 0;
	}

	return count;
}

uint8_t* StageSkip(uint8_t* function)
{
	const size_t length = ImageScanner::FunctionLength(function);
	const size_t window = std::min(length, Render::kGateWindow);
	uint8_t* branch = nullptr;
	int branches = 0;

	for (size_t i = 0; i + sizeof(Render::kSkipBranch) + sizeof(uint32_t) <= window; ++i)
	{
		if (!Matches(function + i, Render::kSkipBranch, sizeof(Render::kSkipBranch)))
			continue;

		branch = function + i + Render::kSkipTestLength;
		++branches;
	}

	if (branches != 1 || GlobalCompares(function, static_cast<size_t>(branch - function)) < Render::kGateCompares)
		return nullptr;

	const auto relative = static_cast<int32_t>(ImageScanner::ReadDword(branch + Render::kSkipOpcodeLength));
	const uint8_t* const target = branch + Render::kSkipBranchLength + relative;

	return target > branch && target < function + length ? branch : nullptr;
}

uint8_t* ResolveStageDraw()
{
	const std::vector<uint8_t*> specular = ImageScanner::FunctionsReferencing(ImageScanner::FindString(Render::kStageSpecularAnchor));
	std::vector<uint8_t*> matches;

	for (uint8_t* function : ImageScanner::FunctionsReferencing(ImageScanner::FindString(Render::kStageDrawAnchor)))
	{
		if (std::find(specular.begin(), specular.end(), function) == specular.end())
			continue;

		if (StageSkip(function))
			matches.push_back(function);
	}

	if (matches.size() == 1)
		return matches.front();

	LOG("RenderMap: the stage draw has %u candidate(s), expected exactly one", static_cast<unsigned>(matches.size()));
	return nullptr;
}

uintptr_t ResolveFxaa(const uint8_t* stageDraw)
{
	const size_t length = ImageScanner::FunctionLength(stageDraw);
	std::set<uintptr_t> values;

	for (size_t i = 0; i + Render::kCompareByteLength <= length; ++i)
	{
		if (!Matches(stageDraw + i, Render::kCompareByteGlobal, sizeof(Render::kCompareByteGlobal)) ||
			stageDraw[i + Render::kCompareByteLength - 1] != 0)
		{
			continue;
		}

		values.insert(ImageScanner::ReadDword(stageDraw + i + sizeof(Render::kCompareByteGlobal)));
	}

	if (values.size() == 1 && ImageScanner::InData(*values.begin()))
		return *values.begin();

	LOG("RenderMap: the stage FXAA switch has %u candidate(s), expected exactly one", static_cast<unsigned>(values.size()));
	return 0;
}

bool IsStretchRectCall(const uint8_t* at)
{
	return at[0] == Render::kCallMemory && at[1] >= Render::kCallDisp32First && at[1] <= Render::kCallDisp32Last &&
		Matches(at + 2, Render::kStretchRectSlot, sizeof(Render::kStretchRectSlot));
}

uintptr_t ResolveSourceSurface(const uint8_t* stageDraw)
{
	const size_t length = ImageScanner::FunctionLength(stageDraw);

	for (size_t call = 0; call + Render::kStretchRectLength <= length; ++call)
	{
		if (!IsStretchRectCall(stageDraw + call))
			continue;

		const size_t first = call > Render::kPushWindow ? call - Render::kPushWindow : 0;

		for (size_t at = call; at > first;)
		{
			--at;

			if (Matches(stageDraw + at, Render::kPushGlobal, sizeof(Render::kPushGlobal)))
				return ImageScanner::ReadDword(stageDraw + at + sizeof(Render::kPushGlobal));
		}

		return 0;
	}

	return 0;
}

uint8_t* SampleRead(uint8_t* function, size_t length)
{
	uint8_t* found = nullptr;
	int reads = 0;

	for (size_t i = 0; i + Render::kLoadSamplesLength + sizeof(Render::kTestSamples) <= length; ++i)
	{
		if (!Matches(function + i, Render::kLoadSamples, sizeof(Render::kLoadSamples)))
			continue;

		const size_t last = std::min(length - sizeof(Render::kTestSamples),
			i + Render::kLoadSamplesLength + Render::kTestSamplesWindow);

		for (size_t at = i + Render::kLoadSamplesLength; at <= last; ++at)
		{
			if (!Matches(function + at, Render::kTestSamples, sizeof(Render::kTestSamples)))
				continue;

			found = function + i;
			++reads;
			break;
		}
	}

	return reads == 1 ? found : nullptr;
}

uintptr_t SampleStore(const uint8_t* function, size_t length, uintptr_t surface)
{
	std::set<uintptr_t> stores;

	for (size_t i = 0; i + Render::kStoreSamplesLength <= length; ++i)
	{
		if (!Matches(function + i, Render::kStoreSamples, sizeof(Render::kStoreSamples)))
			continue;

		const uintptr_t field = ImageScanner::ReadDword(function + i + sizeof(Render::kStoreSamples));

		if (field > surface && field <= surface + Render::kSurfaceFields)
			stores.insert(field);
	}

	return stores.size() == 1 ? *stores.begin() : 0;
}

void ResolveMultisample(const uint8_t* stageDraw)
{
	const uintptr_t surface = ResolveSourceSurface(stageDraw);

	if (!ImageScanner::InData(surface))
	{
		LOG("RenderMap: the stage resolve source was not found");
		return;
	}

	RenderAddresses found;
	int matches = 0;

	for (uint8_t* function : ImageScanner::FunctionsReferencing({ reinterpret_cast<uint8_t*>(surface) }))
	{
		const size_t length = ImageScanner::FunctionLength(function);
		uint8_t* const read = SampleRead(function, length);
		const uintptr_t samples = read ? SampleStore(function, length, surface) : 0;

		if (!samples)
			continue;

		found.stageSampleRead = read;
		found.stageSamples = samples;
		++matches;
	}

	if (matches != 1)
	{
		LOG("RenderMap: the stage multisampling setup has %d candidate(s), expected exactly one", matches);
		return;
	}

	g_addresses.stageSampleRead = found.stageSampleRead;
	g_addresses.stageSamples = found.stageSamples;
}

}

bool RenderMap::Initialize()
{
	if (!ImageScanner::Initialize())
		return false;

	uint8_t* const stageDraw = ResolveStageDraw();

	g_addresses.stageSkip = stageDraw ? StageSkip(stageDraw) : nullptr;
	g_addresses.stageFxaa = stageDraw ? ResolveFxaa(stageDraw) : 0;

	if (stageDraw)
		ResolveMultisample(stageDraw);

	Anchors::Record("Stage ready check", reinterpret_cast<uintptr_t>(g_addresses.stageSkip), "empty stage");
	Anchors::Record("Stage multisampling", reinterpret_cast<uintptr_t>(g_addresses.stageSampleRead),
		"POTATO MODE plain stage");
	Anchors::Record("Stage samples in use", g_addresses.stageSamples, "POTATO MODE plain stage");
	Anchors::Record("Stage FXAA", g_addresses.stageFxaa, "POTATO MODE plain stage");

	return g_addresses.stageSkip != nullptr;
}

const RenderAddresses& RenderMap::Addresses()
{
	return g_addresses;
}
