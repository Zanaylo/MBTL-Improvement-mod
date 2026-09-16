#include "Music/BgmControl.h"

#include "Core/logger.h"
#include "Hooks/HookManager.h"
#include "Music/BgmNames.h"
#include "Music/BgmRules.h"
#include "Music/BgmShuffle.h"
#include "Music/BgmTable.h"
#include "Music/BgmVolume.h"

#include <windows.h>

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace {

using PlayBgm_t = int(__fastcall*)(int);
using BgmCommand_t = void(__cdecl*)();

enum Playback
{
	Playback_None,
	Playback_Play,
	Playback_Stop,
	Playback_Release,
};

struct Pending
{
	Playback playback;
	int id;
	bool volume;
};

constexpr long kUnknownThread = 0;
constexpr int kNoTrack = BgmTable::kNoTrack;
constexpr const char* kPlaybackNames[] = { "do nothing", "play", "stop", "let the game choose" };

constexpr const char* kNotInstalled = "not started";
constexpr const char* kNoPlay = "the music player was not found on this game version";
constexpr const char* kHookFailed = "the mod could not attach to the music player";
constexpr const char* kWaiting = "ready. Play and Stop work once the game has played some music";
constexpr const char* kOnPresent = "ready. Play and Stop work right away";
constexpr const char* kOnSceneChange = "ready. Play and Stop take effect on the next screen change";
constexpr const char* kNoCommands = "ready, but Play and Stop take effect on the next screen change";

PlayBgm_t oPlayBgm = nullptr;
BgmCommand_t g_stop = nullptr;
BgmCommand_t g_start = nullptr;

bool g_hooked = false;
std::atomic<const char*> g_status{ kNotInstalled };

volatile long g_held = kNoTrack;
volatile long g_lastAsked = kNoTrack;
volatile long g_lastPlayed = kNoTrack;
volatile long g_calls = 0;

volatile long g_gameThread = kUnknownThread;
volatile long g_presentThread = kUnknownThread;
volatile long g_threadsDiffer = 0;

volatile long g_queued = 0;
SRWLOCK g_pendingLock = SRWLOCK_INIT;
Pending g_pending = { Playback_None, kNoTrack, false };

SRWLOCK g_reasonLock = SRWLOCK_INIT;
char g_reason[192] = "no music asked for yet";

void UpdateStatus()
{
	if (g_stop == nullptr || g_start == nullptr)
	{
		g_status = kNoCommands;
		return;
	}

	if (g_threadsDiffer != 0)
	{
		g_status = kOnSceneChange;
		return;
	}

	if (g_gameThread == kUnknownThread || g_presentThread == kUnknownThread)
	{
		g_status = kWaiting;
		return;
	}

	g_status = kOnPresent;
}

void CompareThreads()
{
	const long game = g_gameThread;
	const long present = g_presentThread;

	if (game != kUnknownThread && present != kUnknownThread && game != present)
		InterlockedExchange(&g_threadsDiffer, 1);

	UpdateStatus();
}

void NoteGameThread()
{
	const long thread = static_cast<long>(GetCurrentThreadId());
	const long seen = InterlockedCompareExchange(&g_gameThread, thread, kUnknownThread);

	if (seen == kUnknownThread)
	{
		LOG("BgmControl: PlayBgm runs on thread %lu", static_cast<unsigned long>(thread));
		CompareThreads();
		return;
	}

	if (seen == thread || g_threadsDiffer != 0)
		return;

	InterlockedExchange(&g_threadsDiffer, 1);
	LOG("BgmControl: PlayBgm also ran on thread %lu, so Play and Stop from the window wait for the next scene change",
		static_cast<unsigned long>(thread));
	UpdateStatus();
}

void NotePresentThread()
{
	const long thread = static_cast<long>(GetCurrentThreadId());

	if (InterlockedCompareExchange(&g_presentThread, thread, kUnknownThread) != kUnknownThread)
		return;

	LOG("BgmControl: Present runs on thread %lu", static_cast<unsigned long>(thread));
	CompareThreads();
}

void Explain(const char* format, ...)
{
	char text[sizeof(g_reason)] = {};

	va_list arguments;
	va_start(arguments, format);
	vsnprintf(text, sizeof(text), format, arguments);
	va_end(arguments);

	AcquireSRWLockExclusive(&g_reasonLock);
	strncpy_s(g_reason, text, _TRUNCATE);
	ReleaseSRWLockExclusive(&g_reasonLock);
}

void Queue(Playback playback, int id)
{
	AcquireSRWLockExclusive(&g_pendingLock);
	g_pending.playback = playback;
	g_pending.id = id;
	ReleaseSRWLockExclusive(&g_pendingLock);

	InterlockedExchange(&g_queued, 1);
}

Pending TakePending()
{
	AcquireSRWLockExclusive(&g_pendingLock);
	const Pending taken = g_pending;
	g_pending = { Playback_None, kNoTrack, false };
	ReleaseSRWLockExclusive(&g_pendingLock);

	return taken;
}

int Choose(int asked)
{
	char askedName[64] = {};
	BgmNames::Describe(asked, askedName, sizeof(askedName));

	const int held = static_cast<int>(g_held);

	if (held != kNoTrack && BgmTable::IsPresent(held))
	{
		Explain("your pick, played instead of %s", askedName);
		return held;
	}

	const int ruled = BgmRules::Resolve(asked);

	if (ruled != kNoTrack)
	{
		Explain("your rule, played instead of %s", askedName);
		return ruled;
	}

	const int drawn = BgmShuffle::Pick(asked);

	if (drawn != kNoTrack)
	{
		Explain("the randomizer, played instead of %s", askedName);
		return drawn;
	}

	Explain("the game's choice: %s", askedName);
	return asked;
}

void Restart(int id)
{
	g_stop();
	BgmVolume::ApplyToSlot(id);
	oPlayBgm(id);
	InterlockedExchange(&g_lastPlayed, id);
	g_start();
}

void PlayHeld(int id)
{
	if (!BgmTable::IsPresent(id))
	{
		LOG("BgmControl: %d has no track, so it is not played", id);
		return;
	}

	InterlockedExchange(&g_held, id);
	Explain("your pick, kept until you press Stop or Let the game choose");
	Restart(id);
}

void StopNow()
{
	InterlockedExchange(&g_held, kNoTrack);
	g_stop();
	Explain("you pressed Stop. Music comes back on the next screen");
}

void GiveBack()
{
	InterlockedExchange(&g_held, kNoTrack);

	const int asked = static_cast<int>(g_lastAsked);

	if (!BgmTable::IsValidId(asked))
	{
		Explain("the game picks the music on the next screen");
		return;
	}

	Restart(Choose(asked));
}

void Execute(const Pending& pending)
{
	switch (pending.playback)
	{
	case Playback_Play:
		PlayHeld(pending.id);
		break;
	case Playback_Stop:
		StopNow();
		break;
	case Playback_Release:
		GiveBack();
		break;
	default:
		break;
	}

	if (pending.volume)
		BgmVolume::ApplyNow();
}

void Adopt(const Pending& pending)
{
	if (pending.playback == Playback_Play)
	{
		InterlockedExchange(&g_held, pending.id);
		return;
	}

	if (pending.playback == Playback_None)
		return;

	InterlockedExchange(&g_held, kNoTrack);
}

void AdoptQueued()
{
	if (BgmControl::RunsOnPresent() || InterlockedExchange(&g_queued, 0) == 0)
		return;

	Adopt(TakePending());
}

int __fastcall HookedPlayBgm(int id)
{
	InterlockedIncrement(&g_calls);
	NoteGameThread();
	AdoptQueued();
	InterlockedExchange(&g_lastAsked, id);

	if (!BgmTable::IsValidId(id))
		return oPlayBgm(id);

	const int chosen = Choose(id);

	BgmVolume::ApplyToSlot(chosen);
	InterlockedExchange(&g_lastPlayed, chosen);

	LOG("BgmControl: the game asked for %d, playing %d", id, chosen);
	return oPlayBgm(chosen);
}

}

bool BgmControl::Install(uint8_t* play, uint8_t* stop, uint8_t* start)
{
	if (play == nullptr)
	{
		g_status = kNoPlay;
		LOG("BgmControl: %s", kNoPlay);
		return false;
	}

	if (!HookManager::CreateHook(play, reinterpret_cast<void*>(&HookedPlayBgm), reinterpret_cast<void**>(&oPlayBgm),
		"PlayBgm"))
	{
		g_status = kHookFailed;
		LOG("BgmControl: %s", kHookFailed);
		return false;
	}

	g_stop = reinterpret_cast<BgmCommand_t>(stop);
	g_start = reinterpret_cast<BgmCommand_t>(start);
	g_hooked = true;

	UpdateStatus();
	LOG("BgmControl: %s", StatusText());
	return true;
}

bool BgmControl::IsHooked()
{
	return g_hooked;
}

const char* BgmControl::StatusText()
{
	return g_status;
}

bool BgmControl::RunsOnPresent()
{
	return g_status == kOnPresent;
}

void BgmControl::Play(int id)
{
	if (!g_hooked || !BgmTable::IsPresent(id))
		return;

	Queue(Playback_Play, id);
}

void BgmControl::Stop()
{
	if (!g_hooked)
		return;

	Queue(Playback_Stop, kNoTrack);
}

void BgmControl::Release()
{
	if (!g_hooked)
		return;

	Queue(Playback_Release, kNoTrack);
}

void BgmControl::Redraw()
{
	BgmShuffle::Redraw();

	if (g_held != kNoTrack)
		return;

	Release();
}

void BgmControl::RefreshVolume()
{
	if (!g_hooked)
		return;

	AcquireSRWLockExclusive(&g_pendingLock);
	g_pending.volume = true;
	ReleaseSRWLockExclusive(&g_pendingLock);

	InterlockedExchange(&g_queued, 1);
}

void BgmControl::OnPresent()
{
	if (!g_hooked)
		return;

	if (g_presentThread == kUnknownThread)
		NotePresentThread();

	if (!RunsOnPresent() || InterlockedExchange(&g_queued, 0) == 0)
		return;

	Execute(TakePending());
}

int BgmControl::HeldId()
{
	return static_cast<int>(g_held);
}

int BgmControl::LastAsked()
{
	return static_cast<int>(g_lastAsked);
}

int BgmControl::LastPlayed()
{
	return static_cast<int>(g_lastPlayed);
}

long BgmControl::CallCount()
{
	return static_cast<long>(g_calls);
}

void BgmControl::ReasonText(char* out, size_t size)
{
	AcquireSRWLockShared(&g_reasonLock);
	strncpy_s(out, size, g_reason, _TRUNCATE);
	ReleaseSRWLockShared(&g_reasonLock);
}

bool BgmControl::HasPending()
{
	return g_queued != 0;
}

void BgmControl::PendingText(char* out, size_t size)
{
	AcquireSRWLockShared(&g_pendingLock);
	const Pending pending = g_pending;
	ReleaseSRWLockShared(&g_pendingLock);

	const char* const volume = pending.volume ? ", then a volume change" : "";

	if (pending.playback == Playback_Play)
	{
		sprintf_s(out, size, "Waiting to play %03d%s", pending.id, volume);
		return;
	}

	sprintf_s(out, size, "Waiting to %s%s", kPlaybackNames[pending.playback], volume);
}

unsigned long BgmControl::GameThread()
{
	return static_cast<unsigned long>(g_gameThread);
}

unsigned long BgmControl::PresentThread()
{
	return static_cast<unsigned long>(g_presentThread);
}
