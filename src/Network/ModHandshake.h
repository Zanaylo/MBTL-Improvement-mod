#pragma once

#include <cstdint>

namespace ModHandshake
{
	constexpr int kStageIdBytes = 24;
	constexpr int kVersionBytes = 16;

	enum PeerState
	{
		Peer_None,
		Peer_Waiting,
		Peer_Modded,
		Peer_Unmodded
	};

	void Initialize();
	void Update();

	void SetLocalStages(const char* id);

	PeerState GetPeerState();
	bool PeerHasMod();
	bool HeardFrom(uint64_t id);

	const char* PeerVersion();
	const char* PeerStages();
	bool PeerSharesStages();

	const char* StatusText();
}
