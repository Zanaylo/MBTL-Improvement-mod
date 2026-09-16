#include "Performance/InputLagMeter.h"

#include "Core/logger.h"
#include "Core/utils.h"
#include "Game/Anchors.h"
#include "Game/GameOffsets.h"
#include "Hooks/ImageScanner.h"
#include "Training/Players.h"

#include <windows.h>

#include <algorithm>
#include <cstring>

#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif

namespace {

namespace Input = GameOffsets::Input;

constexpr int kHistory = 50;
constexpr double kTimeoutMs = 400.0;
constexpr float kTrustGapMs = 5.0f;
constexpr DWORD kIdleAfterMs = 500;
constexpr DWORD kIdleSleepMs = 50;
constexpr DWORD kTimerWaitMs = 20;
constexpr LONGLONG kPeriod100ns = 10000;
constexpr int kFirstKey = 8;
constexpr int kKeys = 256;
constexpr int kXInputSlots = 4;
constexpr int kTriggerThreshold = 64;
constexpr int kThumbDeadzone = 12000;
constexpr DWORD kDeadSlotRetryMs = 1000;

struct Sample
{
	float lagMs;
	bool trusted;
};

struct XPad
{
	WORD buttons;
	BYTE leftTrigger;
	BYTE rightTrigger;
	SHORT thumbs[4];
};

struct XState
{
	DWORD packet;
	XPad pad;
};

using XInputGetState_t = DWORD(WINAPI*)(DWORD, XState*);

uintptr_t g_inputOffset = 0;

CRITICAL_SECTION g_lock;
bool g_lockReady = false;
Sample g_history[kHistory] = {};
int g_count = 0;
int g_next = 0;
float g_lastMs = -1.0f;

HANDLE g_thread = nullptr;
volatile LONG g_team = 0;
volatile LONG g_aliveTick = 0;

bool g_keyDown[kKeys] = {};
bool g_pending = false;
double g_pressMs = 0.0;
double g_worstGapMs = 0.0;
double g_lastTickMs = 0.0;
bool g_hasPrevious = false;
uint32_t g_previousInput = 0;

XInputGetState_t g_xInputGetState = nullptr;
bool g_slotLive[kXInputSlots] = {};
DWORD g_slotRetryTick[kXInputSlots] = {};
DWORD g_slotPacket[kXInputSlots] = {};
XPad g_slotPrevious[kXInputSlots] = {};
bool g_slotHasPrevious[kXInputSlots] = {};

uintptr_t ResolveInputOffset()
{
	const uint8_t* const native = ImageScanner::NativeFunction(Input::kStickNative);

	if (!native)
	{
		LOG("InputLagMeter: the script native %s was not found", Input::kStickNative);
		return 0;
	}

	const size_t length = ImageScanner::FunctionLength(native);
	std::vector<uintptr_t> offsets;

	for (size_t i = 0; i + Input::kLoadLeverLength <= length; ++i)
	{
		if (std::memcmp(native + i, Input::kLoadLeverByte, sizeof(Input::kLoadLeverByte)) != 0)
			continue;
		if ((native[i + sizeof(Input::kLoadLeverByte)] & Input::kLoadLeverModRmMask) != Input::kLoadLeverModRm)
			continue;

		const uintptr_t lever = ImageScanner::ReadDword(native + i + Input::kLoadLeverOffsetAt);

		if (lever >= Input::kLeverShift / 8 && std::find(offsets.begin(), offsets.end(), lever) == offsets.end())
			offsets.push_back(lever);
	}

	if (offsets.size() == 1)
		return offsets.front() - Input::kLeverShift / 8;

	LOG("InputLagMeter: %s reads %u lever field(s), expected exactly one", Input::kStickNative,
		static_cast<unsigned>(offsets.size()));
	return 0;
}

double NowMs()
{
	static LARGE_INTEGER frequency = {};

	if (frequency.QuadPart == 0)
		QueryPerformanceFrequency(&frequency);

	LARGE_INTEGER now = {};
	QueryPerformanceCounter(&now);
	return static_cast<double>(now.QuadPart) * 1000.0 / static_cast<double>(frequency.QuadPart);
}

bool ForegroundIsOurs()
{
	const HWND foreground = GetForegroundWindow();
	DWORD process = 0;

	return foreground != nullptr && GetWindowThreadProcessId(foreground, &process) != 0 &&
		process == GetCurrentProcessId();
}

void Arm(double now)
{
	g_pending = true;
	g_pressMs = now;
	g_worstGapMs = 0.0;
}

void Record(double lagMs, double worstGapMs)
{
	EnterCriticalSection(&g_lock);

	Sample& sample = g_history[g_next];
	sample.lagMs = static_cast<float>(lagMs);
	sample.trusted = static_cast<float>(worstGapMs) <= kTrustGapMs;

	g_next = (g_next + 1) % kHistory;
	g_count = g_count < kHistory ? g_count + 1 : g_count;
	g_lastMs = sample.lagMs;

	LeaveCriticalSection(&g_lock);
}

bool CrossedInto(int value, int previous, int threshold)
{
	return value >= threshold && previous < threshold;
}

int Magnitude(SHORT value)
{
	return value < 0 ? -static_cast<int>(value) : value;
}

bool PadPressed(const XPad& now, const XPad& before)
{
	if ((now.buttons & ~before.buttons) != 0)
		return true;

	if (CrossedInto(now.leftTrigger, before.leftTrigger, kTriggerThreshold) ||
		CrossedInto(now.rightTrigger, before.rightTrigger, kTriggerThreshold))
	{
		return true;
	}

	for (int axis = 0; axis < 4; ++axis)
	{
		if (CrossedInto(Magnitude(now.thumbs[axis]), Magnitude(before.thumbs[axis]), kThumbDeadzone))
			return true;
	}

	return false;
}

void ResolveXInput()
{
	if (g_xInputGetState != nullptr)
		return;

	HMODULE module = GetModuleHandleA(Input::kXInputLibrary);

	if (module == nullptr)
		return;

	g_xInputGetState = reinterpret_cast<XInputGetState_t>(GetProcAddress(module, MAKEINTRESOURCEA(Input::kXInputGetStateOrdinal)));
}

void SampleSlot(int slot, double now, DWORD tick)
{
	if (!g_slotLive[slot] && tick - g_slotRetryTick[slot] < kDeadSlotRetryMs)
		return;

	XState state = {};

	if (g_xInputGetState(static_cast<DWORD>(slot), &state) != ERROR_SUCCESS)
	{
		g_slotLive[slot] = false;
		g_slotRetryTick[slot] = tick;
		g_slotHasPrevious[slot] = false;
		return;
	}

	g_slotLive[slot] = true;

	if (g_slotHasPrevious[slot] && state.packet == g_slotPacket[slot])
		return;

	if (g_slotHasPrevious[slot] && PadPressed(state.pad, g_slotPrevious[slot]))
		Arm(now);

	g_slotPacket[slot] = state.packet;
	g_slotPrevious[slot] = state.pad;
	g_slotHasPrevious[slot] = true;
}

void SamplePads(double now)
{
	ResolveXInput();

	if (g_xInputGetState == nullptr)
		return;

	const DWORD tick = GetTickCount();

	for (int slot = 0; slot < kXInputSlots; ++slot)
		SampleSlot(slot, now, tick);
}

void SampleKeyboard(double now)
{
	for (int key = kFirstKey; key < kKeys; ++key)
	{
		const bool down = (GetAsyncKeyState(key) & 0x8000) != 0;

		if (down && !g_keyDown[key])
			Arm(now);

		g_keyDown[key] = down;
	}
}

void Forget()
{
	g_pending = false;
	g_hasPrevious = false;

	for (bool& previous : g_slotHasPrevious)
		previous = false;
}

void WatchCharacter(double now)
{
	const uintptr_t chara = Players::Chara(static_cast<int>(InterlockedCompareExchange(&g_team, 0, 0)));
	uint32_t input = 0;

	if (chara == 0 || !TryRead(chara + g_inputOffset, input))
		return;

	const uint32_t meaningful = input & (0xFFu << Input::kLeverShift | Input::kButtonMask);
	const bool changed = g_hasPrevious && meaningful != g_previousInput;

	g_hasPrevious = true;
	g_previousInput = meaningful;

	if (!changed)
		return;

	if (g_pending && now - g_pressMs <= kTimeoutMs)
		Record(now - g_pressMs, g_worstGapMs);

	g_pending = false;
}

void Tick()
{
	const double now = NowMs();
	const double gap = g_lastTickMs > 0.0 ? now - g_lastTickMs : 0.0;
	g_lastTickMs = now;

	if (!ForegroundIsOurs())
	{
		Forget();
		return;
	}

	if (g_pending && gap > g_worstGapMs)
		g_worstGapMs = gap;

	WatchCharacter(now);
	SampleKeyboard(now);
	SamplePads(now);

	if (g_pending && now - g_pressMs > kTimeoutMs)
		g_pending = false;
}

void WaitOneMillisecond(HANDLE timer)
{
	LARGE_INTEGER due = {};
	due.QuadPart = -kPeriod100ns;

	if (timer != nullptr && SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE))
	{
		WaitForSingleObject(timer, kTimerWaitMs);
		return;
	}

	Sleep(1);
}

DWORD WINAPI SamplerThread(LPVOID)
{
	const HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr,
		CREATE_WAITABLE_TIMER_MANUAL_RESET | CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);

	for (;;)
	{
		const DWORD alive = static_cast<DWORD>(InterlockedCompareExchange(&g_aliveTick, 0, 0));

		if (GetTickCount() - alive > kIdleAfterMs)
		{
			Forget();
			g_lastTickMs = 0.0;
			Sleep(kIdleSleepMs);
			continue;
		}

		Tick();
		WaitOneMillisecond(timer);
	}
}

}

bool InputLagMeter::Initialize()
{
	g_inputOffset = ResolveInputOffset();
	Anchors::Record("Character input field", g_inputOffset, "input lag meter");

	if (!g_lockReady)
	{
		InitializeCriticalSection(&g_lock);
		g_lockReady = true;
	}

	return g_inputOffset != 0;
}

bool InputLagMeter::IsAvailable()
{
	return g_inputOffset != 0 && g_lockReady;
}

void InputLagMeter::KeepAlive(int team)
{
	if (!IsAvailable() || team < 0 || team >= Players::kTeams)
		return;

	InterlockedExchange(&g_team, team);
	InterlockedExchange(&g_aliveTick, static_cast<LONG>(GetTickCount()));

	if (g_thread != nullptr)
		return;

	g_thread = CreateThread(nullptr, 0, &SamplerThread, nullptr, 0, nullptr);

	if (g_thread == nullptr)
		LOG("InputLagMeter: could not start the sampler thread");
}

void InputLagMeter::Reset()
{
	if (!g_lockReady)
		return;

	EnterCriticalSection(&g_lock);
	g_count = 0;
	g_next = 0;
	g_lastMs = -1.0f;
	LeaveCriticalSection(&g_lock);
}

float InputLagMeter::GetLastMs()
{
	if (!g_lockReady)
		return -1.0f;

	EnterCriticalSection(&g_lock);
	const float last = g_lastMs;
	LeaveCriticalSection(&g_lock);
	return last;
}

float InputLagMeter::GetAverageMs()
{
	if (!g_lockReady)
		return -1.0f;

	float sum = 0.0f;
	int used = 0;

	EnterCriticalSection(&g_lock);

	for (int i = 0; i < g_count; ++i)
	{
		if (!g_history[i].trusted)
			continue;

		sum += g_history[i].lagMs;
		++used;
	}

	LeaveCriticalSection(&g_lock);
	return used > 0 ? sum / static_cast<float>(used) : -1.0f;
}

int InputLagMeter::GetTrustedCount()
{
	if (!g_lockReady)
		return 0;

	int used = 0;

	EnterCriticalSection(&g_lock);

	for (int i = 0; i < g_count; ++i)
		used += g_history[i].trusted ? 1 : 0;

	LeaveCriticalSection(&g_lock);
	return used;
}
