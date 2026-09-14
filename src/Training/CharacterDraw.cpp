#include "Training/CharacterDraw.h"

#include "Core/Settings.h"
#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Game/Anchors.h"
#include "Game/GameOffsets.h"
#include "Hooks/HookManager.h"
#include "Hooks/ImageScanner.h"
#include "Training/BattleMap.h"
#include "Training/GameState.h"

#include <windows.h>

#include <algorithm>
#include <cstring>
#include <vector>

namespace {

namespace Draw = GameOffsets::Draw;

using ObjectDraw_t = int(__cdecl*)(int, int, void*);
using ObjectType_t = int(__fastcall*)(void*, void*);

constexpr int kFlagMask = 0xFF;
constexpr int kTrackedTypes = 32;

ObjectDraw_t oObjectDraw = nullptr;

bool g_hooked = false;
volatile LONG g_seenTypes = 0;
const char* g_status = "does not work in this game version";

std::vector<uint8_t*> Unique(const std::vector<uint8_t*>& list)
{
	std::vector<uint8_t*> unique;

	for (uint8_t* value : list)
	{
		if (std::find(unique.begin(), unique.end(), value) == unique.end())
			unique.push_back(value);
	}

	return unique;
}

bool HasActiveTest(const uint8_t* function, size_t length)
{
	for (size_t i = 0; i + Draw::kActiveDisplacementAt + sizeof(Draw::kActiveDisplacement) <= length; ++i)
	{
		if (std::memcmp(function + i, Draw::kMovzx, sizeof(Draw::kMovzx)) == 0 &&
			std::memcmp(function + i + Draw::kActiveDisplacementAt, Draw::kActiveDisplacement,
				sizeof(Draw::kActiveDisplacement)) == 0)
		{
			return true;
		}
	}

	return false;
}

bool IsWalker(const uint8_t* function)
{
	const size_t length = ImageScanner::FunctionLength(function);

	return ImageScanner::Contains(function, length, Draw::kTypeTwoCompare, sizeof(Draw::kTypeTwoCompare)) &&
		HasActiveTest(function, length);
}

uint8_t* ResolveWalker(const uint8_t* update)
{
	std::vector<uint8_t*> walkers;

	for (uint8_t* target : Unique(ImageScanner::CallsCleanedBy(update, Draw::kTwoArgumentCleanup,
		sizeof(Draw::kTwoArgumentCleanup))))
	{
		if (IsWalker(target))
			walkers.push_back(target);
	}

	if (walkers.size() == 1)
		return walkers.front();

	LOG("CharacterDraw: the draw list walker has %u candidate(s), expected exactly one",
		static_cast<unsigned>(walkers.size()));
	return nullptr;
}

uint8_t* Resolve()
{
	const uint8_t* const step = BattleMap::Addresses().battleStep;
	const std::vector<uint8_t*> stepCalls = step ? ImageScanner::CallSequence(step) : std::vector<uint8_t*>();

	if (stepCalls.empty())
		return nullptr;

	const uint8_t* const walker = ResolveWalker(stepCalls.back());

	if (!walker)
		return nullptr;

	const std::vector<uint8_t*> draws = Unique(ImageScanner::CallsCleanedBy(walker, Draw::kThreeArgumentCleanup,
		sizeof(Draw::kThreeArgumentCleanup)));

	if (draws.size() != 1)
	{
		LOG("CharacterDraw: the object draw has %u candidate(s), expected exactly one", static_cast<unsigned>(draws.size()));
		return nullptr;
	}

	if (!ImageScanner::Contains(draws.front(), ImageScanner::FunctionLength(draws.front()), Draw::kDrawFlagReturn,
		sizeof(Draw::kDrawFlagReturn)))
	{
		LOG("CharacterDraw: the object draw does not return early on its draw flag");
		return nullptr;
	}

	return draws.front();
}

int TypeOf(void* object)
{
	void** const vtable = *static_cast<void***>(object);
	return reinterpret_cast<ObjectType_t>(vtable[Draw::kTypeSlot])(object, nullptr);
}

void NoteType(int type)
{
	if (type < 0 || type >= kTrackedTypes)
		return;

	const LONG bit = static_cast<LONG>(1u << type);

	if ((InterlockedOr(&g_seenTypes, bit) & bit) == 0)
		LOG("CharacterDraw: first object of type %d while hiding", type);
}

bool Hides(void* object)
{
	const int type = TypeOf(object);
	NoteType(type);

	return type == Draw::kCharacterType || (type == Draw::kEffectType && g_settings.hideEffects);
}

int __cdecl HookedObjectDraw(int sim, int draw, void* object)
{
	if (!g_settings.hideCharacters || object == nullptr || (draw & kFlagMask) == 0)
		return oObjectDraw(sim, draw, object);

	if (!GameState::IsOnlineKnown() || GameState::IsOnline() || !Hides(object))
		return oObjectDraw(sim, draw, object);

	return oObjectDraw(sim, 0, object);
}

}

void CharacterDraw::Install()
{
	uint8_t* const draw = Resolve();

	Anchors::Record("Object draw", reinterpret_cast<uintptr_t>(draw), "hide the characters");

	if (!draw)
	{
		LOG("CharacterDraw: %s", g_status);
		return;
	}

	g_hooked = HookManager::CreateHook(draw, reinterpret_cast<void*>(&HookedObjectDraw),
		reinterpret_cast<void**>(&oObjectDraw), "object draw");

	g_status = g_hooked ? "" : "could not start";
	LOG("CharacterDraw: %s", g_hooked ? "ready" : g_status);
}

bool CharacterDraw::IsAvailable()
{
	return g_hooked;
}

bool CharacterDraw::IsHidden()
{
	return g_settings.hideCharacters;
}

void CharacterDraw::SetHidden(bool hidden)
{
	g_settings.hideCharacters = hidden;
	Settings::SaveBool("Training", "HideCharacters", hidden);
}

void CharacterDraw::Toggle()
{
	SetHidden(!IsHidden());
}

bool CharacterDraw::EffectsHidden()
{
	return g_settings.hideEffects;
}

void CharacterDraw::SetEffectsHidden(bool hidden)
{
	g_settings.hideEffects = hidden;
	Settings::SaveBool("Training", "HideEffects", hidden);
}

const char* CharacterDraw::StatusText()
{
	return g_status;
}
