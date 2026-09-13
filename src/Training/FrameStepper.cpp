#include "Training/FrameStepper.h"

#include "Core/Profiler.h"
#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Hooks/HookManager.h"
#include "Training/BattleMap.h"
#include "Training/GameState.h"

#include <windows.h>

#include <cstring>
#include <vector>

namespace {

using BattleStep_t = int(__cdecl*)(int, int, int);

constexpr int kFlagMask = 0xFF;

BattleStep_t oBattleStep = nullptr;

bool g_hooked = false;
bool g_paused = false;
volatile LONG g_pendingSteps = 0;
volatile LONG g_stepped = 0;
volatile LONG64 g_calls = 0;
volatile LONG64 g_suppressed = 0;
char g_status[160] = "BattleStep has not been resolved";
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

int Simulate(int sim, int first, int second)
{
	int result = 0;

	{
		Profiler::Scope scope(Profiler::Section_TickGame);
		result = oBattleStep(sim, first, second);
	}

	for (ITickListener* listener : g_tickListeners)
		listener->OnBattleTick();

	Profiler::EndTickFrame();
	return result;
}

int Freeze(int first, int second)
{
	InterlockedIncrement64(&g_suppressed);
	return oBattleStep(0, g_settings.replayFrozenFrame ? 0 : first, second);
}

int __cdecl HookedBattleStep(int sim, int first, int second)
{
	InterlockedIncrement64(&g_calls);
	GameState::NoteBattleStep();

	if ((sim & kFlagMask) == 0)
		return oBattleStep(sim, first, second);

	if (!g_paused)
		return Simulate(sim, first, second);

	if (!GameState::AllowsTrainingTools())
	{
		FrameStepper::SetPaused(false);
		return Simulate(sim, first, second);
	}

	if (GameState::IsGamePaused())
		return Simulate(sim, first, second);

	if (!TakeStep())
		return Freeze(first, second);

	InterlockedExchange(&g_stepped, 1);
	return Simulate(sim, first, second);
}

}

bool FrameStepper::Initialize()
{
	const uint8_t* const target = BattleMap::Addresses().battleStep;

	if (!target)
	{
		strncpy_s(g_status, "BattleStep was not found in this build", _TRUNCATE);
		LOG("FrameStepper: %s", g_status);
		return false;
	}

	g_hooked = HookManager::CreateHook(const_cast<uint8_t*>(target), reinterpret_cast<void*>(&HookedBattleStep),
		reinterpret_cast<void**>(&oBattleStep), "BattleStep");

	strncpy_s(g_status, g_hooked ? "ready" : "the BattleStep hook could not be created", _TRUNCATE);
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
