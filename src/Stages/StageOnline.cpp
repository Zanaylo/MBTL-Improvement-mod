#include "Stages/StageOnline.h"

#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Core/utils.h"
#include "Game/GameOffsets.h"
#include "Network/ModHandshake.h"
#include "Network/ModPresence.h"
#include "Network/NetLink.h"
#include "Network/NetLog.h"
#include "Stages/GameStages.h"
#include "Stages/StageLibrary.h"
#include "Stages/StageRevision.h"
#include "Stages/StageTable.h"
#include "Training/GameState.h"

#include <algorithm>
#include <cstdio>
#include <vector>

namespace {

namespace Stages = GameOffsets::Stages;

constexpr int kEveryFrames = 30;
constexpr int32_t kDisabled = 1;
constexpr uint32_t kHashSeed = 2166136261u;
constexpr uint32_t kHashPrime = 16777619u;

struct Added
{
	int number;
	bool captured;
	int32_t select;
	int32_t random;
	int32_t vs;
};

std::vector<Added> g_added;
uint32_t g_revision = 0;
bool g_listed = false;

bool g_held = false;
int g_frame = 0;

char g_id[ModHandshake::kStageIdBytes] = "0";
char g_status[192] = "offline, every stage is offered";

void Describe(const std::vector<int>& numbers)
{
	if (numbers.empty())
	{
		strncpy_s(g_id, "0", _TRUNCATE);
		return;
	}

	uint32_t hash = kHashSeed;

	for (int number : numbers)
	{
		hash = (hash ^ static_cast<uint32_t>(number)) * kHashPrime;
	}

	sprintf_s(g_id, "%u/%08x", static_cast<unsigned>(numbers.size()), hash);
}

void Relist()
{
	std::vector<StageLibrary::Entry> entries;
	StageLibrary::Snapshot(entries);

	std::vector<int> numbers;

	for (const StageLibrary::Entry& entry : entries)
	{
		if (entry.removed || GameStages::Owns(entry.number))
			continue;

		numbers.push_back(entry.number);
	}

	std::sort(numbers.begin(), numbers.end());

	std::vector<Added> added;
	added.reserve(numbers.size());

	for (int number : numbers)
		added.push_back({ number, false, 0, 0, 0 });

	g_added.swap(added);
	g_listed = true;

	Describe(numbers);
	ModPresence::SetStages(g_id);
	ModHandshake::SetLocalStages(g_id);

	NetLog::Write("stages: %u number(s) this side added, id %s", static_cast<unsigned>(g_added.size()), g_id);
}

bool Capture(Added& added, uintptr_t record)
{
	if (added.captured)
		return true;

	if (!TryRead(record + Stages::kRecordSelectDisable, added.select) ||
		!TryRead(record + Stages::kRecordRandomDisable, added.random) ||
		!TryRead(record + Stages::kRecordVsDisable, added.vs))
	{
		return false;
	}

	added.captured = true;
	return true;
}

void Write(Added& added, bool hold)
{
	const uintptr_t record = StageTable::RecordAt(added.number);

	if (record == 0 || !Capture(added, record))
		return;

	TryWrite(record + Stages::kRecordSelectDisable, hold ? kDisabled : added.select);
	TryWrite(record + Stages::kRecordRandomDisable, hold ? kDisabled : added.random);
	TryWrite(record + Stages::kRecordVsDisable, hold ? kDisabled : added.vs);
}

bool InOnlineContext()
{
	return NetLink::Lobby() != 0 || NetLink::InSession() || (GameState::IsOnlineKnown() && GameState::IsOnline());
}

bool Agreed()
{
	if (ModHandshake::PeerSharesStages())
		return true;

	return ModPresence::RoomAgrees(g_id);
}

void Report(bool held, bool online, bool agreed)
{
	if (!online)
	{
		strncpy_s(g_status, "offline, every stage is offered", _TRUNCATE);
		return;
	}

	if (g_added.empty())
	{
		strncpy_s(g_status, "online, no added stage numbers to hold back", _TRUNCATE);
		return;
	}

	if (!held)
	{
		sprintf_s(g_status, "online, %s, the %u added stage(s) stay offered",
			agreed ? "the other side has the same stages" : "holding back is off",
			static_cast<unsigned>(g_added.size()));
		return;
	}

	sprintf_s(g_status, "online, the %u added stage(s) are held back so both sides pick from the same list",
		static_cast<unsigned>(g_added.size()));
}

void Apply(bool hold)
{
	for (Added& added : g_added)
		Write(added, hold);
}

}

void StageOnline::OnFrame()
{
	if (++g_frame % kEveryFrames != 0)
		return;

	if (!GameStages::Learned())
		return;

	const uint32_t revision = StageRevision::Current();

	if (!g_listed || revision != g_revision)
	{
		g_revision = revision;
		Relist();
	}

	const bool online = InOnlineContext();
	const bool agreed = Agreed();
	const bool hold = g_settings.stageHoldBack && online && !agreed && !g_added.empty();

	if (hold != g_held)
	{
		g_held = hold;
		LOG("StageOnline: the added stage numbers are %s", hold ? "held back for netplay" : "offered again");
		NetLog::Write("stages: added numbers %s", hold ? "held back" : "offered again");
	}

	Apply(hold);
	Report(hold, online, agreed);
}

bool StageOnline::IsHeldBack()
{
	return g_held;
}

int StageOnline::AddedCount()
{
	return static_cast<int>(g_added.size());
}

const char* StageOnline::StageId()
{
	return g_id;
}

const char* StageOnline::StatusText()
{
	return g_status;
}
