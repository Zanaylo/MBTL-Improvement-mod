#include "Training/FrameStepper.h"

#include "Core/Profiler.h"
#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Game/GameOffsets.h"
#include "Hooks/HookManager.h"
#include "Training/BattleMap.h"
#include "Training/GameState.h"

#include <windows.h>

#include <cstring>
#include <vector>

namespace {

using BattleUpdate_t = int(__fastcall*)(uint8_t*);

BattleUpdate_t oBattleUpdate = nullptr;

bool g_hooked = false;
bool g_paused = false;
volatile LONG g_pendingSteps = 0;
volatile LONG g_stepped = 0;
volatile LONG64 g_calls = 0;
volatile LONG64 g_suppressed = 0;
char g_status[160] = "not set up yet";
std::vector<ITickListener*> g_tickListeners;

bool TakeStep()
{
	for (;;)
	{
		const LONG pending = InterlockedCompareExchange(&g_pendingSteps, 0, 0);

		if (pending <= 0)
			return false;

		if (InterlockedCompareExchange(&g_pendingSteps, pending - 1, pending) == pending)
			return true;
	}
}

int Simulate(uint8_t* flags)
{
	int result = 0;

	{
		Profiler::Scope scope(Profiler::Section_TickGame);
		result = oBattleUpdate(flags);
	}

	for (ITickListener* listener : g_tickListeners)
		listener->OnBattleTick();

	Profiler::EndTickFrame();
	return result;
}

int Freeze(uint8_t* flags)
{
	namespace Battle = GameOffsets::Battle;

	const uint8_t sim = flags[Battle::kSimFlag];
	const uint8_t first = flags[Battle::kFirstFlag];

	flags[Battle::kSimFlag] = 0;

	if (g_settings.replayFrozenFrame)
		flags[Battle::kFirstFlag] = 0;

	InterlockedIncrement64(&g_suppressed);
	const int result = oBattleUpdate(flags);

	flags[Battle::kSimFlag] = sim;
	flags[Battle::kFirstFlag] = first;
	return result;
}

int __fastcall HookedBattleUpdate(uint8_t* flags)
{
	InterlockedIncrement64(&g_calls);
	GameState::NoteBattleStep();

	if (flags == nullptr || flags[GameOffsets::Battle::kSimFlag] == 0)
		return oBattleUpdate(flags);

	if (!g_paused)
		return Simulate(flags);

	if (!GameState::AllowsTrainingTools())
	{
		FrameStepper::SetPaused(false);
		return Simulate(flags);
	}

	if (GameState::IsGamePaused())
		return Simulate(flags);

	if (!TakeStep())
		return Freeze(flags);

	InterlockedExchange(&g_stepped, 1);
	return Simulate(flags);
}

}

bool FrameStepper::Initialize()
{
	const uint8_t* const target = BattleMap::Addresses().battleUpdate;

	if (!target)
	{
		strncpy_s(g_status, "does not work in this game version", _TRUNCATE);
		LOG("FrameStepper: %s", g_status);
		return false;
	}

	g_hooked = HookManager::CreateHook(const_cast<uint8_t*>(target), reinterpret_cast<void*>(&HookedBattleUpdate),
		reinterpret_cast<void**>(&oBattleUpdate), "BattleUpdate");

	strncpy_s(g_status, g_hooked ? "ready" : "could not start", _TRUNCATE);
	LOG("FrameStepper: %s", g_status);
	return g_hooked;
}

void FrameStepper::AddTickListener(ITickListener* listener)
{
	if (listener == nullptr)
		return;

	g_tickListeners.push_back(listener);
}

bool FrameStepper::IsImplemented()
{
	return g_hooked;
}

const char* FrameStepper::StatusText()
{
	return g_status;
}

bool FrameStepper::IsPaused()
{
	return g_paused;
}

bool FrameStepper::IsFrozen()
{
	return g_hooked && g_paused && GameState::AllowsTrainingTools() && !GameState::IsGamePaused();
}

void FrameStepper::SetPaused(bool paused)
{
	if (paused && !GameState::AllowsTrainingTools())
		return;

	g_paused = paused;
	InterlockedExchange(&g_pendingSteps, 0);
}

void FrameStepper::TogglePaused()
{
	SetPaused(!g_paused);
}

void FrameStepper::RequestStep(int frames)
{
	if (!g_paused || frames <= 0)
		return;

	InterlockedExchangeAdd(&g_pendingSteps, frames);
}

bool FrameStepper::ConsumeSteppedFlag()
{
	return InterlockedExchange(&g_stepped, 0) != 0;
}

bool FrameStepper::NeedsFrozenFrameReplay()
{
	return g_settings.replayFrozenFrame && IsFrozen();
}

uint64_t FrameStepper::CallCount()
{
	return static_cast<uint64_t>(InterlockedCompareExchange64(&g_calls, 0, 0));
}

uint64_t FrameStepper::SuppressedCount()
{
	return static_cast<uint64_t>(InterlockedCompareExchange64(&g_suppressed, 0, 0));
}
