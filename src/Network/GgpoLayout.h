#pragma once

#include <cstdint>

namespace GgpoLayout
{
	struct Peer
	{
		uint64_t id;
		uint32_t state;
		int pending;
		int ping;
		int kbps;
		int localBehind;
		int remoteBehind;
	};

	bool Resolve();
	bool IsResolved();

	bool IsPlayerBackend(uintptr_t backend);

	int PlayerCount(uintptr_t backend);
	int SpectatorCount(uintptr_t backend);
	bool IsSynchronizing(uintptr_t backend);

	bool ReadPeer(uintptr_t backend, uint64_t wanted, Peer& out);

	const char* StatusText();
}
