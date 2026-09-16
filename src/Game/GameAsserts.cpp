#include "Game/GameAsserts.h"

#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Game/Anchors.h"
#include "Game/AssertSites.h"
#include "Hooks/CodePatch.h"

#include <windows.h>
#include <intrin.h>

#include <cstdint>

namespace {

constexpr size_t kRememberedCallers = 256;
constexpr size_t kPlaceBytes = 128;
constexpr size_t kExpressionBytes = 256;

void* g_callers[kRememberedCallers] = {};
volatile LONG g_skipped = 0;

bool FirstFrom(void* caller)
{
	for (void*& slot : g_callers)
	{
		const void* const previous = InterlockedCompareExchangePointer(&slot, caller, nullptr);

		if (!previous)
			return true;
		if (previous == caller)
			return false;
	}

	return false;
}

void __cdecl HookedAssert(const wchar_t* message, const wchar_t* file, unsigned line)
{
	const LONG skipped = InterlockedIncrement(&g_skipped);

	if (!FirstFrom(_ReturnAddress()))
		return;

	char place[kPlaceBytes] = {};
	char expression[kExpressionBytes] = {};
	AssertSites::Describe(file, line, place, sizeof(place));
	AssertSites::Narrow(message, expression, sizeof(expression));

	LOG("GameAsserts: %s \"%s\" failed and was skipped (%ld so far)", place, expression, skipped);
}

}

void GameAsserts::Install()
{
	if (g_settings.showGameAsserts)
	{
		LOG("GameAsserts: the game shows its own assert dialogs ([Compat] ShowGameAsserts = 1)");
		return;
	}

	uint8_t* const slot = AssertSites::ImportSlot();

	if (!slot)
	{
		LOG("GameAsserts: the _wassert import was not found, the game keeps its dialogs");
		return;
	}

	const auto detour = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&HookedAssert));

	if (!CodePatch::Write(slot, &detour, sizeof(detour)))
	{
		LOG("GameAsserts: the _wassert import could not be patched");
		return;
	}

	Anchors::Record("_wassert import", reinterpret_cast<uintptr_t>(slot), "game asserts are logged and skipped");
	LOG("GameAsserts: game asserts are logged and skipped");
}
