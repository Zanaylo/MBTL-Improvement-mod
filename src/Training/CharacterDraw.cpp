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

using ObjectType_t = int(__fastcall*)(void*, void*);

void* oObjectDraw = nullptr;

bool g_hooked = false;
volatile LONG g_seenTypes = 0;
const char* g_status = "does not work in this game version";

constexpr int kTrackedTypes = 8;

bool IsTypeCall(const uint8_t* at)
{
	return at[0] == Draw::kCallSlot && (at[1] >> 6) == 1 && ((at[1] >> 3) & 7) == Draw::kCallSlotDigit &&
		at[2] == Draw::kTypeSlot * sizeof(uint32_t);
}

bool ComparesEffectType(const uint8_t* after)
{
	const uint8_t compare[] = { Draw::kCompareEax[0], Draw::kCompareEax[1], static_cast<uint8_t>(Draw::kEffectType) };

	return ImageScanner::Contains(after, Draw::kTypeCompareWindow, compare, sizeof(compare));
}

bool ChecksOwnerActive(const uint8_t* function, size_t length)
{
	for (size_t i = 0; i + Draw::kActiveTestLength <= length; ++i)
	{
		if (function[i] != Draw::kCompareByte || (function[i + 1] & 0xC0) != 0x80 ||
			((function[i + 1] >> 3) & 7) != Draw::kCompareDigit)
		{
			continue;
		}

		if (std::memcmp(function + i + 2, Draw::kActiveDisplacement, sizeof(Draw::kActiveDisplacement)) == 0 &&
			function[i + Draw::kActiveTestLength - 1] == 0)
		{
			return true;
		}
	}

	return false;
}

uint8_t* Resolve()
{
	const uint8_t* const update = BattleMap::Addresses().battleUpdate;

	if (!update)
		return nullptr;

	const std::vector<uint8_t*> drawn = ImageScanner::CallTargets(update);
	const ImageSection code = ImageScanner::Code();

	std::vector<uint8_t*> walkers;
	std::vector<uint8_t*> found;

	for (size_t i = 0; i + Draw::kTypeCallLength + Draw::kTypeCompareWindow < code.size; ++i)
	{
		uint8_t* const at = code.begin + i;

		if (!IsTypeCall(at) || !ComparesEffectType(at + Draw::kTypeCallLength))
			continue;

		uint8_t* const walker = ImageScanner::FunctionStart(at);

		if (!walker || std::find(walkers.begin(), walkers.end(), walker) != walkers.end())
			continue;

		walkers.push_back(walker);

		const size_t length = ImageScanner::FunctionLength(walker);
		const std::vector<uint8_t*> calls = ImageScanner::CallTargets(walker);

		if (length == 0 || calls.size() != 1 || !ChecksOwnerActive(walker, length))
			continue;
		if (std::find(drawn.begin(), drawn.end(), calls.front()) == drawn.end())
			continue;

		found.push_back(calls.front());
	}

	if (found.size() == 1)
		return found.front();

	LOG("CharacterDraw: the draw list walker has %u candidate(s), expected exactly one",
		static_cast<unsigned>(found.size()));
	return nullptr;
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

int __cdecl ChooseDraw(int draw, void* object)
{
	if (!g_settings.hideCharacters || object == nullptr || draw == 0)
		return draw;

	if (!GameState::IsOnlineKnown() || GameState::IsOnline() || !Hides(object))
		return draw;

	return 0;
}

__declspec(naked) void HookedObjectDraw()
{
	__asm
	{
		push ebp
		mov ebp, esp
		push ecx
		push dword ptr [ebp + 8]
		movzx eax, dl
		push eax
		call ChooseDraw
		add esp, 8
		pop ecx
		movzx edx, al
		pop ebp
		jmp dword ptr [oObjectDraw]
	}
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

	g_hooked = HookManager::CreateHook(draw, reinterpret_cast<void*>(&HookedObjectDraw), &oObjectDraw, "object draw");

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
