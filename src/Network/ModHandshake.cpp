#include "Network/ModHandshake.h"

#include "Core/info.h"
#include "Network/ModChannel.h"
#include "Network/ModPresence.h"
#include "Network/NetLink.h"
#include "Network/NetLog.h"

#include <windows.h>

#include <cstdio>
#include <cstring>

namespace {

constexpr uint16_t kHelloVersion = 1;
constexpr uint8_t kFlagAnswerMe = 1;

constexpr DWORD kHelloTtlMs = 20000;
constexpr DWORD kAnswerWaitMs = 30000;

constexpr int kVersionBytes = ModHandshake::kVersionBytes;
constexpr int kStageBytes = ModHandshake::kStageIdBytes;

#pragma pack(push, 1)
struct Hello
{
	ModChannel::Header header;
	uint8_t flags;
	char modVersion[kVersionBytes];
	char stages[kStageBytes];
};
#pragma pack(pop)

SRWLOCK g_lock = SRWLOCK_INIT;
char g_localStages[kStageBytes] = "";

uint64_t g_peer = 0;
volatile LONG64 g_heardFrom = 0;
DWORD g_trackedAt = 0;
bool g_asked = false;
bool g_sent = false;
char g_sentStages[kStageBytes] = "";

ModHandshake::PeerState g_state = ModHandshake::Peer_None;
char g_peerVersion[kVersionBytes] = "";
char g_peerStages[kStageBytes] = "";

char g_status[192] = "no session";

void LocalStages(char* out, int size)
{
	AcquireSRWLockShared(&g_lock);
	strncpy_s(out, size, g_localStages, _TRUNCATE);
	ReleaseSRWLockShared(&g_lock);
}

void UpdateStatus()
{
	switch (g_state)
	{
	case ModHandshake::Peer_Waiting:
		strncpy_s(g_status, "connected, waiting for the other side's hello", _TRUNCATE);
		return;
	case ModHandshake::Peer_Modded:
		sprintf_s(g_status, "the other side runs %s, stages %s", g_peerVersion,
			ModHandshake::PeerSharesStages() ? "match yours" : "differ from yours");
		return;
	case ModHandshake::Peer_Unmodded:
		strncpy_s(g_status, "the other side shows no sign of the mod, so nothing is sent to it", _TRUNCATE);
		return;
	default:
		strncpy_s(g_status, "no session", _TRUNCATE);
		return;
	}
}

bool LocalChanged()
{
	char stages[kStageBytes] = {};
	LocalStages(stages, sizeof(stages));

	return strcmp(stages, g_sentStages) != 0;
}

void QueueHello(uint8_t flags)
{
	Hello hello = {};
	hello.header.magic = ModChannel::kMagic;
	hello.header.version = kHelloVersion;
	hello.header.kind = ModChannel::kKindHello;
	hello.flags = flags;
	strncpy_s(hello.modVersion, MBTL_IM_VERSION, _TRUNCATE);
	LocalStages(hello.stages, sizeof(hello.stages));

	if (!ModChannel::ProbePeer(&hello, sizeof(hello), kHelloTtlMs,
		(flags & kFlagAnswerMe) != 0 ? "hello" : "hello answer"))
	{
		return;
	}

	g_sent = true;
	strncpy_s(g_sentStages, hello.stages, _TRUNCATE);
}

void Track(uint64_t peer)
{
	if (peer == g_peer)
		return;

	g_peer = peer;
	g_trackedAt = GetTickCount();
	g_asked = false;
	g_sent = false;
	g_sentStages[0] = 0;
	g_peerVersion[0] = 0;
	g_peerStages[0] = 0;
	g_state = peer != 0 ? ModHandshake::Peer_Waiting : ModHandshake::Peer_None;

	UpdateStatus();
}

void Wait()
{
	if (!g_asked)
	{
		g_asked = true;
		QueueHello(kFlagAnswerMe);
		NetLog::Write("handshake: hello queued for %llu, %s", static_cast<unsigned long long>(g_peer),
			ModPresence::PeerHasMod(g_peer) ? "the room already marks them as modded" : "the room says nothing about them");
	}

	if (GetTickCount() - g_trackedAt < kAnswerWaitMs)
		return;

	g_state = ModHandshake::Peer_Unmodded;
	UpdateStatus();
	NetLog::Write("handshake: no hello back from %llu, nothing more is sent to them",
		static_cast<unsigned long long>(g_peer));
}

void HandleHello(const uint8_t* data, int size, uint64_t from)
{
	if (size < static_cast<int>(sizeof(Hello)) || from == 0 || from != NetLink::Peer())
		return;

	Track(from);

	Hello hello = {};
	memcpy(&hello, data, sizeof(hello));
	hello.modVersion[kVersionBytes - 1] = 0;
	hello.stages[kStageBytes - 1] = 0;

	InterlockedExchange64(&g_heardFrom, static_cast<LONG64>(from));
	g_state = ModHandshake::Peer_Modded;
	strncpy_s(g_peerVersion, hello.modVersion, _TRUNCATE);
	strncpy_s(g_peerStages, hello.stages, _TRUNCATE);
	UpdateStatus();

	NetLog::Write("handshake: %llu runs %s, stages %s", static_cast<unsigned long long>(from), g_peerVersion,
		g_peerStages);

	if ((hello.flags & kFlagAnswerMe) == 0 || g_sent)
		return;

	QueueHello(0);
}

}

void ModHandshake::Initialize()
{
	ModChannel::Register(ModChannel::kKindHello, &HandleHello);
}

void ModHandshake::Update()
{
	Track(NetLink::HasPeer() ? NetLink::Peer() : 0);

	if (g_state == Peer_Waiting)
	{
		Wait();
		return;
	}

	if (g_state == Peer_Modded && g_sent && LocalChanged())
		QueueHello(0);
}

void ModHandshake::SetLocalStages(const char* id)
{
	AcquireSRWLockExclusive(&g_lock);
	strncpy_s(g_localStages, id != nullptr ? id : "", _TRUNCATE);
	ReleaseSRWLockExclusive(&g_lock);
}

ModHandshake::PeerState ModHandshake::GetPeerState()
{
	return g_state;
}

bool ModHandshake::PeerHasMod()
{
	return g_state == Peer_Modded;
}

bool ModHandshake::HeardFrom(uint64_t id)
{
	return id != 0 && static_cast<uint64_t>(g_heardFrom) == id;
}

const char* ModHandshake::PeerVersion()
{
	return g_peerVersion;
}

const char* ModHandshake::PeerStages()
{
	return g_peerStages;
}

bool ModHandshake::PeerSharesStages()
{
	if (g_state != Peer_Modded)
		return false;

	char stages[kStageBytes] = {};
	LocalStages(stages, sizeof(stages));

	return strcmp(stages, g_peerStages) == 0;
}

const char* ModHandshake::StatusText()
{
	return g_status;
}
