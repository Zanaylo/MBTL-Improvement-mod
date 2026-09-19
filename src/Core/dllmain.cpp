#include "Core/Compat.h"
#include "Core/ProcessTuning.h"
#include "Core/Profiler.h"
#include "Core/Settings.h"
#include "Core/crashdump.h"
#include "Core/info.h"
#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Core/utils.h"
#include "D3D9/D3D9Hooks.h"
#include "D3D9/D3D9Proxy.h"
#include "D3D9/DeviceHooks.h"
#include "Game/CharacterIndexGuard.h"
#include "Game/GameAsserts.h"
#include "Game/GameRestart.h"
#include "Game/HiddenCharacters.h"
#include "Game/MemoryMap.h"
#include "Game/ModFiles.h"
#include "Game/SaveDataGuard.h"
#include "Hooks/HookManager.h"
#include "Hooks/ImageScanner.h"
#include "Music/MusicModule.h"
#include "Network/NetworkModule.h"
#include "Overlay/FrameMeterHud.h"
#include "Overlay/WindowManager.h"
#include "Palette/PaletteModule.h"
#include "Performance/PerformanceModule.h"
#include "Stages/StageTable.h"
#include "Stages/StagesModule.h"
#include "Training/TrainingModule.h"
#include "Web/UpdateCheck.h"
#include "Web/UpdateInstall.h"

#include <windows.h>

#include <string>

namespace {

constexpr DWORD kSlowStageMs = 50;

using DirectInput8Create_t = HRESULT(WINAPI*)(HINSTANCE, DWORD, const GUID&, void**, void*);
using StageFn = void (*)();

HANDLE g_instanceMutex = nullptr;
bool g_secondInstance = false;
bool g_mapped = false;

HMODULE g_originalDinput = nullptr;
DirectInput8Create_t g_originalDirectInput8Create = nullptr;
SRWLOCK g_dinputLock = SRWLOCK_INIT;
bool g_dinputLoadAttempted = false;

int LogStageFault(const char* name, DWORD code)
{
	LOG("Stage '%s' faulted (0x%08lX). Continuing without it.", name, static_cast<unsigned long>(code));
	return EXCEPTION_EXECUTE_HANDLER;
}

void RunStage(const char* name, StageFn stage)
{
	const DWORD started = GetTickCount();

	__try
	{
		stage();
	}
	__except (LogStageFault(name, GetExceptionCode()))
	{
	}

	const DWORD elapsed = GetTickCount() - started;

	if (elapsed >= kSlowStageMs)
		LOG("Stage '%s' took %lu ms", name, static_cast<unsigned long>(elapsed));
}

void LoadOriginalDinput()
{
	std::string path = g_settings.dinputWrapper;
	if (path.empty())
		path = GetSystemDirectoryPath() + "dinput8.dll";

	g_originalDinput = LoadLibraryA(path.c_str());
	if (!g_originalDinput)
	{
		LOG("Could not load the original dinput8 from %s (error %lu)", path.c_str(), GetLastError());
		return;
	}

	if (g_originalDinput == GetModModuleHandle())
	{
		LOG("%s resolved back to this mod, so there is nothing to chain to", path.c_str());
		g_originalDinput = nullptr;
		return;
	}

	g_originalDirectInput8Create = reinterpret_cast<DirectInput8Create_t>(
		GetProcAddress(g_originalDinput, "DirectInput8Create"));

	if (!g_originalDirectInput8Create)
	{
		LOG("DirectInput8Create not found in %s", path.c_str());
		return;
	}

	LOG("Original dinput8 loaded from %s", path.c_str());
}

bool EnsureOriginalDinputLoaded()
{
	AcquireSRWLockExclusive(&g_dinputLock);

	if (!g_dinputLoadAttempted)
	{
		g_dinputLoadAttempted = true;
		LoadOriginalDinput();
	}

	const bool ready = g_originalDirectInput8Create != nullptr;
	ReleaseSRWLockExclusive(&g_dinputLock);
	return ready;
}

void Stage_Settings()
{
	Settings::Load();
}

void Stage_MemoryMap()
{
	g_mapped = MemoryMap::Initialize();
}

void Stage_StageTable()
{
	StageTable::Initialize();
}

void Stage_FileOverrides()
{
	if (!g_settings.modFilesEnabled)
	{
		LOG("Mods are turned off in the ini ([ModFiles] Enabled = 0)");
		return;
	}

	if (!g_mapped)
		return;

	ModFiles::Initialize();
}

void Stage_UpdateCheck()
{
	DeviceHooks::AddListener(UpdateInstall::Listener());
	UpdateCheck::Start();
}

void Stage_Overlay()
{
	ProcessTuning::Initialize();
	Profiler::SetEnabled(g_settings.profiler);

	DeviceHooks::AddListener(FrameMeterHud::Listener());
	DeviceHooks::AddListener(&WindowManager::GetInstance());
	D3D9Hooks::Install();
}

void Install()
{
	CreateModDirectories();
	RunStage("settings", Stage_Settings);
	OpenLogger();
	InstallCrashHandler();

	LOG("%s %s starting, loaded as %s", MBTL_IM_NAME, MBTL_IM_VERSION, D3D9Proxy::LoadedAs());
	Compat::Detect();

	if (!HookManager::Initialize())
	{
		LOG("HookManager could not start, so nothing is modded this run");
		return;
	}

	RunStage("memory map", Stage_MemoryMap);
	RunStage("stage table", Stage_StageTable);
	RunStage("file overrides", Stage_FileOverrides);
	RunStage("stages", StagesModule::Install);
	RunStage("music", MusicModule::Install);
	RunStage("training", TrainingModule::Install);
	RunStage("network", NetworkModule::Install);
	RunStage("performance", PerformanceModule::Install);
	RunStage("palettes", PaletteModule::Install);
	RunStage("hidden characters", HiddenCharacters::Install);
	RunStage("save data guard", SaveDataGuard::Install);
	RunStage("character index guard", CharacterIndexGuard::Install);
	RunStage("game asserts", GameAsserts::Install);
	RunStage("game restart", GameRestart::Install);
	RunStage("update check", Stage_UpdateCheck);
	RunStage("overlay", Stage_Overlay);

	HookManager::EnableAllHooks();
	ImageScanner::ReleaseCallIndex();

	LOG("Initialization finished");
}

}

extern "C" HRESULT WINAPI DirectInput8Create(HINSTANCE instance, DWORD version, const GUID& riid, void** out,
	void* outer)
{
	if (!EnsureOriginalDinputLoaded())
		return E_FAIL;

	return g_originalDirectInput8Create(instance, version, riid, out, outer);
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reasonForCall, LPVOID reserved)
{
	if (reasonForCall == DLL_PROCESS_ATTACH)
	{
		SetModModuleHandle(module);
		DisableThreadLibraryCalls(module);

		char mutexName[64] = {};
		sprintf_s(mutexName, "MBTL_IM_instance_%lu", GetCurrentProcessId());

		g_instanceMutex = CreateMutexA(nullptr, TRUE, mutexName);
		g_secondInstance = g_instanceMutex && GetLastError() == ERROR_ALREADY_EXISTS;

		if (!g_secondInstance)
			Install();

		return TRUE;
	}

	if (reasonForCall != DLL_PROCESS_DETACH)
		return TRUE;

	if (reserved)
	{
		CloseLogger();
		return TRUE;
	}

	if (g_instanceMutex)
	{
		ReleaseMutex(g_instanceMutex);
		CloseHandle(g_instanceMutex);
		g_instanceMutex = nullptr;
	}

	if (g_secondInstance)
		return TRUE;

	NetworkModule::Shutdown();
	HookManager::Shutdown();

	if (g_originalDinput)
	{
		FreeLibrary(g_originalDinput);
		g_originalDinput = nullptr;
	}

	CloseLogger();
	return TRUE;
}
