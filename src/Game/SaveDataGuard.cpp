#include "Game/SaveDataGuard.h"

#include "Core/logger.h"
#include "Game/Anchors.h"
#include "Game/GameOffsets.h"
#include "Hooks/HookManager.h"
#include "Hooks/ImageScanner.h"

#include <windows.h>

#include <cstring>
#include <vector>

namespace {

namespace SaveData = GameOffsets::SaveData;

using IncrementCount_t = void(__fastcall*)(void*, void*, int, uint32_t);
using SetCount_t = void(__fastcall*)(void*, void*, int, uint32_t, uint32_t);

struct GuardedCounter
{
	const wchar_t* anchor;
	const char* name;
	uint16_t stackBytes;
	uint8_t* function;
	int types;
	volatile LONG skipped;
};

IncrementCount_t oIncrementCount = nullptr;
SetCount_t oSetCount = nullptr;

GuardedCounter g_increment = { SaveData::kIncrementAnchor, "AchievementCountIncrement_Base", SaveData::kIncrementStackBytes };
GuardedCounter g_set = { SaveData::kSetAnchor, "SetAchievementCount_Base", SaveData::kSetStackBytes };

bool Skips(GuardedCounter& counter, int type)
{
	if (type == SaveData::kSharedType || (type >= 0 && type < counter.types))
		return false;

	if (InterlockedIncrement(&counter.skipped) == 1)
		LOG("SaveDataGuard: %s got character type %d but the save data holds %d, skipped", counter.name, type,
			counter.types);

	return true;
}

void __fastcall HookedIncrementCount(void* self, void* unused, int type, uint32_t slot)
{
	if (Skips(g_increment, type))
		return;

	oIncrementCount(self, unused, type, slot);
}

void __fastcall HookedSetCount(void* self, void* unused, int type, uint32_t slot, uint32_t value)
{
	if (Skips(g_set, type))
		return;

	oSetCount(self, unused, type, slot, value);
}

int TypeBound(const uint8_t* function, size_t length)
{
	const size_t boundAt = SaveData::kTypeUpperCheckAt + sizeof(SaveData::kTypeUpperCheck);

	for (size_t i = 0; i + boundAt + 2 <= length; ++i)
	{
		if (std::memcmp(function + i, SaveData::kTypeLowerCheck, sizeof(SaveData::kTypeLowerCheck)) != 0 ||
			std::memcmp(function + i + SaveData::kTypeUpperCheckAt, SaveData::kTypeUpperCheck,
				sizeof(SaveData::kTypeUpperCheck)) != 0 ||
			function[i + boundAt + 1] != SaveData::kJumpLess)
		{
			continue;
		}

		return static_cast<int8_t>(function[i + boundAt]);
	}

	return 0;
}

bool Resolve(GuardedCounter& counter)
{
	const std::vector<uint8_t*> functions =
		ImageScanner::FunctionsReferencing(ImageScanner::FindWideString(counter.anchor));

	if (functions.size() != 1)
	{
		LOG("SaveDataGuard: %s has %u candidate(s), expected one", counter.name,
			static_cast<unsigned>(functions.size()));
		return false;
	}

	const uint8_t* const function = functions.front();

	if (!ImageScanner::ReturnsWith(function, counter.stackBytes))
	{
		LOG("SaveDataGuard: %s does not end with ret 0x%X", counter.name, counter.stackBytes);
		return false;
	}

	const int types = TypeBound(function, ImageScanner::FunctionLength(function));

	if (types <= 0)
	{
		LOG("SaveDataGuard: the character type bound of %s was not found", counter.name);
		return false;
	}

	counter.function = functions.front();
	counter.types = types;
	return true;
}

void Guard(GuardedCounter& counter, void* detour, void** original)
{
	const bool resolved = Resolve(counter);

	Anchors::Record(counter.name, reinterpret_cast<uintptr_t>(counter.function), "new characters skip save counters");

	if (!resolved)
		return;

	if (!HookManager::CreateHook(counter.function, detour, original, counter.name))
	{
		LOG("SaveDataGuard: %s could not be hooked", counter.name);
		return;
	}

	LOG("SaveDataGuard: %s accepts character types 0..%d", counter.name, counter.types - 1);
}

}

void SaveDataGuard::Install()
{
	Guard(g_increment, reinterpret_cast<void*>(&HookedIncrementCount), reinterpret_cast<void**>(&oIncrementCount));
	Guard(g_set, reinterpret_cast<void*>(&HookedSetCount), reinterpret_cast<void**>(&oSetCount));
}
