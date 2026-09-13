#pragma once

namespace GameRestart
{
	void Install();

	bool CanSoftReset();
	bool SoftReset();

	void OnFrame();

	bool IsPending();
	const char* StatusText();
}
