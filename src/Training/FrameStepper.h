#pragma once

#include "Training/TickListener.h"

#include <cstdint>

namespace FrameStepper
{
	bool Initialize();
	void AddTickListener(ITickListener* listener);
	bool IsImplemented();
	const char* StatusText();

	bool IsPaused();
	bool IsFrozen();
	void SetPaused(bool paused);
	void TogglePaused();
	void RequestStep(int frames);

	bool ConsumeSteppedFlag();
	bool NeedsFrozenFrameReplay();

	uint64_t CallCount();
	uint64_t SuppressedCount();
}
