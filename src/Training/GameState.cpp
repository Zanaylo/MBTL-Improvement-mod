#include "Training/GameState.h"

#include "Core/utils.h"
#include "Game/GameOffsets.h"
#include "Training/BattleMap.h"

#include <windows.h>

namespace {

namespace Battle = GameOffsets::Battle;

constexpr DWORD kRunningMs = 250;
constexpr int kUnknown = -1;

volatile LONG g_lastStep = 0;

bool ReadField(uintptr_t object, uintptr_t offset, uint32_t& out)
{
	return object != 0 && TryRead(object + offset, out);
}

int ReadInfo(uintptr_t offset)
{
	uint32_t value = 0;
	return ReadField(BattleMap::Addresses().battleInfo, offset, value) ? static_cast<int>(value) : kUnknown;
}

}

void GameState::NoteBattleStep()
{
	const DWORD now = GetTickCount();
	InterlockedExchange(&g_lastStep, static_cast<LONG>(now == 0 ? 1 : now));
}

bool GameState::IsBattleRunning()
{
	const DWORD last = static_cast<DWORD>(InterlockedCompareExchange(&g_lastStep, 0, 0));
	return last != 0 && GetTickCount() - last < kRunningMs;
}

bool GameState::IsOnlineKnown()
{
	return BattleMap::Addresses().session != 0;
}

bool GameState::IsOnline()
{
	uint32_t session = 0;
	return ReadField(BattleMap::Addresses().session, 0, session) && session != 0;
}

bool GameState::IsGamePaused()
{
	uint32_t state = 0;
	return ReadField(BattleMap::Addresses().pause, Battle::kPauseState, state) && state != 0;
}

bool GameState::IsTraining()
{
	const int mode = Mode();
	return mode == Battle::kModeTraining || (mode == Battle::kModeSingle && SubMode() == Battle::kSubModeTraining);
}

int GameState::Mode()
{
	return ReadInfo(Battle::kMode);
}

int GameState::SubMode()
{
	return ReadInfo(Battle::kSubMode);
}

uint32_t GameState::FrameCounter()
{
	uint32_t counter = 0;
	ReadField(BattleMap::Addresses().battleInfo, Battle::kFrameCounter, counter);
	return counter;
}

bool GameState::AllowsTrainingTools()
{
	return IsBattleRunning() && IsOnlineKnown() && !IsOnline();
}

const char* GameState::StatusText()
{
	if (!IsOnlineKnown())
		return "The online session was not found in this build, so training tools stay off.";

	if (IsOnline())
		return "Not while you are online.";

	if (!IsBattleRunning())
		return "In a match only - training, versus, arcade or a replay.";

	return IsTraining() ? "In training." : "In a match.";
}
