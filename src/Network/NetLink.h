#pragma once

#include <windows.h>

#include <cstdint>

namespace NetLink
{
	enum State
	{
		State_Syncing,
		State_Synchronized,
		State_Running,
		State_Disconnected
	};

	struct Endpoint
	{
		uint64_t id;
		uint32_t state;
		int pending;
		int ping;
		int kbps;
		int localBehind;
		int remoteBehind;
	};

	struct Snapshot
	{
		DWORD tick;
		bool resolved;
		uint32_t session;
		bool ggpoRead;
		bool synchronizing;
		int players;
		int spectators;
		bool hasPeer;
		Endpoint peer;
		uint64_t lastPeer;
		DWORD peerSeenAt;
		uint64_t lobby;
		int ownSide;
		bool presentSeen;
	};

	constexpr DWORD kSessionHoldMs = 5000;

	bool Install();
	bool IsHooked();

	void OnPresent();
	void Update();

	const Snapshot& Current();
	void Copy(Snapshot& out);

	bool InSession(const Snapshot& snapshot);
	bool InSession();

	bool HasPeer();
	uint64_t Peer();
	bool PeerSeenWithin(DWORD milliseconds);

	uint64_t Lobby();
	int OwnSide();
	bool ReadsGgpo();

	const char* StateName(uint32_t state);
}
