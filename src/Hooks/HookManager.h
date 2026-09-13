#pragma once

#include <string>
#include <vector>

namespace HookManager
{
	struct Hook
	{
		std::string label;
		std::string where;
		bool enabled;
	};

	bool Initialize();
	void Shutdown();

	bool CreateHook(void* target, void* detour, void** original, const char* label);
	bool CreateApiHook(const wchar_t* module, const char* function, void* detour, void** original);
	bool CreateAndEnableHook(void* target, void* detour, void** original, const char* label);
	bool EnableAllHooks();

	void Snapshot(std::vector<Hook>& out);
}
