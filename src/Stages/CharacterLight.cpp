#include "Stages/CharacterLight.h"

#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Game/Anchors.h"
#include "Game/GameOffsets.h"
#include "Hooks/HookManager.h"
#include "Hooks/ImageScanner.h"

#include <algorithm>
#include <map>
#include <vector>

namespace {

namespace Light = GameOffsets::Light;

using ColourGetter_t = int(__cdecl*)(int, int, float*, float*);
using AlphaGetter_t = int(__cdecl*)(int, int, float*);

struct Getters
{
	uint8_t* colour;
	uint8_t* specular;
	uint8_t* bokashi;
};

ColourGetter_t oColour = nullptr;
ColourGetter_t oSpecular = nullptr;
AlphaGetter_t oBokashi = nullptr;

bool g_hooked = false;
const char* g_status = "the character light getters were not found in this build";

int Percent()
{
	return std::clamp(g_settings.lightStrength, 0, Light::kFullPercent);
}

float Strength()
{
	return static_cast<float>(Percent()) / Light::kFullPercent;
}

void Blend(float* values, int count, float target, float strength)
{
	if (values == nullptr)
		return;

	for (int i = 0; i < count; ++i)
		values[i] = target + (values[i] - target) * strength;
}

int __cdecl HookedColour(int bank, int x, float* base, float* height)
{
	const int result = oColour(bank, x, base, height);

	if (Percent() == Light::kFullPercent)
		return result;

	const float strength = Strength();
	Blend(base, Light::kColourChannels, Light::kWhite, strength);
	Blend(height, Light::kColourChannels, Light::kNone, strength);
	return result;
}

int __cdecl HookedSpecular(int bank, int x, float* base, float* height)
{
	const int result = oSpecular(bank, x, base, height);

	if (Percent() == Light::kFullPercent)
		return result;

	const float strength = Strength();
	Blend(base, Light::kColourChannels, Light::kNone, strength);
	Blend(height, Light::kColourChannels, Light::kNone, strength);
	return result;
}

int __cdecl HookedBokashi(int bank, int x, float* alpha)
{
	const int result = oBokashi(bank, x, alpha);

	if (Percent() != Light::kFullPercent)
		Blend(alpha, 1, Light::kNone, Strength());

	return result;
}

uint8_t* BuilderOf(const std::vector<uint8_t*>& getters)
{
	std::map<uint8_t*, int> counts;

	for (uint8_t* getter : getters)
	{
		for (uint8_t* caller : ImageScanner::CallerFunctions(getter))
			++counts[caller];
	}

	uint8_t* best = nullptr;
	int bestCount = 0;
	bool tied = false;

	for (const std::pair<uint8_t* const, int>& entry : counts)
	{
		tied = tied || entry.second == bestCount;

		if (entry.second <= bestCount)
			continue;

		best = entry.first;
		bestCount = entry.second;
		tied = false;
	}

	if (best != nullptr && !tied && bestCount >= Light::kLeastBuilderCalls)
		return best;

	LOG("CharacterLight: no single function builds the draw context (%d getter call(s) at most)", bestCount);
	return nullptr;
}

std::vector<uint8_t*> Among(const std::vector<uint8_t*>& calls, const std::vector<uint8_t*>& getters)
{
	std::vector<uint8_t*> kept;

	for (uint8_t* call : calls)
	{
		if (std::find(getters.begin(), getters.end(), call) != getters.end())
			kept.push_back(call);
	}

	return kept;
}

bool Resolve(Getters& out)
{
	const std::vector<uint8_t*> getters = ImageScanner::FunctionsReferencing(ImageScanner::FindString(Light::kBankAnchor));
	const uint8_t* const builder = BuilderOf(getters);

	if (!builder)
		return false;

	const std::vector<uint8_t*> pairs = Among(ImageScanner::CallsCleanedBy(builder, Light::kFourArgumentCleanup,
		sizeof(Light::kFourArgumentCleanup)), getters);
	const std::vector<uint8_t*> singles = Among(ImageScanner::CallsCleanedBy(builder, Light::kThreeArgumentCleanup,
		sizeof(Light::kThreeArgumentCleanup)), getters);

	if (pairs.size() != Light::kPairGetters || singles.size() != Light::kSingleGetters)
	{
		LOG("CharacterLight: the draw context builder calls %u colour and %u alpha getter(s), expected 2 and 1",
			static_cast<unsigned>(pairs.size()), static_cast<unsigned>(singles.size()));
		return false;
	}

	out = { pairs[0], pairs[1], singles[0] };
	return true;
}

}

void CharacterLight::Install()
{
	Getters getters = {};
	const bool resolved = Resolve(getters);

	Anchors::Record("Character colour getter", reinterpret_cast<uintptr_t>(getters.colour), "character light");
	Anchors::Record("Character specular getter", reinterpret_cast<uintptr_t>(getters.specular), "character light");
	Anchors::Record("Character bokashi getter", reinterpret_cast<uintptr_t>(getters.bokashi), "character light");

	if (!resolved)
	{
		LOG("CharacterLight: %s", g_status);
		return;
	}

	g_hooked = HookManager::CreateHook(getters.colour, reinterpret_cast<void*>(&HookedColour),
		reinterpret_cast<void**>(&oColour), "character colour getter") &&
		HookManager::CreateHook(getters.specular, reinterpret_cast<void*>(&HookedSpecular),
			reinterpret_cast<void**>(&oSpecular), "character specular getter") &&
		HookManager::CreateHook(getters.bokashi, reinterpret_cast<void*>(&HookedBokashi),
			reinterpret_cast<void**>(&oBokashi), "character bokashi getter");

	g_status = g_hooked ? "" : "the character light getters could not be hooked";
	LOG("CharacterLight: %s", g_hooked ? "ready" : g_status);
}

bool CharacterLight::IsAvailable()
{
	return g_hooked;
}

const char* CharacterLight::StatusText()
{
	return g_status;
}
