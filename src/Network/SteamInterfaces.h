#pragma once

#include <cstdint>

namespace SteamInterfaces
{
	bool Initialize();
	bool IsReady();

	uint64_t GetOwnSteamId();

	bool SetLobbyMemberData(uint64_t lobby, const char* key, const char* value);

	int GetNumLobbyMembers(uint64_t lobby);
	uint64_t GetLobbyMemberByIndex(uint64_t lobby, int index);
	const char* GetLobbyMemberData(uint64_t lobby, uint64_t member, const char* key);
	const char* GetLobbyData(uint64_t lobby, const char* key);

	const char* StatusText();
}
