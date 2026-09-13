#pragma once

#include <cstdint>

namespace SteamNetwork
{
	constexpr int kChannel = 0x504C;

	bool Install();
	void OnFrame();

	bool IsHooked();
	bool IsReady();

	uint64_t GetPeer();
	bool HasPeer();
	int GetOwnSide();

	bool Send(const void* data, int size);
	bool Receive(void* buffer, int capacity, int& outSize, uint64_t& outPeer);

	const char* StatusText();
}
