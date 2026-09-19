#pragma once

#include <cstdint>

namespace LobbyWatch
{
	void Entered(uint64_t lobby, int response);
	void MemberChanged(uint64_t lobby, uint64_t member, int stateChange);

	uint64_t Current();
}
