#pragma once

class ITickListener;

namespace StageTint
{
	void Install();

	ITickListener* Listener();
	bool IsAvailable();
	const char* StatusText();
}
