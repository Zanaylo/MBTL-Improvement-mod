#include "Hooks/HookManager.h"

#include "Core/logger.h"
#include "Core/utils.h"

#include <MinHook.h>

#include <windows.h>

namespace {

struct Record
{
	void* target;
	std::string label;
	bool enabled;
};

std::vector<Record> g_hooks;
SRWLOCK g_lock = SRWLOCK_INIT;
bool g_initialized = false;

void Remember(void* target, const char* label)
{
	AcquireSRWLockExclusive(&g_lock);
	g_hooks.push_back({ target, label, false });
	ReleaseSRWLockExclusive(&g_lock);
}

void MarkEnabled(void* target)
{
	AcquireSRWLockExclusive(&g_lock);

	for (Record& record : g_hooks)
	{
		if (target == MH_ALL_HOOKS || record.target == target)
			record.enabled = true;
	}

	ReleaseSRWLockExclusive(&g_lock);
}

bool Ready(void* detour, void** original, const char* label)
{
	if (g_initialized && detour && original)
		return true;

	LOG("Hook '%s' refused: the hook manager is not ready or an address is missing", label);
	return false;
}

}

bool HookManager::Initialize()
{
	if (g_initialized)
		return true;

	const MH_STATUS status = MH_Initialize();
	if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED)
	{
		LOG("MH_Initialize failed: %s", MH_StatusToString(status));
		return false;
	}

	g_initialized = true;
	return true;
}

void HookManager::Shutdown()
{
	if (!g_initialized)
		return;

	MH_DisableHook(MH_ALL_HOOKS);
	MH_Uninitialize();

	AcquireSRWLockExclusive(&g_lock);
	g_hooks.clear();
	ReleaseSRWLockExclusive(&g_lock);

	g_initialized = false;
}

bool HookManager::CreateHook(void* target, void* detour, void** original, const char* label)
{
	if (!Ready(detour, original, label) || !target)
		return false;

	const MH_STATUS status = MH_CreateHook(target, detour, original);
	if (status != MH_OK)
	{
		LOG("MH_CreateHook '%s' failed: %s", label, MH_StatusToString(status));
		return false;
	}

	Remember(target, label);
	LOG("Hook '%s' created at %s", label, DescribeAddress(reinterpret_cast<uintptr_t>(target)).c_str());
	return true;
}

bool HookManager::CreateApiHook(const wchar_t* module, const char* function, void* detour, void** original)
{
	if (!Ready(detour, original, function))
		return false;

	void* target = nullptr;
	const MH_STATUS status = MH_CreateHookApiEx(module, function, detour, original, &target);
	if (status != MH_OK)
	{
		LOG("MH_CreateHookApi '%s' failed: %s", function, MH_StatusToString(status));
		return false;
	}

	Remember(target, function);
	LOG("Hook '%s' created at %s", function, DescribeAddress(reinterpret_cast<uintptr_t>(target)).c_str());
	return true;
}

bool HookManager::CreateAndEnableHook(void* target, void* detour, void** original, const char* label)
{
	if (!CreateHook(target, detour, original, label))
		return false;

	const MH_STATUS status = MH_EnableHook(target);
	if (status != MH_OK)
	{
		LOG("MH_EnableHook '%s' failed: %s", label, MH_StatusToString(status));
		return false;
	}

	MarkEnabled(target);
	return true;
}

bool HookManager::EnableAllHooks()
{
	if (!g_initialized)
		return false;

	const MH_STATUS status = MH_EnableHook(MH_ALL_HOOKS);
	if (status != MH_OK)
	{
		LOG("MH_EnableHook failed: %s", MH_StatusToString(status));
		return false;
	}

	MarkEnabled(MH_ALL_HOOKS);
	LOG("Hooks enabled");
	return true;
}

void HookManager::Snapshot(std::vector<Hook>& out)
{
	out.clear();

	AcquireSRWLockShared(&g_lock);

	for (const Record& record : g_hooks)
		out.push_back({ record.label, DescribeAddress(reinterpret_cast<uintptr_t>(record.target)), record.enabled });

	ReleaseSRWLockShared(&g_lock);
}
