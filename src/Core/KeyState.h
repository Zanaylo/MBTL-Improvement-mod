#pragma once

namespace KeyState
{
	void Poll();
	void Clear();

	bool Pressed(int virtualKey);
	bool Held(int virtualKey);
	bool Repeating(int virtualKey, unsigned delayMs, unsigned intervalMs);
}
