#include "Network/SteamInterfaces.h"

#include "Core/logger.h"

#include <windows.h>

#include <cstdio>
#include <cstring>

namespace {

using Accessor_t = void*(__cdecl*)();
using GetSteamId_t = uint64_t(__cdecl*)(void*);
using SetLobbyMemberData_t = void(__cdecl*)(void*, uint64_t, const char*, const char*);
using GetNumLobbyMembers_t = int(__cdecl*)(void*, uint64_t);
using GetLobbyMemberByIndex_t = uint64_t(__cdecl*)(void*, uint64_t, int);
using GetLobbyMemberData_t = const char*(__cdecl*)(void*, uint64_t, uint64_t, const char*);
using GetLobbyData_t = const char*(__cdecl*)(void*, uint64_t, const char*);

constexpr const char* kUserAccessors[] = { "SteamAPI_SteamUser_v023", "SteamAPI_SteamUser_v021",
	"SteamAPI_SteamUser_v020", "SteamAPI_SteamUser_v019" };
constexpr const char* kMatchmakingAccessors[] = { "SteamAPI_SteamMatchmaking_v009" };

HMODULE g_steam = nullptr;
bool g_ready = false;

Accessor_t g_user = nullptr;
Accessor_t g_matchmaking = nullptr;

GetSteamId_t g_getSteamId = nullptr;
SetLobbyMemberData_t g_setLobbyMemberData = nullptr;
GetNumLobbyMembers_t g_getNumLobbyMembers = nullptr;
GetLobbyMemberByIndex_t g_getLobbyMemberByIndex = nullptr;
GetLobbyMemberData_t g_getLobbyMemberData = nullptr;
GetLobbyData_t g_getLobbyData = nullptr;

char g_status[192] = "not started";

template <typename T>
T Resolve(const char* name)
{
	return reinterpret_cast<T>(GetProcAddress(g_steam, name));
}

template <size_t N>
Accessor_t ResolveAny(const char* const (&names)[N])
{
	for (const char* name : names)
	{
		const Accessor_t accessor = Resolve<Accessor_t>(name);

		if (accessor != nullptr)
			return accessor;
	}

	return nullptr;
}

void* Call(Accessor_t accessor)
{
	if (accessor == nullptr)
		return nullptr;

	void* result = nullptr;

	__try
	{
		result = accessor();
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		result = nullptr;
	}

	return result;
}

}

bool SteamInterfaces::Initialize()
{
	if (g_ready)
		return true;

	g_steam = GetModuleHandleA("steam_api.dll");

	if (g_steam == nullptr)
	{
		strncpy_s(g_status, "steam_api.dll is not loaded", _TRUNCATE);
		return false;
	}

	g_user = ResolveAny(kUserAccessors);
	g_matchmaking = ResolveAny(kMatchmakingAccessors);

	g_getSteamId = Resolve<GetSteamId_t>("SteamAPI_ISteamUser_GetSteamID");
	g_setLobbyMemberData = Resolve<SetLobbyMemberData_t>("SteamAPI_ISteamMatchmaking_SetLobbyMemberData");
	g_getNumLobbyMembers = Resolve<GetNumLobbyMembers_t>("SteamAPI_ISteamMatchmaking_GetNumLobbyMembers");
	g_getLobbyMemberByIndex = Resolve<GetLobbyMemberByIndex_t>("SteamAPI_ISteamMatchmaking_GetLobbyMemberByIndex");
	g_getLobbyMemberData = Resolve<GetLobbyMemberData_t>("SteamAPI_ISteamMatchmaking_GetLobbyMemberData");
	g_getLobbyData = Resolve<GetLobbyData_t>("SteamAPI_ISteamMatchmaking_GetLobbyData");

	if (g_matchmaking == nullptr || g_setLobbyMemberData == nullptr || g_getNumLobbyMembers == nullptr)
	{
		strncpy_s(g_status, "steam_api.dll has no flat matchmaking exports", _TRUNCATE);
		LOG("SteamInterfaces: %s", g_status);
		return false;
	}

	g_ready = true;
	sprintf_s(g_status, "flat API resolved%s", g_user != nullptr ? "" : ", own Steam id unavailable");
	LOG("SteamInterfaces: %s", g_status);
	return true;
}

bool SteamInterfaces::IsReady()
{
	return g_ready;
}

uint64_t SteamInterfaces::GetOwnSteamId()
{
	if (!g_ready && !Initialize())
		return 0;

	void* const user = Call(g_user);

	if (user == nullptr || g_getSteamId == nullptr)
		return 0;

	uint64_t id = 0;

	__try
	{
		id = g_getSteamId(user);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		id = 0;
	}

	return id;
}

bool SteamInterfaces::SetLobbyMemberData(uint64_t lobby, const char* key, const char* value)
{
	if (!g_ready || lobby == 0 || key == nullptr || value == nullptr)
		return false;

	void* const matchmaking = Call(g_matchmaking);

	if (matchmaking == nullptr)
		return false;

	__try
	{
		g_setLobbyMemberData(matchmaking, lobby, key, value);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}

	return true;
}

int SteamInterfaces::GetNumLobbyMembers(uint64_t lobby)
{
	if (!g_ready || lobby == 0)
		return 0;

	void* const matchmaking = Call(g_matchmaking);

	if (matchmaking == nullptr)
		return 0;

	int count = 0;

	__try
	{
		count = g_getNumLobbyMembers(matchmaking, lobby);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		count = 0;
	}

	return count;
}

uint64_t SteamInterfaces::GetLobbyMemberByIndex(uint64_t lobby, int index)
{
	if (!g_ready || g_getLobbyMemberByIndex == nullptr || lobby == 0 || index < 0)
		return 0;

	void* const matchmaking = Call(g_matchmaking);

	if (matchmaking == nullptr)
		return 0;

	uint64_t member = 0;

	__try
	{
		member = g_getLobbyMemberByIndex(matchmaking, lobby, index);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		member = 0;
	}

	return member;
}

const char* SteamInterfaces::GetLobbyMemberData(uint64_t lobby, uint64_t member, const char* key)
{
	if (!g_ready || g_getLobbyMemberData == nullptr || lobby == 0 || member == 0 || key == nullptr)
		return "";

	void* const matchmaking = Call(g_matchmaking);

	if (matchmaking == nullptr)
		return "";

	const char* value = nullptr;

	__try
	{
		value = g_getLobbyMemberData(matchmaking, lobby, member, key);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		value = nullptr;
	}

	return value != nullptr ? value : "";
}

const char* SteamInterfaces::GetLobbyData(uint64_t lobby, const char* key)
{
	if (!g_ready || g_getLobbyData == nullptr || lobby == 0 || key == nullptr)
		return "";

	void* const matchmaking = Call(g_matchmaking);

	if (matchmaking == nullptr)
		return "";

	const char* value = nullptr;

	__try
	{
		value = g_getLobbyData(matchmaking, lobby, key);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		value = nullptr;
	}

	return value != nullptr ? value : "";
}

const char* SteamInterfaces::StatusText()
{
	return g_status;
}
