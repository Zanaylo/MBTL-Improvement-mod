#include "Network/GgpoLayout.h"

#include "Core/utils.h"
#include "Game/Anchors.h"
#include "Game/GameOffsets.h"
#include "Hooks/ImageScanner.h"

#include <cstdio>
#include <cstring>
#include <vector>

namespace {

namespace Ggpo = GameOffsets::Ggpo;

bool g_tried = false;
uintptr_t g_backendVTable = 0;
char g_status[160] = "not resolved yet";

uint8_t* Only(const std::vector<uint8_t*>& found)
{
	return found.size() == 1 ? found.front() : nullptr;
}

std::vector<uint8_t*> ReferencesIn(ImageSection where, const uint8_t* address)
{
	const auto value = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(address));
	return ImageScanner::FindBytes(where, reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

uint8_t* LocatorOf(uint8_t* typeDescriptor)
{
	const ImageSection rdata = ImageScanner::ReadOnlyData();
	uint8_t* locator = nullptr;

	for (uint8_t* site : ReferencesIn(rdata, typeDescriptor))
	{
		uint8_t* const candidate = site - Ggpo::kLocatorTypeDescriptor;

		if (ImageScanner::ReadDword(candidate + Ggpo::kLocatorSignature) != 0 ||
			ImageScanner::ReadDword(candidate + Ggpo::kLocatorOffset) != 0)
		{
			continue;
		}

		if (locator != nullptr)
			return nullptr;

		locator = candidate;
	}

	return locator;
}

uintptr_t Field(uintptr_t backend, uintptr_t offset)
{
	uint32_t value = 0;
	return TryRead(backend + offset, value) ? value : 0;
}

bool ReadEndpoint(uintptr_t endpoint, uint64_t wanted, GgpoLayout::Peer& out)
{
	uint32_t low = 0;
	uint32_t high = 0;

	if (Field(endpoint, Ggpo::kEndpointUdp) == 0 || !TryRead(endpoint + Ggpo::kEndpointSteamLow, low) ||
		!TryRead(endpoint + Ggpo::kEndpointSteamHigh, high))
	{
		return false;
	}

	if (high != Ggpo::kSteamIdIndividualHigh)
		return false;

	const uint64_t id = (static_cast<uint64_t>(high) << 32) | low;

	if (wanted != 0 && id != wanted)
		return false;

	out.id = id;
	out.state = static_cast<uint32_t>(Field(endpoint, Ggpo::kEndpointState));
	out.pending = static_cast<int>(Field(endpoint, Ggpo::kEndpointPendingOutput));
	out.ping = static_cast<int>(Field(endpoint, Ggpo::kEndpointRoundTrip));
	out.kbps = static_cast<int>(Field(endpoint, Ggpo::kEndpointKbpsSent));
	out.localBehind = static_cast<int>(Field(endpoint, Ggpo::kEndpointLocalBehind));
	out.remoteBehind = static_cast<int>(Field(endpoint, Ggpo::kEndpointRemoteBehind));
	return true;
}

}

bool GgpoLayout::Resolve()
{
	if (g_tried)
		return g_backendVTable != 0;

	g_tried = true;

	uint8_t* const name = Only(ImageScanner::FindBytes(ImageScanner::InitialisedData(),
		reinterpret_cast<const uint8_t*>(Ggpo::kBackendTypeName),
		std::strlen(Ggpo::kBackendTypeName) + 1));

	if (name == nullptr)
	{
		strncpy_s(g_status, "the rollback backend type is not named in this game version", _TRUNCATE);
		return false;
	}

	uint8_t* const locator = LocatorOf(name - Ggpo::kTypeDescriptorFromName);

	if (locator == nullptr)
	{
		strncpy_s(g_status, "the rollback backend has no single class locator", _TRUNCATE);
		return false;
	}

	uint8_t* const site = Only(ReferencesIn(ImageScanner::ReadOnlyData(), locator));

	if (site == nullptr)
	{
		strncpy_s(g_status, "the rollback backend has no single table", _TRUNCATE);
		return false;
	}

	g_backendVTable = reinterpret_cast<uintptr_t>(site + Ggpo::kVTableFromLocator);

	Anchors::Record("GGPO player backend table", g_backendVTable, "online link");
	sprintf_s(g_status, "player backend table at %s", DescribeAddress(g_backendVTable).c_str());
	return true;
}

bool GgpoLayout::IsResolved()
{
	return g_backendVTable != 0;
}

bool GgpoLayout::IsPlayerBackend(uintptr_t backend)
{
	if (g_backendVTable == 0 || backend == 0)
		return false;

	uint32_t table = 0;
	return TryRead(backend, table) && table == g_backendVTable;
}

int GgpoLayout::PlayerCount(uintptr_t backend)
{
	const uintptr_t count = Field(backend, Ggpo::kPlayerCount);

	return count >= 1 && count <= Ggpo::kMostPlayers ? static_cast<int>(count) : 0;
}

int GgpoLayout::SpectatorCount(uintptr_t backend)
{
	const uintptr_t count = Field(backend, Ggpo::kSpectatorCount);

	return count <= Ggpo::kMostSpectators ? static_cast<int>(count) : 0;
}

bool GgpoLayout::IsSynchronizing(uintptr_t backend)
{
	uint8_t flag = 0;
	return TryRead(backend + Ggpo::kSynchronizing, flag) && flag != 0;
}

bool GgpoLayout::ReadPeer(uintptr_t backend, uint64_t wanted, Peer& out)
{
	if (!IsPlayerBackend(backend))
		return false;

	const int players = PlayerCount(backend);
	const uintptr_t endpoints = Field(backend, Ggpo::kPlayerEndpoints);

	if (players == 0 || endpoints == 0)
		return false;

	for (int i = 0; i < players; ++i)
	{
		if (ReadEndpoint(endpoints + i * Ggpo::kEndpointBytes, wanted, out))
			return true;
	}

	const int spectators = SpectatorCount(backend);

	for (int i = 0; i < spectators; ++i)
	{
		if (ReadEndpoint(backend + Ggpo::kSpectatorEndpoints + i * Ggpo::kEndpointBytes, wanted, out))
			return true;
	}

	return false;
}

const char* GgpoLayout::StatusText()
{
	return g_status;
}
