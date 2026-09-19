#include "Network/LobbyWatch.h"

#include "Network/NetLog.h"
#include "Network/SteamInterfaces.h"

#include <windows.h>

namespace {

constexpr int kEnterSuccess = 1;
constexpr int kLeftStates = 0x02 | 0x04 | 0x08 | 0x10;

volatile LONG64 g_lobby = 0;
uint64_t g_self = 0;

uint64_t Self()
{
	if (g_self == 0)
		g_self = SteamInterfaces::GetOwnSteamId();

	return g_self;
}

void Store(uint64_t lobby)
{
	InterlockedExchange64(&g_lobby, static_cast<LONG64>(lobby));
}

}

void LobbyWatch::Entered(uint64_t lobby, int response)
{
	if (lobby == 0 || response != kEnterSuccess)
		return;

	Store(lobby);
	NetLog::Write("lobby: joined %llu", static_cast<unsigned long long>(lobby));
}

void LobbyWatch::MemberChanged(uint64_t lobby, uint64_t member, int stateChange)
{
	const uint64_t self = Self();

	if (lobby == 0 || member == 0 || member != self || (stateChange & kLeftStates) == 0)
		return;

	if (static_cast<uint64_t>(g_lobby) != lobby)
		return;

	Store(0);
	NetLog::Write("lobby: left %llu", static_cast<unsigned long long>(lobby));
}

uint64_t LobbyWatch::Current()
{
	return static_cast<uint64_t>(g_lobby);
}
