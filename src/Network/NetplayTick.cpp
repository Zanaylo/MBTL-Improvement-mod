#include "Network/NetplayTick.h"

#include "Core/info.h"
#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Network/GgpoLayout.h"
#include "Network/ModChannel.h"
#include "Network/ModHandshake.h"
#include "Network/ModPresence.h"
#include "Network/NetGate.h"
#include "Network/NetLink.h"
#include "Network/NetLog.h"
#include "Network/NetWorker.h"
#include "Network/SteamInterfaces.h"
#include "Network/SteamWatch.h"

#include <cstdio>

namespace {

bool g_installed = false;
char g_status[192] = "not started";

bool PeerVerified(uint64_t id)
{
	return ModHandshake::HeardFrom(id) || ModPresence::PeerHasMod(id);
}

void WriteHeader()
{
	NetLog::Write("mod %s, GGPO player start %s, %s", MBTL_IM_VERSION, NetLink::IsHooked() ? "hooked" : "NOT hooked",
		GgpoLayout::StatusText());
	NetLog::Write("settings: SharePalettes %d, ShowOnlinePalettes %d, StageHoldBack %d, NetworkLog %d",
		g_settings.sharePalettes, g_settings.showOnlinePalettes, g_settings.stageHoldBack, g_settings.netLog);
}

}

void NetplayTick::Install()
{
	if (g_installed)
		return;

	NetLog::SetEnabled(g_settings.netLog);
	NetLog::Initialize();

	const bool hooked = NetLink::Install();

	WriteHeader();

	NetGate::SetPeerVerifier(&PeerVerified);
	ModHandshake::Initialize();
	NetWorker::Start();

	g_installed = true;
	sprintf_s(g_status, "peer discovery %s, rollback layout %s", hooked ? "hooked" : "unavailable",
		GgpoLayout::IsResolved() ? "read" : "not read");
	LOG("NetplayTick: %s", g_status);
}

void NetplayTick::Update()
{
	NetLink::OnPresent();
	NetLink::Update();

	if (!g_installed)
		return;

	if (!SteamWatch::IsRegistered() && SteamInterfaces::IsReady())
		SteamWatch::Register();

	ModChannel::Pump();
	ModHandshake::Update();
}

void NetplayTick::Shutdown()
{
	if (!g_installed)
		return;

	NetWorker::Stop();
	NetLog::Shutdown();
}

const char* NetplayTick::StatusText()
{
	return g_status;
}
