#include "Performance/PumpWait.h"

#include "Core/Compat.h"
#include "Core/interfaces.h"
#include "Core/logger.h"
#include "D3D9/DeviceHooks.h"
#include "Hooks/HookManager.h"

#include <windows.h>

#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif

namespace {

constexpr DWORD kMaxSubstitutedMs = 2;
constexpr DWORD kResolveRetryMs = 500;
constexpr DWORD kTimerSlackMs = 20;

using Sleep_t = void(WINAPI*)(DWORD);

Sleep_t oSleep = nullptr;

bool g_installed = false;
volatile LONG g_enabled = 0;
volatile LONG g_windowThread = 0;
DWORD g_lastResolve = 0;

HANDLE ThreadTimer()
{
	static thread_local HANDLE timer = nullptr;

	if (timer != nullptr)
		return timer;

	timer = CreateWaitableTimerExW(nullptr, nullptr,
		CREATE_WAITABLE_TIMER_MANUAL_RESET | CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
	return timer;
}

DWORD WindowThread()
{
	const DWORD known = static_cast<DWORD>(InterlockedCompareExchange(&g_windowThread, 0, 0));

	if (known != 0)
		return known;

	const DWORD now = GetTickCount();

	if (g_lastResolve != 0 && now - g_lastResolve < kResolveRetryMs)
		return 0;

	g_lastResolve = now;

	const HWND window = DeviceHooks::Window();
	DWORD process = 0;
	const DWORD thread = window != nullptr ? GetWindowThreadProcessId(window, &process) : 0;

	if (thread == 0 || process != GetCurrentProcessId())
		return 0;

	InterlockedExchange(&g_windowThread, static_cast<LONG>(thread));
	LOG("[PumpWait] the game window belongs to thread %lu", static_cast<unsigned long>(thread));
	return thread;
}

bool WaitPrecisely(DWORD milliseconds, bool wakeOnInput)
{
	HANDLE timer = ThreadTimer();

	if (timer == nullptr)
		return false;

	LARGE_INTEGER due = {};
	due.QuadPart = -(static_cast<LONGLONG>(milliseconds) * 10000);

	if (!SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE))
		return false;

	if (!wakeOnInput)
		return WaitForSingleObject(timer, milliseconds + kTimerSlackMs) != WAIT_FAILED;

	return MsgWaitForMultipleObjectsEx(1, &timer, milliseconds + kTimerSlackMs, QS_ALLINPUT, MWMO_INPUTAVAILABLE) != WAIT_FAILED;
}

void WINAPI HookedSleep(DWORD milliseconds)
{
	if (milliseconds == 0 || milliseconds > kMaxSubstitutedMs || InterlockedCompareExchange(&g_enabled, 0, 0) == 0)
	{
		oSleep(milliseconds);
		return;
	}

	const bool wakeOnInput = g_settings.pumpWaitAllInput && GetCurrentThreadId() == WindowThread();

	if (!WaitPrecisely(milliseconds, wakeOnInput))
		oSleep(milliseconds);
}

}

bool PumpWait::Install()
{
	if (g_installed)
		return true;

	if (!HookManager::CreateApiHook(L"kernel32.dll", "Sleep", &HookedSleep, reinterpret_cast<void**>(&oSleep)))
	{
		LOG("[PumpWait] could not hook Sleep");
		return false;
	}

	g_installed = true;
	return true;
}

void PumpWait::Apply()
{
	const bool wanted = g_installed && g_settings.pumpWait && !Compat::SafeMode();
	InterlockedExchange(&g_enabled, wanted ? 1 : 0);
}

bool PumpWait::IsActive()
{
	return InterlockedCompareExchange(&g_enabled, 0, 0) != 0;
}
