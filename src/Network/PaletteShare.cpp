#include "Network/PaletteShare.h"

#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Network/ModChannel.h"
#include "Network/ModHandshake.h"
#include "Network/NetLink.h"
#include "Palette/EffectPaint.h"
#include "Palette/PaletteChoice.h"
#include "Palette/PaletteControl.h"
#include "Palette/PaletteFile.h"
#include "Palette/PaletteOwner.h"
#include "Palette/PalettePaint.h"

#include <windows.h>

#include <cstdio>
#include <cstring>
#include <memory>

namespace {

constexpr uint16_t kVersion = 3;
constexpr DWORD kTtlMs = 20000;

constexpr int kSendDelayFrames = 60;
constexpr int kSettleFrames = 30;

#pragma pack(push, 1)
struct Packet
{
	ModChannel::Header header;
	int32_t chara;
	int8_t seat;
	char name[PaletteFile::kNameLength];
	uint8_t subMask;
	uint8_t pages[PaletteFile::kSubPalettes][PaletteFile::kBytes];
	uint8_t hasEffect;
	uint8_t effectColors[PaletteFile::kBytes];
};
#pragma pack(pop)

struct Remote
{
	bool valid;
	int chara;
	char name[PaletteFile::kNameLength];
};

bool g_inMatch = false;
int g_frames = 0;
bool g_queued = false;

unsigned g_seenRevision = 0;
int g_settled = 0;
unsigned g_sentRevision = 0;

int g_sentCount = 0;
int g_receivedCount = 0;
char g_status[192] = "Nothing sent or received yet.";

Remote g_remote[PaletteOwner::kSeats] = {};

int OwnSide()
{
	return PaletteControl::IsSpectating() ? -1 : PaletteControl::LocalPlayer();
}

unsigned OwnRevision(int side)
{
	unsigned revision = 0;

	for (int member = 0; side >= 0 && member < PaletteOwner::kMembers; ++member)
	{
		const int seat = PaletteOwner::SeatOf(side, member);
		revision = revision * 131u + PalettePaint::GetRevision(seat) * 31u + EffectPaint::GetRevision(seat);
	}

	return revision;
}

void WornName(int seat, char* out, size_t size)
{
	strncpy_s(out, size, PaletteChoice::WornFile(seat), _TRUNCATE);

	const size_t length = strlen(out);
	const size_t suffix = strlen(PaletteFile::kExtension);

	if (length > suffix && _stricmp(out + length - suffix, PaletteFile::kExtension) == 0)
		out[length - suffix] = '\0';
}

const char* PeerText()
{
	switch (ModHandshake::GetPeerState())
	{
	case ModHandshake::Peer_Waiting:
		return ", waiting for the opponent's mod";
	case ModHandshake::Peer_Modded:
		return "";
	case ModHandshake::Peer_Unmodded:
		return ", the opponent has no mod";
	default:
		return ", no opponent yet";
	}
}

void UpdateStatus()
{
	const int own = OwnSide();
	char seat[32] = {};

	if (own < 0)
		strncpy_s(seat, "your side is unknown", _TRUNCATE);
	else
		sprintf_s(seat, "you are P%d", own + 1);

	sprintf_s(g_status, "%s, sent %d, received %d%s", seat, g_sentCount, g_receivedCount, PeerText());
}

void FillPacket(int seat, Packet& packet)
{
	packet.header.magic = ModChannel::kMagic;
	packet.header.version = kVersion;
	packet.header.kind = ModChannel::kKindPalette;
	packet.chara = PaletteOwner::CharaNumber(seat);
	packet.seat = static_cast<int8_t>(seat);

	for (int sub = 0; sub < PaletteFile::kSubPalettes; ++sub)
	{
		const uint8_t* const page = PalettePaint::GetStaged(seat, sub);

		if (page == nullptr)
			continue;

		std::memcpy(packet.pages[sub], page, PaletteFile::kBytes);
		packet.subMask |= static_cast<uint8_t>(1u << sub);
	}

	WornName(seat, packet.name, sizeof(packet.name));

	if (EffectPaint::GetEditedCount(seat) == 0)
		return;

	EffectPaint::GetBlock(seat, packet.effectColors);
	packet.hasEffect = 1;
}

bool SendSeat(int seat)
{
	if (PaletteOwner::CharaNumber(seat) < 0)
		return false;

	const std::unique_ptr<Packet> packet = std::make_unique<Packet>();
	std::memset(packet.get(), 0, sizeof(Packet));
	FillPacket(seat, *packet);

	char label[24] = {};
	sprintf_s(label, "palette seat %d", seat);

	if (!ModChannel::SendToPeer(packet.get(), sizeof(Packet), kTtlMs, label))
		return false;

	LOG("PaletteShare: queued '%s' for character %d on p%d", packet->name, packet->chara,
		PaletteOwner::SideOf(seat) + 1);
	return true;
}

void SendOurs()
{
	const int own = OwnSide();

	if (!g_settings.sharePalettes || own < 0)
		return;

	bool queued = false;

	for (int member = 0; member < PaletteOwner::kMembers; ++member)
		queued = SendSeat(PaletteOwner::SeatOf(own, member)) || queued;

	if (!queued)
		return;

	g_sentRevision = OwnRevision(own);
	g_queued = true;
	++g_sentCount;
}

void PlaceForeign(int seat, const Packet& packet)
{
	PalettePaint::ClearRemote(seat);
	EffectPaint::ClearRemote(seat);

	for (int sub = 0; sub < PaletteFile::kSubPalettes; ++sub)
	{
		if ((packet.subMask & (1u << sub)) != 0)
			PalettePaint::StageRemote(seat, sub, packet.pages[sub]);
	}

	if (packet.hasEffect != 0)
		EffectPaint::SetRemote(seat, packet.effectColors);

	Remote& remote = g_remote[seat];
	remote.valid = packet.subMask != 0;
	remote.chara = packet.chara;
	strncpy_s(remote.name, packet.name, _TRUNCATE);

	++g_receivedCount;
}

int SeatForPeer(int claimed)
{
	if (claimed < 0 || claimed >= PaletteOwner::kSeats)
		return -1;

	const int own = OwnSide();
	const int side = own == 0 || own == 1 ? 1 - own : PaletteOwner::SideOf(claimed);

	return PaletteOwner::SeatOf(side, PaletteOwner::MemberOf(claimed));
}

void HandlePalette(const uint8_t* data, int size, uint64_t from)
{
	if (size != static_cast<int>(sizeof(Packet)) || !g_settings.showOnlinePalettes)
		return;

	if (from == 0 || from != NetLink::Peer() || !ModHandshake::HeardFrom(from))
		return;

	Packet packet = {};
	std::memcpy(&packet, data, sizeof(packet));

	if (packet.header.version != kVersion)
		return;

	const int seat = SeatForPeer(packet.seat);

	if (seat < 0 || PaletteOwner::SideOf(seat) == OwnSide())
		return;

	packet.name[PaletteFile::kNameLength - 1] = '\0';
	PlaceForeign(seat, packet);
}

void DropStaleForeign()
{
	for (int seat = 0; seat < PaletteOwner::kSeats; ++seat)
	{
		if (!g_remote[seat].valid)
			continue;

		const int chara = PaletteOwner::CharaNumber(seat);

		if (chara < 0 || chara == g_remote[seat].chara)
			continue;

		PalettePaint::ClearRemote(seat);
		EffectPaint::ClearRemote(seat);
		g_remote[seat] = {};
	}
}

void ForgetForeign()
{
	for (int seat = 0; seat < PaletteOwner::kSeats; ++seat)
	{
		PalettePaint::ClearRemote(seat);
		EffectPaint::ClearRemote(seat);
		g_remote[seat] = {};
	}
}

void ResetMatch(bool inMatch)
{
	g_inMatch = inMatch;
	g_frames = 0;
	g_queued = false;
	g_sentRevision = 0;
	g_settled = 0;

	if (!inMatch)
		ForgetForeign();
}

bool Due(int own)
{
	const unsigned revision = OwnRevision(own);

	if (revision != g_seenRevision)
	{
		g_seenRevision = revision;
		g_settled = 0;
		return false;
	}

	if (g_settled < kSettleFrames)
		++g_settled;

	if (g_settled < kSettleFrames || g_frames < kSendDelayFrames)
		return false;

	return !g_queued || revision != g_sentRevision;
}

}

void PaletteShare::Initialize()
{
	ModChannel::Register(ModChannel::kKindPalette, &HandlePalette);
}

void PaletteShare::OnFrame()
{
	const bool inMatch = PaletteControl::IsOnline() && PaletteOwner::CharaNumber(0) >= 0;

	if (inMatch != g_inMatch)
		ResetMatch(inMatch);

	if (!inMatch)
		return;

	DropStaleForeign();
	++g_frames;

	const int own = OwnSide();

	if (own >= 0 && NetLink::HasPeer() && Due(own))
		SendOurs();

	UpdateStatus();
}

const char* PaletteShare::GetStatusText()
{
	return g_status;
}

const char* PaletteShare::GetRemoteName(int seat)
{
	if (seat < 0 || seat >= PaletteOwner::kSeats || !g_remote[seat].valid)
		return "";

	return g_remote[seat].name;
}
