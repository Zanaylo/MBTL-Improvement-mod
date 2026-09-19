#include "Network/SteamNetwork.h"

#include "Core/utils.h"
#include "Game/GameOffsets.h"
#include "Network/NetLog.h"

#include <windows.h>

#include <cstdio>
#include <cstring>

namespace {

namespace Netplay = GameOffsets::Netplay;

using Accessor_t = void*(__cdecl*)();
using Send_t = int(__cdecl*)(void*, const uint8_t*, const void*, uint32_t, int, int);
using Receive_t = int(__cdecl*)(void*, int, void**, int);
using Release_t = void(__cdecl*)(void*);
using SetIdentity_t = void(__cdecl*)(uint8_t*, uint64_t);
using GetIdentity_t = uint64_t(__cdecl*)(const uint8_t*);

Accessor_t g_accessor = nullptr;
Send_t g_send = nullptr;
Receive_t g_receive = nullptr;
Release_t g_release = nullptr;
SetIdentity_t g_setIdentity = nullptr;
GetIdentity_t g_getIdentity = nullptr;

volatile LONG g_ready = 0;
char g_status[160] = "not started";

void* Messages()
{
	void* messages = nullptr;

	__try
	{
		messages = g_accessor();
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		messages = nullptr;
	}

	return messages;
}

bool Resolve()
{
	if (g_accessor != nullptr)
		return true;

	const HMODULE steam = GetModuleHandleA(Netplay::kSteamLibrary);

	if (steam == nullptr)
	{
		strncpy_s(g_status, "steam_api.dll is not loaded", _TRUNCATE);
		return false;
	}

	const Accessor_t accessor =
		reinterpret_cast<Accessor_t>(GetProcAddress(steam, "SteamAPI_SteamNetworkingMessages_SteamAPI_v002"));

	g_send = reinterpret_cast<Send_t>(GetProcAddress(steam, "SteamAPI_ISteamNetworkingMessages_SendMessageToUser"));
	g_receive =
		reinterpret_cast<Receive_t>(GetProcAddress(steam, "SteamAPI_ISteamNetworkingMessages_ReceiveMessagesOnChannel"));
	g_release = reinterpret_cast<Release_t>(GetProcAddress(steam, "SteamAPI_SteamNetworkingMessage_t_Release"));
	g_setIdentity = reinterpret_cast<SetIdentity_t>(GetProcAddress(steam, "SteamAPI_SteamNetworkingIdentity_SetSteamID64"));
	g_getIdentity = reinterpret_cast<GetIdentity_t>(GetProcAddress(steam, "SteamAPI_SteamNetworkingIdentity_GetSteamID64"));

	if (accessor == nullptr || g_send == nullptr || g_receive == nullptr || g_release == nullptr ||
		g_setIdentity == nullptr || g_getIdentity == nullptr)
	{
		strncpy_s(g_status, "steam_api.dll lacks the networking exports", _TRUNCATE);
		return false;
	}

	g_accessor = accessor;
	return true;
}

uint64_t PeerOf(uintptr_t message)
{
	for (uintptr_t at : Netplay::kMessageIdentityOffsets)
	{
		int32_t type = 0;
		int32_t size = 0;

		if (TryRead(message + at, type) && TryRead(message + at + sizeof(type), size) &&
			type == Netplay::kIdentitySteamId && size == Netplay::kIdentitySteamIdBytes)
		{
			return g_getIdentity(reinterpret_cast<const uint8_t*>(message + at));
		}
	}

	return 0;
}

}

bool SteamNetwork::Initialize()
{
	if (g_ready != 0)
		return true;

	if (!Resolve())
		return false;

	if (Messages() == nullptr)
	{
		strncpy_s(g_status, "Steam networking is not ready yet", _TRUNCATE);
		return false;
	}

	InterlockedExchange(&g_ready, 1);
	sprintf_s(g_status, "ready, mod channel %d open", kChannel);
	NetLog::Write("steam networking ready, mod channel %d", kChannel);
	return true;
}

bool SteamNetwork::IsReady()
{
	return g_ready != 0;
}

bool SteamNetwork::SendTo(uint64_t steamId, const void* data, int size)
{
	if (g_ready == 0 || steamId == 0 || data == nullptr || size <= 0)
		return false;

	void* const messages = Messages();

	if (messages == nullptr)
		return false;

	uint8_t identity[Netplay::kIdentityBytes] = {};
	int result = 0;

	__try
	{
		g_setIdentity(identity, steamId);
		result = g_send(messages, identity, data, static_cast<uint32_t>(size), Netplay::kSendReliable, kChannel);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		result = 0;
	}

	return result == Netplay::kResultOk;
}

bool SteamNetwork::Receive(void* buffer, int capacity, int& outSize, uint64_t& outPeer)
{
	outSize = 0;
	outPeer = 0;

	if (g_ready == 0 || buffer == nullptr || capacity <= 0)
		return false;

	void* const messages = Messages();

	if (messages == nullptr)
		return false;

	void* message = nullptr;
	int taken = 0;

	__try
	{
		taken = g_receive(messages, kChannel, &message, 1);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		taken = 0;
	}

	if (taken <= 0 || message == nullptr)
		return false;

	const uintptr_t at = reinterpret_cast<uintptr_t>(message);
	uint32_t data = 0;
	int32_t size = 0;

	__try
	{
		outPeer = PeerOf(at);

		if (TryRead(at + Netplay::kMessageData, data) && TryRead(at + Netplay::kMessageSize, size) && data != 0 &&
			size > 0 && size <= capacity &&
			TryReadMemory(buffer, reinterpret_cast<const void*>(static_cast<uintptr_t>(data)), size))
		{
			outSize = size;
		}

		g_release(message);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		outSize = 0;
	}

	return true;
}

const char* SteamNetwork::StatusText()
{
	return g_status;
}
