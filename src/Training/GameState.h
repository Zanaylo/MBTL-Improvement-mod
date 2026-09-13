#pragma once

#include <cstdint>

namespace GameState
{
	void NoteBattleStep();

	bool IsBattleRunning();
	bool IsOnlineKnown();
	bool IsOnline();
	bool IsGamePaused();
	bool IsTraining();

	int Mode();
	int SubMode();
	uint32_t FrameCounter();

	bool AllowsTrainingTools();
	const char* StatusText();
}
