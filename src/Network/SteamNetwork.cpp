#include "Network/SteamNetwork.h"

#include "Core/logger.h"
#include "Core/utils.h"
#include "Game/Anchors.h"
#include "Game/GameOffsets.h"
#include "Hooks/HookManager.h"
#include "Hooks/ImageScanner.h"
#include "Training/BattleMap.h"
#include "Training/GameState.h"

#include <windows.h>

#include <cstdio>
#include <cstring>
#include <set>

namespace {

namespace Netplay = GameOffsets::Netplay;

using Interface_t = void* (__cdecl*)();
using Send_t = int(__cdecl*)(void*, const uint8_t*, const void*, uint32_t, int, int);
using Receive_t = int(__cdecl*)(void*, int, void**, int);
using Release_t = void(__cdecl*)(void*);
using SetIdentity_t = void(__cdecl*)(uint8_t*, uint64_t);
using GetIdentity_t = uint64_t(__cdecl*)(const uint8_t*);

struct Api
{
	Interface_t messages;
	Send_t send;
	Receive_t receive;
	Release_t release;
	SetIdentity_t setIdentity;
	GetIdentity_t getIdentity;
	void* self;
};

Api g_api = {};
bool g_ready = false;
bool g_hooked = false;
int g_attempts = 0;

void* oStartPlayers = nullptr;

volatile int g_side = -1;
volatile uint64_t g_peer = 0;
bool g_sessionSeen = false;

char g_status[160] = "not started";

void __cdecl NoteStart(const uint32_t* args)
{
	const int players = static_cast<int>(args[Netplay::kArgPlayers]);
	const uintptr_t records = args[Netplay::kArgRecords];
	const uintptr_t ids = args[Netplay::kArgSteamIds];

	if (players <= 0 || players > Netplay::kMostPlayers || records == 0 || ids == 0)
		return;

	int local = -1;
	int remote = -1;
	int32_t number = 0;

	for (int i = 0; i < players; ++i)
	{
		int32_t type = -1;
		const uintptr_t record = records + i * Netplay::kPlayerRecordBytes;

		if (!TryRead(record + Netplay::kPlayerType, type))
			return;

		if (type == Netplay::kPlayerLocal && local < 0)
		{
			local = i;
			TryRead(record + Netplay::kPlayerNumber, number);
			continue;
		}

		if (remote < 0)
			remote = i;
	}

	uint64_t peer = 0;

	if (local < 0 || remote < 0 || !TryReadMemory(&peer, reinterpret_cast<const void*>(ids + remote * sizeof(peer)),
		sizeof(peer)))
	{
		return;
	}

	g_side = number >= 1 && number <= 2 ? number - 1 : local;
	g_peer = peer;

	LOG("SteamNetwork: the match started with this machine as p%d and %llu across", g_side + 1,
		static_cast<unsigned long long>(peer));
}

__declspec(naked) void HookedStartPlayers()
{
	__asm
	{
		pushad
		lea eax, [esp + 36]
		push eax
		call NoteStart
		add esp, 4
		popad
		jmp dword ptr [oStartPlayers]
	}
}

bool WalksPlayerRecords(const uint8_t* function)
{
	const size_t length = ImageScanner::FunctionLength(function);

	for (size_t i = 0; i + Netplay::kImulFrameLength <= length; ++i)
	{
		if (function[i] == Netplay::kImulFrame && (function[i + 1] & Netplay::kImulFrameModRmMask) == Netplay::kImulFrameModRm &&
			function[i + Netplay::kImulFrameLength - 1] == Netplay::kPlayerRecordBytes)
		{
			return true;
		}
	}

	return false;
}

uint8_t* FindStartPlayers(uintptr_t session)
{
	uint8_t pattern[5] = { Netplay::kPushImmediate };
	std::memcpy(pattern + 1, &session, sizeof(uint32_t));

	std::set<uint8_t*> found;

	for (uint8_t* site : ImageScanner::FindBytes(ImageScanner::Code(), pattern, sizeof(pattern)))
	{
		uint8_t* const start = ImageScanner::FunctionStart(site);

		if (start != nullptr && WalksPlayerRecords(start))
			found.insert(start);
	}

	if (found.size() == 1)
		return *found.begin();

	LOG("SteamNetwork: the GGPO player start has %u candidate(s), expected exactly one",
		static_cast<unsigned>(found.size()));
	return nullptr;
}

FARPROC Export(HMODULE library, const char* name)
{
	const FARPROC found = GetProcAddress(library, name);

	if (found == nullptr)
		LOG("SteamNetwork: %s is missing from %s", name, Netplay::kSteamLibrary);

	return found;
}

void* CallInterface(Interface_t messages)
{
	__try
	{
		return messages();
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return nullptr;
	}
}

bool Initialize()
{
	if (g_ready || g_attempts >= Netplay::kMostAttempts)
		return g_ready;

	++g_attempts;

	const HMODULE library = GetModuleHandleA(Netplay::kSteamLibrary);

	if (library == nullptr)
	{
		strncpy_s(g_status, "steam_api.dll is not loaded", _TRUNCATE);
		return false;
	}

	g_api.messages = reinterpret_cast<Interface_t>(Export(library, "SteamAPI_SteamNetworkingMessages_SteamAPI_v002"));
	g_api.send = reinterpret_cast<Send_t>(Export(library, "SteamAPI_ISteamNetworkingMessages_SendMessageToUser"));
	g_api.receive = reinterpret_cast<Receive_t>(Export(library, "SteamAPI_ISteamNetworkingMessages_ReceiveMessagesOnChannel"));
	g_api.release = reinterpret_cast<Release_t>(Export(library, "SteamAPI_SteamNetworkingMessage_t_Release"));
	g_api.setIdentity = reinterpret_cast<SetIdentity_t>(Export(library, "SteamAPI_SteamNetworkingIdentity_SetSteamID64"));
	g_api.getIdentity = reinterpret_cast<GetIdentity_t>(Export(library, "SteamAPI_SteamNetworkingIdentity_GetSteamID64"));

	if (!g_api.messages || !g_api.send || !g_api.receive || !g_api.release || !g_api.setIdentity || !g_api.getIdentity)
	{
		strncpy_s(g_status, "steam_api.dll lacks the networking calls", _TRUNCATE);
		g_attempts = Netplay::kMostAttempts;
		return false;
	}

	g_api.self = CallInterface(g_api.messages);

	if (g_api.self == nullptr)
	{
		strncpy_s(g_status, "Steam networking is not up yet", _TRUNCATE);
		return false;
	}

	g_ready = true;
	sprintf_s(g_status, "Steam networking ready, channel %d", SteamNetwork::kChannel);
	LOG("SteamNetwork: %s", g_status);
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
			return g_api.getIdentity(reinterpret_cast<const uint8_t*>(message + at));
		}
	}

	return 0;
}

void Forget()
{
	g_side = -1;
	g_peer = 0;
	g_sessionSeen = false;
}

}

bool SteamNetwork::Install()
{
	const uintptr_t session = BattleMap::Addresses().session;
	uint8_t* const start = session != 0 ? FindStartPlayers(session) : nullptr;

	Anchors::Record("GGPO player start", reinterpret_cast<uintptr_t>(start), "online palettes");

	if (start == nullptr)
		return false;

	g_hooked = HookManager::CreateHook(start, reinterpret_cast<void*>(&HookedStartPlayers), &oStartPlayers,
		"GGPO player start");

	return g_hooked;
}

void SteamNetwork::OnFrame()
{
	const bool online = GameState::IsOnline();

	if (online)
	{
		g_sessionSeen = true;
		Initialize();
		return;
	}

	if (g_sessionSeen)
		Forget();
}

bool SteamNetwork::IsHooked()
{
	return g_hooked;
}

bool SteamNetwork::IsReady()
{
	return g_ready;
}

uint64_t SteamNetwork::GetPeer()
{
	return g_peer;
}

bool SteamNetwork::HasPeer()
{
	return g_ready && g_peer != 0 && GameState::IsOnline();
}

int SteamNetwork::GetOwnSide()
{
	return g_side;
}

bool SteamNetwork::Send(const void* data, int size)
{
	if (!HasPeer() || data == nullptr || size <= 0)
		return false;

	uint8_t identity[Netplay::kIdentityBytes] = {};
	g_api.setIdentity(identity, g_peer);

	return g_api.send(g_api.self, identity, data, static_cast<uint32_t>(size), Netplay::kSendReliable, kChannel) ==
		Netplay::kResultOk;
}

bool SteamNetwork::Receive(void* buffer, int capacity, int& outSize, uint64_t& outPeer)
{
	outSize = 0;
	outPeer = 0;

	void* message = nullptr;

	if (!g_ready || buffer == nullptr || capacity <= 0 || g_api.receive(g_api.self, kChannel, &message, 1) <= 0 ||
		message == nullptr)
	{
		return false;
	}

	const uintptr_t at = reinterpret_cast<uintptr_t>(message);
	uint32_t data = 0;
	int32_t size = 0;

	outPeer = PeerOf(at);

	if (TryRead(at + Netplay::kMessageData, data) && TryRead(at + Netplay::kMessageSize, size) && data != 0 && size > 0 &&
		size <= capacity && TryReadMemory(buffer, reinterpret_cast<const void*>(static_cast<uintptr_t>(data)), size))
	{
		outSize = size;
	}

	g_api.release(message);
	return true;
}

const char* SteamNetwork::StatusText()
{
	return g_status;
}
