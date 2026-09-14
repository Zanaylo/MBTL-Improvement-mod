#include "Game/GameRestart.h"

#include "Core/logger.h"
#include "D3D9/DeviceHooks.h"
#include "Game/GameOffsets.h"
#include "Game/SceneMap.h"
#include "Game/SceneWatch.h"
#include "Hooks/HookManager.h"
#include "Training/GameState.h"

#include <windows.h>

#include <cstdio>
#include <cstring>

namespace {

constexpr int kTitleFrames = 900;
constexpr LONG kNoScene = -1;
constexpr uint32_t kEntering = 1;

using SceneStep_t = int(__cdecl*)(int);
using SceneRequest_t = void(__fastcall*)(void*, void*, int);

SceneStep_t oSceneStep = nullptr;

bool g_hooked = false;
volatile LONG g_titleQueued = 0;
volatile LONG g_enterQueued = kNoScene;
volatile LONG g_steps = 0;
char g_status[256] = "";
bool g_pending = false;
int g_waited = 0;

void RunTitle(const SceneAddresses& scenes)
{
	if (InterlockedExchange(&g_titleQueued, 0) == 0)
		return;

	reinterpret_cast<SceneRequest_t>(scenes.request)(reinterpret_cast<void*>(scenes.manager), nullptr, scenes.titleFlag);
	LOG("GameRestart: the game was sent back to the title");
}

void ResetReplayCountdown(const SceneAddresses& scenes)
{
	uint32_t* const countdown = reinterpret_cast<uint32_t*>(scenes.replayChecker + scenes.replayCountdown);

	LOG("GameRestart: the replay check countdown stood at %u, set back to 0", *countdown);
	*countdown = 0;
}

void RunEnter(const SceneAddresses& scenes)
{
	const LONG scene = InterlockedExchange(&g_enterQueued, kNoScene);

	if (scene == kNoScene)
		return;

	ResetReplayCountdown(scenes);
	*reinterpret_cast<uint32_t*>(scenes.manager + GameOffsets::Scenes::kSceneId) = static_cast<uint32_t>(scene);
	*reinterpret_cast<uint32_t*>(scenes.entering) = kEntering;

	LOG("GameRestart: the game entered scene %ld", static_cast<long>(scene));
}

int __cdecl HookedSceneStep(int running)
{
	if (InterlockedIncrement(&g_steps) == 1)
		LOG("GameRestart: the scene step runs");

	const SceneAddresses& scenes = SceneMap::Addresses();

	RunTitle(scenes);
	RunEnter(scenes);
	return oSceneStep(running);
}

void EnterStart()
{
	const uint32_t scene = SceneWatch::First();

	InterlockedExchange(&g_enterQueued, static_cast<LONG>(scene));

	sprintf_s(g_status, "going to scene %u, the loading screen the game started on", scene);
	LOG("GameRestart: %s", g_status);
}

bool ReachedTitle()
{
	return !GameState::IsBattleRunning() &&
		SceneWatch::Current() == static_cast<uint32_t>(SceneMap::Addresses().titleScene);
}

bool Refuse(const char* reason)
{
	strncpy_s(g_status, reason, _TRUNCATE);
	return false;
}

class RestartListener final : public IDeviceListener
{
public:
	void OnPresent(IDirect3DDevice9*) override
	{
		SceneWatch::OnFrame();
		GameRestart::OnFrame();
	}
};

RestartListener g_listener;

}

void GameRestart::Install()
{
	const bool resolved = SceneMap::Initialize();
	const SceneAddresses& scenes = SceneMap::Addresses();

	if (scenes.manager != 0)
		DeviceHooks::AddListener(&g_listener);

	if (!resolved || scenes.manager == 0)
	{
		strncpy_s(g_status, "restart does not work on this game version", _TRUNCATE);
		LOG("GameRestart: %s", g_status);
		return;
	}

	g_hooked = HookManager::CreateHook(scenes.step, reinterpret_cast<void*>(&HookedSceneStep),
		reinterpret_cast<void**>(&oSceneStep), "scene step");

	if (!g_hooked)
		strncpy_s(g_status, "restart could not be set up", _TRUNCATE);

	LOG("GameRestart: %s", g_hooked ? "ready" : g_status);
}

bool GameRestart::CanSoftReset()
{
	if (!g_hooked || SceneWatch::First() == SceneWatch::kNone || g_pending)
		return false;

	return !GameState::IsBattleRunning() || GameState::IsTraining();
}

bool GameRestart::SoftReset()
{
	if (!g_hooked)
		return Refuse("restart does not work on this game version");

	if (SceneWatch::First() == SceneWatch::kNone)
		return Refuse("not ready: the mod did not see the game start");

	if (g_pending)
		return Refuse("already restarting");

	if (!GameState::IsOnlineKnown())
		return Refuse("restart is off: the online check does not work on this game version");

	if (GameState::IsOnline())
		return Refuse("not available during an online match");

	if (GameState::IsBattleRunning() && !GameState::IsTraining())
		return Refuse("leave the match first");

	InterlockedExchange(&g_titleQueued, 1);
	g_pending = true;
	g_waited = kTitleFrames;

	strncpy_s(g_status, "going to the title screen, then the loading screen", _TRUNCATE);
	LOG("GameRestart: %s", g_status);
	return true;
}

void GameRestart::OnFrame()
{
	if (!g_pending)
		return;

	if (!ReachedTitle())
	{
		if (--g_waited > 0)
			return;

		g_pending = false;

		strncpy_s(g_status, "the game did not reach the title screen, so the restart stopped", _TRUNCATE);
		LOG("GameRestart: %s", g_status);
		return;
	}

	g_pending = false;
	EnterStart();
}

bool GameRestart::IsPending()
{
	return g_pending;
}

const char* GameRestart::StatusText()
{
	return g_status;
}
