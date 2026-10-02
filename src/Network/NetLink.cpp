#include "Network/NetLink.h"

#include "Core/utils.h"
#include "Game/Anchors.h"
#include "Game/GameOffsets.h"
#include "Hooks/HookManager.h"
#include "Network/GgpoLayout.h"
#include "Network/LobbyWatch.h"
#include "Network/NetLog.h"
#include "Network/SteamLink.h"
#include "Training/BattleMap.h"

namespace {

namespace Netplay = GameOffsets::Netplay;

constexpr int kMessagesConnected = 3;
constexpr int kMessagesClosed = 4;

NetLink::Snapshot g_now = {};
NetLink::Snapshot g_shared = {};
SRWLOCK g_lock = SRWLOCK_INIT;

using AddPlayer_t = int(__fastcall*)(void*, void*, const uint8_t*, void*, const uint64_t*);

AddPlayer_t oAddPlayer = nullptr;
bool g_hooked = false;

volatile LONG g_side = -1;
volatile LONG64 g_peer = 0;

void LogMatchWhenKnown()
{
	const LONG side = g_side;
	const LONG64 peer = g_peer;

	if (side < 0 || peer == 0)
		return;

	NetLog::Write("ggpo match started with this machine as p%d and %llu across", static_cast<int>(side) + 1,
		static_cast<unsigned long long>(peer));
}

void NoteLocal(int32_t number)
{
	if (number < Netplay::kFirstNumber || number > Netplay::kSides)
		return;

	InterlockedExchange(&g_side, number - Netplay::kFirstNumber);
	LogMatchWhenKnown();
}

void NoteRemote(const uint64_t* steamId)
{
	uint64_t peer = 0;

	if (steamId == nullptr || !TryReadMemory(&peer, steamId, sizeof(peer)) || peer == 0)
		return;

	InterlockedExchange64(&g_peer, static_cast<LONG64>(peer));
	LogMatchWhenKnown();
}

void NotePlayer(const uint8_t* player, const uint64_t* steamId)
{
	const auto record = reinterpret_cast<uintptr_t>(player);
	int32_t type = -1;
	int32_t number = 0;

	if (player == nullptr || !TryRead(record + Netplay::kPlayerType, type))
		return;

	if (type == Netplay::kPlayerRemote)
	{
		NoteRemote(steamId);
		return;
	}

	if (type == Netplay::kPlayerLocal && TryRead(record + Netplay::kPlayerNumber, number))
		NoteLocal(number);
}

int __fastcall HookedAddPlayer(void* backend, void* unused, const uint8_t* player, void* handle,
	const uint64_t* steamId)
{
	NotePlayer(player, steamId);
	return oAddPlayer(backend, unused, player, handle, steamId);
}

uint32_t StateOf(const SteamLink::Sample& sample)
{
	if (sample.sessionError != 0 || (sample.messagesRead && sample.messagesState >= kMessagesClosed))
		return NetLink::State_Disconnected;

	if ((sample.messagesRead && sample.messagesState == kMessagesConnected) ||
		(sample.p2pRead && sample.connectionActive))
	{
		return NetLink::State_Running;
	}

	if (sample.connecting || (sample.messagesRead && sample.messagesState > 0))
		return NetLink::State_Synchronized;

	return NetLink::State_Syncing;
}

bool ReadFromGgpo(uintptr_t backend, uint64_t peer, NetLink::Snapshot& out)
{
	if (!GgpoLayout::IsPlayerBackend(backend))
		return false;

	out.players = GgpoLayout::PlayerCount(backend);
	out.spectators = GgpoLayout::SpectatorCount(backend);
	out.synchronizing = GgpoLayout::IsSynchronizing(backend);

	GgpoLayout::Peer found = {};

	if (!GgpoLayout::ReadPeer(backend, peer, found))
		return false;

	out.ggpoRead = true;
	out.hasPeer = true;
	out.peer.id = found.id;
	out.peer.state = found.state;
	out.peer.pending = found.pending;
	out.peer.ping = found.ping;
	out.peer.kbps = found.kbps;
	out.peer.localBehind = found.localBehind;
	out.peer.remoteBehind = found.remoteBehind;
	return true;
}

void ReadFromSteam(uint64_t peer, NetLink::Snapshot& out)
{
	SteamLink::Sample sample = {};
	SteamLink::Take(sample);

	const bool measured = sample.valid && sample.peer == peer;

	out.hasPeer = true;
	out.peer.id = peer;
	out.peer.state = measured ? StateOf(sample) : NetLink::State_Syncing;
	out.peer.pending = measured ? sample.pendingReliable + sample.pendingUnreliable : 0;
	out.peer.ping = measured ? sample.ping : 0;
}

void ReadPeer(NetLink::Snapshot& out)
{
	const uint64_t peer = static_cast<uint64_t>(g_peer);

	if (peer == 0 || out.session == 0)
		return;

	if (ReadFromGgpo(out.session, peer, out))
		return;

	ReadFromSteam(peer, out);
}

NetLink::Snapshot Read(const NetLink::Snapshot& previous)
{
	NetLink::Snapshot out = {};
	out.tick = GetTickCount();
	out.lastPeer = previous.lastPeer;
	out.peerSeenAt = previous.peerSeenAt;
	out.presentSeen = previous.presentSeen;

	const uintptr_t sessionAddress = BattleMap::Addresses().session;

	out.resolved = sessionAddress != 0;

	if (out.resolved)
		TryRead(sessionAddress, out.session);

	out.ownSide = static_cast<int>(g_side);

	ReadPeer(out);

	if (out.hasPeer)
	{
		out.lastPeer = out.peer.id;
		out.peerSeenAt = out.tick;
	}

	out.lobby = LobbyWatch::Current();
	return out;
}

void Forget()
{
	InterlockedExchange(&g_side, -1);
	InterlockedExchange64(&g_peer, 0);
}

void LogSession(const NetLink::Snapshot& before, const NetLink::Snapshot& after)
{
	if (before.session == after.session)
		return;

	NetLog::Write("ggpo session 0x%08x -> 0x%08x, %s, players %d, spectators %d", before.session, after.session,
		after.session == 0 ? "gone" : (after.ggpoRead ? "layout read" : "layout NOT read, Steam is the only view"),
		after.players, after.spectators);
}

void LogPeer(const NetLink::Snapshot& before, const NetLink::Snapshot& after)
{
	if (after.hasPeer && (!before.hasPeer || before.peer.id != after.peer.id))
	{
		NetLog::Write("peer %llu connected, link %s, ping %d ms, %d kbps", static_cast<unsigned long long>(after.peer.id),
			NetLink::StateName(after.peer.state), after.peer.ping, after.peer.kbps);
		return;
	}

	if (before.hasPeer && !after.hasPeer)
	{
		NetLog::Write("peer %llu gone, last link %s, pending %d", static_cast<unsigned long long>(before.peer.id),
			NetLink::StateName(before.peer.state), before.peer.pending);
		return;
	}

	if (after.hasPeer && before.peer.state != after.peer.state)
	{
		NetLog::Write("peer %llu link %s -> %s, pending %d, ping %d ms", static_cast<unsigned long long>(after.peer.id),
			NetLink::StateName(before.peer.state), NetLink::StateName(after.peer.state), after.peer.pending,
			after.peer.ping);
	}
}

void LogSync(const NetLink::Snapshot& before, const NetLink::Snapshot& after)
{
	if (before.synchronizing == after.synchronizing || !after.ggpoRead)
		return;

	NetLog::Write("ggpo synchronizing %s", after.synchronizing ? "started" : "ended");
}

void LogLobby(const NetLink::Snapshot& before, const NetLink::Snapshot& after)
{
	if (before.lobby == after.lobby)
		return;

	NetLog::Write("lobby %llu -> %llu", static_cast<unsigned long long>(before.lobby),
		static_cast<unsigned long long>(after.lobby));
}

}

bool NetLink::Install()
{
	uint8_t* const addPlayer = GgpoLayout::AddPlayerFunction();

	Anchors::Record("GGPO add player", reinterpret_cast<uintptr_t>(addPlayer), "online link");

	if (addPlayer == nullptr)
		return false;

	g_hooked = HookManager::CreateHook(addPlayer, reinterpret_cast<void*>(&HookedAddPlayer),
		reinterpret_cast<void**>(&oAddPlayer), "GGPO add player");

	return g_hooked;
}

bool NetLink::IsHooked()
{
	return g_hooked;
}

void NetLink::OnPresent()
{
	g_now.presentSeen = true;
}

void NetLink::Update()
{
	const Snapshot before = g_now;
	const Snapshot after = Read(before);

	if (before.session != 0 && after.session == 0)
		Forget();

	LogSession(before, after);
	LogSync(before, after);
	LogPeer(before, after);
	LogLobby(before, after);

	g_now = after;

	AcquireSRWLockExclusive(&g_lock);
	g_shared = after;
	ReleaseSRWLockExclusive(&g_lock);
}

const NetLink::Snapshot& NetLink::Current()
{
	return g_now;
}

void NetLink::Copy(Snapshot& out)
{
	AcquireSRWLockShared(&g_lock);
	out = g_shared;
	ReleaseSRWLockShared(&g_lock);
}

bool NetLink::InSession(const Snapshot& snapshot)
{
	if (snapshot.session != 0)
		return true;

	return snapshot.lastPeer != 0 && GetTickCount() - snapshot.peerSeenAt < kSessionHoldMs;
}

bool NetLink::InSession()
{
	return InSession(g_now);
}

bool NetLink::HasPeer()
{
	return g_now.hasPeer;
}

uint64_t NetLink::Peer()
{
	return g_now.hasPeer ? g_now.peer.id : g_now.lastPeer;
}

bool NetLink::PeerSeenWithin(DWORD milliseconds)
{
	return g_now.lastPeer != 0 && GetTickCount() - g_now.peerSeenAt < milliseconds;
}

uint64_t NetLink::Lobby()
{
	return g_now.lobby;
}

int NetLink::OwnSide()
{
	return g_now.ownSide;
}

bool NetLink::ReadsGgpo()
{
	return g_now.ggpoRead;
}

const char* NetLink::StateName(uint32_t state)
{
	switch (state)
	{
	case State_Synchronized:
		return "synchronized";
	case State_Running:
		return "running";
	case State_Disconnected:
		return "disconnected";
	default:
		return "syncing";
	}
}
