#pragma once

#include "Network/ModHandshake.h"
#include "Network/NetLink.h"

#include <cstdint>

namespace ModPresence
{
	constexpr int kMaxMembers = 16;
	constexpr int kVersionBytes = 32;

	struct Member
	{
		uint64_t id;
		char version[kVersionBytes];
		char stages[ModHandshake::kStageIdBytes];
		bool hasMod;
	};

	void SetStages(const char* id);
	void Tick(const NetLink::Snapshot& snapshot);

	bool InRoom();
	int RoomSize();
	int ModCount();
	bool MemberAt(int index, Member& out);

	bool PeerHasMod(uint64_t id);
	bool RoomAgrees(const char* stages);
}
