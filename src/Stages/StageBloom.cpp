#include "Stages/StageBloom.h"

#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Game/Anchors.h"
#include "Game/GameOffsets.h"
#include "Hooks/HookManager.h"
#include "Hooks/ImageScanner.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace {

namespace Bloom = GameOffsets::Bloom;

using BloomPass_t = int(__cdecl*)(int, int, int, int, int);

BloomPass_t oBloomPass = nullptr;

float* g_brightness = nullptr;
float* g_alpha = nullptr;
bool g_hooked = false;
const char* g_status = "not found in this game version";

float Scaled(float value, float scale, float most)
{
	return (std::min)(value * scale, most);
}

int __cdecl HookedBloomPass(int first, int second, int third, int fourth, int fifth)
{
	const int percent = std::clamp(g_settings.bloomStrength, 0, Bloom::kMostPercent);

	if (percent == Bloom::kFullPercent)
		return oBloomPass(first, second, third, fourth, fifth);

	const float scale = static_cast<float>(percent) / Bloom::kFullPercent;
	const float brightness = *g_brightness;
	const float alpha = *g_alpha;

	*g_brightness = Scaled(brightness, scale, Bloom::kMostBrightness);
	*g_alpha = Scaled(alpha, scale, Bloom::kMostAlpha);

	const int result = oBloomPass(first, second, third, fourth, fifth);

	*g_brightness = brightness;
	*g_alpha = alpha;
	return result;
}

uint8_t* ResolvePass(const std::vector<uint8_t*>& names)
{
	const std::vector<uint8_t*> passes = ImageScanner::FunctionsReferencing(names);

	if (passes.size() != 1)
	{
		LOG("StageBloom: the bloom pass has %u candidate(s), expected exactly one", static_cast<unsigned>(passes.size()));
		return nullptr;
	}

	const std::vector<uint8_t*> sites = ImageScanner::CallersOf(passes.front());

	if (sites.size() != 1 ||
		std::memcmp(sites.front() + Bloom::kCallLength, Bloom::kFiveArgumentCleanup, sizeof(Bloom::kFiveArgumentCleanup)) != 0)
	{
		LOG("StageBloom: the bloom pass is not a five-argument cdecl with a single caller");
		return nullptr;
	}

	return passes.front();
}

float* ResolveBrightness(const uint8_t* pass, size_t length, const uint8_t* name)
{
	const uint8_t* const after = ImageScanner::AfterPushOf(pass, length, name);
	const uint8_t* const previous = after ? after - 2 * Bloom::kPushLength : nullptr;

	if (previous == nullptr || previous < pass || previous[0] != Bloom::kPushImmediate)
	{
		LOG("StageBloom: the brightness global is not pushed before its name");
		return nullptr;
	}

	const uintptr_t global = ImageScanner::ReadDword(previous + 1);
	return ImageScanner::InData(global) ? reinterpret_cast<float*>(global) : nullptr;
}

float* ResolveAlpha(const uint8_t* pass, size_t length)
{
	std::vector<uintptr_t> globals;
	const size_t window = (std::min)(length, Bloom::kAlphaWindow);

	for (size_t i = 0; i + Bloom::kScalarReadLength <= window; ++i)
	{
		if (std::memcmp(pass + i, Bloom::kLoadScalar, sizeof(Bloom::kLoadScalar)) != 0 ||
			std::memcmp(pass + i + Bloom::kMultiplyScalarAt, Bloom::kMultiplyScalar, sizeof(Bloom::kMultiplyScalar)) != 0)
		{
			continue;
		}

		globals.push_back(ImageScanner::ReadDword(pass + i + sizeof(Bloom::kLoadScalar)));
	}

	if (globals.size() == 1 && ImageScanner::InData(globals.front()))
		return reinterpret_cast<float*>(globals.front());

	LOG("StageBloom: the bloom alpha has %u candidate(s), expected exactly one", static_cast<unsigned>(globals.size()));
	return nullptr;
}

}

void StageBloom::Install()
{
	const std::vector<uint8_t*> names = ImageScanner::FindString(Bloom::kBrightnessName);
	uint8_t* const pass = ResolvePass(names);
	const size_t length = pass ? ImageScanner::FunctionLength(pass) : 0;

	g_brightness = pass ? ResolveBrightness(pass, length, names.front()) : nullptr;
	g_alpha = pass ? ResolveAlpha(pass, length) : nullptr;

	Anchors::Record("Stage bloom pass", reinterpret_cast<uintptr_t>(pass), "bloom strength");
	Anchors::Record("Stage bloom brightness", reinterpret_cast<uintptr_t>(g_brightness), "bloom strength");
	Anchors::Record("Stage bloom alpha", reinterpret_cast<uintptr_t>(g_alpha), "bloom strength");

	if (!pass || !g_brightness || !g_alpha)
	{
		LOG("StageBloom: %s", g_status);
		return;
	}

	g_hooked = HookManager::CreateHook(pass, reinterpret_cast<void*>(&HookedBloomPass),
		reinterpret_cast<void**>(&oBloomPass), "stage bloom pass");

	g_status = g_hooked ? "" : "could not be turned on";
	LOG("StageBloom: %s", g_hooked ? "ready" : g_status);
}

bool StageBloom::IsAvailable()
{
	return g_hooked;
}

const char* StageBloom::StatusText()
{
	return g_status;
}
