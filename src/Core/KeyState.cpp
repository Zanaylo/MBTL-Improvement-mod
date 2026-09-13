#include "Core/KeyState.h"

#include <windows.h>

namespace {

constexpr int kKeyCount = 256;
constexpr int kFirstKey = 1;

bool g_current[kKeyCount] = {};
bool g_previous[kKeyCount] = {};
DWORD g_downSince[kKeyCount] = {};
DWORD g_lastRepeat[kKeyCount] = {};

bool Valid(int virtualKey)
{
	return virtualKey >= kFirstKey && virtualKey < kKeyCount;
}

}

void KeyState::Poll()
{
	const DWORD now = GetTickCount();

	for (int key = kFirstKey; key < kKeyCount; ++key)
	{
		g_previous[key] = g_current[key];
		g_current[key] = (GetAsyncKeyState(key) & 0x8000) != 0;

		if (!g_current[key] || g_previous[key])
			continue;

		g_downSince[key] = now;
		g_lastRepeat[key] = now;
	}
}

void KeyState::Clear()
{
	for (int key = 0; key < kKeyCount; ++key)
	{
		g_current[key] = false;
		g_previous[key] = false;
	}
}

bool KeyState::Pressed(int virtualKey)
{
	return Valid(virtualKey) && g_current[virtualKey] && !g_previous[virtualKey];
}

bool KeyState::Held(int virtualKey)
{
	return Valid(virtualKey) && g_current[virtualKey];
}

bool KeyState::Repeating(int virtualKey, unsigned delayMs, unsigned intervalMs)
{
	if (!Held(virtualKey))
		return false;

	if (!g_previous[virtualKey])
		return true;

	const DWORD now = GetTickCount();

	if (now - g_downSince[virtualKey] < delayMs)
		return false;

	if (now - g_lastRepeat[virtualKey] < intervalMs)
		return false;

	g_lastRepeat[virtualKey] = now;
	return true;
}
