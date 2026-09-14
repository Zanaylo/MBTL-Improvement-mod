#include "Stages/StageTable.h"

#include "Core/logger.h"
#include "Game/Anchors.h"
#include "Game/GameOffsets.h"
#include "Hooks/CodePatch.h"
#include "Hooks/ImageScanner.h"

#include <windows.h>

#include <cstdio>
#include <cstring>
#include <set>
#include <vector>

namespace {

namespace Stages = GameOffsets::Stages;

constexpr size_t kPaddedNumbers = 0x10000;
constexpr size_t kPaddedList = 0x1000;
constexpr size_t kPaddedArray = 0x1000;
constexpr size_t kCaveBytes = 0x1000;
constexpr size_t kSlotBytes = sizeof(uint32_t);
constexpr int32_t kLeastFrameDisp = -0x2000;
constexpr int32_t kMostFrameDisp = -0x100;

constexpr uint8_t kModMask = 0xC7;
constexpr uint8_t kModClear = 0x3F;
constexpr uint8_t kSibOperand = 0x04;
constexpr uint8_t kNoBaseScaled = 0x85;
constexpr uint8_t kFramedSib = 0x84;
constexpr uint8_t kFramedDisp = 0x85;
constexpr uint8_t kEbpBase = 0x05;
constexpr uint8_t kScaleFour = 0x80;
constexpr uint8_t kScaleMask = 0xC0;
constexpr uint8_t kBaseMask = 0x07;

constexpr uint8_t kStore = 0x89;
constexpr uint8_t kLoadAddress = 0x8D;
constexpr uint8_t kStoreImmediate = 0xC7;
constexpr uint8_t kCompareGroup = 0x83;
constexpr uint8_t kCompareImmediate32 = 0x81;
constexpr uint8_t kFramedByte = 0x7D;
constexpr uint8_t kFramedLong = 0xBD;
constexpr uint8_t kFramedImmediate = 0x85;
constexpr uint8_t kTwoByte = 0x0F;
constexpr uint8_t kNearJumpBase = 0x80;
constexpr uint8_t kConditionMask = 0x0F;
constexpr uint8_t kJump = 0xE9;
constexpr uint8_t kNop = 0x90;
constexpr size_t kShortJumpLength = 2;
constexpr size_t kNearJumpLength = 6;
constexpr size_t kJumpLength = 5;
constexpr size_t kLongestPatch = 16;

constexpr uint8_t kClearEax[] = { 0x31, 0xC0 };
constexpr uint8_t kSkipRead[] = { 0x7D, 0x04 };
constexpr size_t kFilterValueLength = 4;
constexpr size_t kFilterTestLength = 2;

struct Cell
{
	uint8_t* modrm;
	uint8_t* disp;
};

struct Plan
{
	uintptr_t table = 0;
	uintptr_t list = 0;
	std::vector<uint8_t*> tableSites;
	std::vector<uint8_t*> listSites;
	std::set<uint8_t*> readers;
	std::set<uint8_t*> bounds;
	std::set<uint8_t*> limits;
	std::vector<Cell> builderCells;
	std::vector<Cell> pickerCells;
	std::vector<Cell> exclusionCells;
	uint8_t* filterRead = nullptr;
	uint8_t* randomEntry = nullptr;
};

class CaveWriter
{
public:
	explicit CaveWriter(uint8_t* start)
		: m_cursor(start)
	{
	}

	uint8_t* Position() const { return m_cursor; }

	void Byte(uint8_t value)
	{
		*m_cursor++ = value;
	}

	void Bytes(const uint8_t* bytes, size_t count)
	{
		std::memcpy(m_cursor, bytes, count);
		m_cursor += count;
	}

	void Dword(uint32_t value)
	{
		std::memcpy(m_cursor, &value, sizeof(value));
		m_cursor += sizeof(value);
	}

	void JumpTo(uint8_t opcode, const uint8_t* target)
	{
		Byte(opcode);
		Relative(target);
	}

	void ConditionalJumpTo(uint8_t condition, const uint8_t* target)
	{
		Byte(kTwoByte);
		Byte(static_cast<uint8_t>(kNearJumpBase | condition));
		Relative(target);
	}

private:
	void Relative(const uint8_t* target)
	{
		Dword(static_cast<uint32_t>(target - (m_cursor + sizeof(uint32_t))));
	}

	uint8_t* m_cursor;
};

bool g_lifted = false;
char g_status[224] = "the game's own 100 stage numbers";

uint8_t* AsPointer(uintptr_t address)
{
	return reinterpret_cast<uint8_t*>(address);
}

bool Refuse(const char* reason)
{
	sprintf_s(g_status, "left at the game's own 100: %s", reason);
	LOG("StageTable: %s", g_status);
	return false;
}

uint8_t* Only(const std::vector<uint8_t*>& list)
{
	return list.size() == 1 ? list.front() : nullptr;
}

bool FindTables(const uint8_t* parser, Plan& plan)
{
	const size_t length = ImageScanner::FunctionLength(parser);
	std::vector<uintptr_t> zeroStores;
	std::set<uintptr_t> stores;

	for (size_t i = 0; i + 11 <= length; ++i)
	{
		const uint8_t* const at = parser + i;

		if (at[0] == kStoreImmediate && at[1] == kSibOperand && at[2] == kNoBaseScaled && ImageScanner::ReadDword(at + 7) == 0)
			zeroStores.push_back(ImageScanner::ReadDword(at + 3));

		if (at[0] == kStore && (at[1] & kModMask) == kSibOperand && (at[2] & kModMask) == kNoBaseScaled)
			stores.insert(ImageScanner::ReadDword(at + 3));
	}

	if (zeroStores.size() != 1)
		return false;

	plan.list = zeroStores.front();
	stores.erase(plan.list);

	if (stores.size() != 1)
		return false;

	plan.table = *stores.begin();
	return true;
}

bool IsSignedJump(const uint8_t* at)
{
	return (at[0] >= 0x7C && at[0] <= 0x7F) || (at[0] == kTwoByte && at[1] >= 0x8C && at[1] <= 0x8F);
}

void CollectBounds(uint8_t* function, Plan& plan)
{
	const size_t length = ImageScanner::FunctionLength(function);

	for (size_t i = 0; i + 10 <= length; ++i)
	{
		uint8_t* const at = function + i;

		if (at[0] == kStoreImmediate && at[1] == kFramedImmediate && ImageScanner::ReadDword(at + 6) == Stages::kAtCount)
			plan.limits.insert(at + 6);

		if (at[0] != kCompareGroup || (at[1] != kFramedByte && at[1] != kFramedLong))
			continue;

		const size_t immediate = at[1] == kFramedByte ? 3 : 6;

		if ((at[immediate] == Stages::kBelowCount || at[immediate] == Stages::kAtCount) && IsSignedJump(at + immediate + 1))
			plan.bounds.insert(at);
	}
}

int32_t FramedArray(const uint8_t* function)
{
	const size_t length = ImageScanner::FunctionLength(function);

	for (size_t i = 0; i + 7 <= length; ++i)
	{
		const uint8_t* const at = function + i;

		if (at[0] != kStore || (at[1] & kModMask) != kFramedSib)
			continue;

		if ((at[2] & kBaseMask) != kEbpBase || (at[2] & kScaleMask) != kScaleFour)
			continue;

		const int32_t disp = static_cast<int32_t>(ImageScanner::ReadDword(at + 3));

		if (disp >= kLeastFrameDisp && disp <= kMostFrameDisp)
			return disp;
	}

	return 0;
}

std::vector<Cell> CellsOf(uint8_t* function, int32_t disp)
{
	std::vector<Cell> cells;
	const size_t length = ImageScanner::FunctionLength(function);

	for (size_t i = 2; i + 4 <= length; ++i)
	{
		uint8_t* const at = function + i;

		if (static_cast<int32_t>(ImageScanner::ReadDword(at)) != disp)
			continue;

		if ((at[-2] & kModMask) == kFramedSib && (at[-1] & kBaseMask) == kEbpBase)
		{
			cells.push_back({ at - 2, at });
			continue;
		}

		if ((at[-1] & kModMask) == kFramedDisp)
			cells.push_back({ at - 1, at });
	}

	return cells;
}

std::vector<Cell> ExclusionCellsOf(uint8_t* function)
{
	std::vector<Cell> cells;
	const size_t length = ImageScanner::FunctionLength(function);

	for (size_t i = 0; i + 6 <= length; ++i)
	{
		uint8_t* const at = function + i;

		if ((at[0] != kStoreImmediate && at[0] != kLoadAddress) || (at[1] & kModMask) != kFramedDisp)
			continue;

		const int32_t disp = static_cast<int32_t>(ImageScanner::ReadDword(at + 2));

		if (disp == Stages::kExclusionBase || disp == Stages::kExclusionBase + static_cast<int32_t>(kSlotBytes))
			cells.push_back({ at + 1, at + 2 });
	}

	return cells;
}

uint8_t* TinyReferencing(uintptr_t address)
{
	std::vector<uint8_t*> tiny;

	for (uint8_t* function : ImageScanner::FunctionsReferencing({ AsPointer(address) }))
	{
		const size_t length = ImageScanner::FunctionLength(function);

		if (length > 0 && length < Stages::kTinyFunction)
			tiny.push_back(function);
	}

	return Only(tiny);
}

uint8_t* FindFilterRead(const std::set<uint8_t*>& functions)
{
	std::vector<uint8_t*> sites;

	for (uint8_t* function : functions)
	{
		const size_t length = ImageScanner::FunctionLength(function);

		for (size_t i = 0; i + Stages::kFilterReadLength <= length; ++i)
		{
			if (std::memcmp(function + i, Stages::kFilterRead, sizeof(Stages::kFilterRead)) == 0)
				sites.push_back(function + i);
		}
	}

	return Only(sites);
}

bool FindBounds(Plan& plan)
{
	const uint8_t* const getRecord = TinyReferencing(plan.table);
	const uint8_t* const random = Only(ImageScanner::FunctionsReferencing(ImageScanner::FindString(Stages::kRandomFilterAnchor)));
	plan.randomEntry = random ? Only(ImageScanner::CallerFunctions(random)) : nullptr;

	if (!getRecord || !plan.randomEntry)
		return false;

	for (uint8_t* function : ImageScanner::FunctionsReferencing({ AsPointer(plan.table) }))
		plan.readers.insert(function);

	for (uint8_t* function : ImageScanner::CallerFunctions(getRecord))
		plan.readers.insert(function);

	for (uint8_t* function : ImageScanner::CallTargets(plan.randomEntry))
		plan.readers.insert(function);

	for (uint8_t* function : plan.readers)
		CollectBounds(function, plan);

	plan.exclusionCells = ExclusionCellsOf(plan.randomEntry);
	plan.filterRead = FindFilterRead(plan.readers);
	return true;
}

uint8_t* RandomPicker(const uint8_t* entry)
{
	const uint8_t* const random = Only(ImageScanner::FunctionsReferencing(ImageScanner::FindString(Stages::kRandomFilterAnchor)));
	const std::vector<uint8_t*> calls = ImageScanner::CallSequence(entry);

	for (size_t i = 0; i + 1 < calls.size(); ++i)
	{
		if (calls[i] == random)
			return calls[i + 1];
	}

	return nullptr;
}

bool FindArrays(Plan& plan)
{
	const uint8_t* const getEntry = TinyReferencing(plan.list);

	if (!getEntry)
		return false;

	std::vector<uint8_t*> builders;

	for (uint8_t* caller : ImageScanner::CallerFunctions(getEntry))
	{
		if (FramedArray(caller) != 0)
			builders.push_back(caller);
	}

	uint8_t* const builder = Only(builders);
	uint8_t* const picker = RandomPicker(plan.randomEntry);

	if (!builder || !picker || FramedArray(picker) == 0)
		return false;

	plan.builderCells = CellsOf(builder, FramedArray(builder));
	plan.pickerCells = CellsOf(picker, FramedArray(picker));
	return true;
}

bool InBand(size_t value, int least, int most)
{
	return static_cast<int>(value) >= least && static_cast<int>(value) <= most;
}

bool Validate(const Plan& plan, char* reason, size_t size)
{
	sprintf_s(reason, size, "%u table and %u list reference(s), %u bound(s), %u limit(s), %u and %u array cell(s), "
		"%u exclusion cell(s), filter read %s", static_cast<unsigned>(plan.tableSites.size()),
		static_cast<unsigned>(plan.listSites.size()), static_cast<unsigned>(plan.bounds.size()),
		static_cast<unsigned>(plan.limits.size()), static_cast<unsigned>(plan.builderCells.size()),
		static_cast<unsigned>(plan.pickerCells.size()), static_cast<unsigned>(plan.exclusionCells.size()),
		plan.filterRead ? "found" : "missing");

	return static_cast<int>(plan.tableSites.size()) == Stages::kTableReferences &&
		static_cast<int>(plan.listSites.size()) == Stages::kListReferences &&
		static_cast<int>(plan.bounds.size()) == Stages::kBoundSites &&
		static_cast<int>(plan.limits.size()) == Stages::kLimitSites &&
		InBand(plan.builderCells.size(), Stages::kLeastArrayCells, Stages::kMostArrayCells) &&
		InBand(plan.pickerCells.size(), Stages::kLeastArrayCells, Stages::kMostArrayCells) &&
		static_cast<int>(plan.exclusionCells.size()) == Stages::kExclusionCells && plan.filterRead != nullptr;
}

uint8_t* Allocate(size_t bytes, DWORD protection)
{
	return static_cast<uint8_t*>(VirtualAlloc(nullptr, bytes, MEM_COMMIT | MEM_RESERVE, protection));
}

bool JumpInto(uint8_t* site, size_t length, const uint8_t* cave)
{
	uint8_t bytes[kLongestPatch] = {};

	if (length < kJumpLength || length > kLongestPatch)
		return false;

	const uint32_t relative = static_cast<uint32_t>(cave - (site + kJumpLength));

	std::memset(bytes, kNop, sizeof(bytes));
	bytes[0] = kJump;
	std::memcpy(bytes + 1, &relative, sizeof(relative));

	return CodePatch::Write(site, bytes, length);
}

int Repoint(const std::vector<uint8_t*>& sites, const uint8_t* destination)
{
	const uint32_t value = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(destination));
	int done = 0;

	for (uint8_t* site : sites)
		done += CodePatch::Write(site, &value, sizeof(value)) ? 1 : 0;

	return done;
}

int Restack(const std::vector<Cell>& cells, const uint8_t* buffer, int32_t base)
{
	int done = 0;

	for (const Cell& cell : cells)
	{
		const int32_t offset = base == 0 ? 0 : static_cast<int32_t>(ImageScanner::ReadDword(cell.disp)) - base;
		const uint32_t value = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(buffer) + offset);
		const uint8_t modrm = static_cast<uint8_t>(cell.modrm[0] & kModClear);

		done += CodePatch::Write(cell.modrm, &modrm, sizeof(modrm)) && CodePatch::Write(cell.disp, &value, sizeof(value)) ? 1 : 0;
	}

	return done;
}

bool LiftBound(uint8_t* compare, CaveWriter& writer)
{
	const size_t dispLength = compare[1] == kFramedLong ? sizeof(uint32_t) : 1;
	const size_t compareLength = 2 + dispLength + 1;
	const uint8_t* const jump = compare + compareLength;
	const bool nearJump = jump[0] == kTwoByte;
	const size_t jumpLength = nearJump ? kNearJumpLength : kShortJumpLength;
	const uint8_t condition = static_cast<uint8_t>((nearJump ? jump[1] : jump[0]) & kConditionMask);
	const int32_t displacement = nearJump ? static_cast<int32_t>(ImageScanner::ReadDword(jump + 2))
		: static_cast<int8_t>(jump[1]);
	const uint8_t* const resume = jump + jumpLength;
	const uint32_t value = compare[compareLength - 1] == Stages::kAtCount ? StageTable::kWideNumbers
		: StageTable::kWideNumbers - 1;
	const uint8_t* const cave = writer.Position();

	writer.Byte(kCompareImmediate32);
	writer.Bytes(compare + 1, 1 + dispLength);
	writer.Dword(value);
	writer.ConditionalJumpTo(condition, resume + displacement);
	writer.JumpTo(kJump, resume);

	return JumpInto(compare, compareLength + jumpLength, cave);
}

bool ClampFilter(uint8_t* site, CaveWriter& writer)
{
	const uint8_t* const resume = site + Stages::kFilterReadLength;
	const uint8_t* const excluded = resume + static_cast<int8_t>(site[Stages::kFilterReadLength - 1]);
	const uint8_t* const cave = writer.Position();

	writer.Bytes(kClearEax, sizeof(kClearEax));
	writer.Bytes(Stages::kFilterIndexCompare, sizeof(Stages::kFilterIndexCompare));
	writer.Dword(Stages::kFilterEntries);
	writer.Bytes(kSkipRead, sizeof(kSkipRead));
	writer.Bytes(site, kFilterValueLength);
	writer.Bytes(site + kFilterValueLength, kFilterTestLength);
	writer.ConditionalJumpTo(site[Stages::kFilterReadLength - 2] & kConditionMask, excluded);
	writer.JumpTo(kJump, resume);

	return JumpInto(site, Stages::kFilterReadLength, cave);
}

int LiftBounds(const Plan& plan, CaveWriter& writer)
{
	int done = 0;

	for (uint8_t* bound : plan.bounds)
		done += LiftBound(bound, writer) ? 1 : 0;

	const uint32_t limit = StageTable::kWideNumbers;

	for (uint8_t* site : plan.limits)
		done += CodePatch::Write(site, &limit, sizeof(limit)) ? 1 : 0;

	return done + (ClampFilter(plan.filterRead, writer) ? 1 : 0);
}

uint8_t* WidenedTable(uintptr_t stock, const uint8_t* empty)
{
	uint8_t* const table = Allocate(kPaddedNumbers * kSlotBytes, PAGE_READWRITE);

	if (!table)
		return nullptr;

	std::memcpy(table, AsPointer(stock), StageTable::kStockNumbers * kSlotBytes);

	const uint32_t beyond = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(empty));
	const auto slots = reinterpret_cast<uint32_t*>(table);

	for (size_t i = StageTable::kWideNumbers; i < kPaddedNumbers; ++i)
		slots[i] = beyond;

	return table;
}

bool Apply(const Plan& plan)
{
	const uint8_t* const empty = Allocate(Stages::kRecordBytes, PAGE_READWRITE);
	const uint8_t* const table = empty ? WidenedTable(plan.table, empty) : nullptr;
	uint8_t* const list = Allocate(kPaddedList * kSlotBytes, PAGE_READWRITE);
	const uint8_t* const builderArray = Allocate(kPaddedArray * kSlotBytes, PAGE_READWRITE);
	const uint8_t* const pickerArray = Allocate(kPaddedArray * kSlotBytes, PAGE_READWRITE);
	const uint8_t* const exclusionArray = Allocate(kPaddedArray * kSlotBytes, PAGE_READWRITE);
	uint8_t* const caves = Allocate(kCaveBytes, PAGE_EXECUTE_READWRITE);

	if (!table || !list || !builderArray || !pickerArray || !exclusionArray || !caves)
		return Refuse("the wider tables could not be allocated");

	std::memcpy(list, AsPointer(plan.list), StageTable::kStockNumbers * kSlotBytes);

	const size_t expected = plan.tableSites.size() + plan.listSites.size() + plan.bounds.size() + plan.limits.size() + 1 +
		plan.builderCells.size() + plan.pickerCells.size() + plan.exclusionCells.size();

	CaveWriter writer(caves);

	const int done = Repoint(plan.tableSites, table) + Repoint(plan.listSites, list) + LiftBounds(plan, writer) +
		Restack(plan.builderCells, builderArray, 0) + Restack(plan.pickerCells, pickerArray, 0) +
		Restack(plan.exclusionCells, exclusionArray, Stages::kExclusionBase);

	if (static_cast<size_t>(done) != expected)
	{
		sprintf_s(g_status, "only %d of %u patch(es) took, so the stage table is in a mixed state", done,
			static_cast<unsigned>(expected));
		LOG("StageTable: %s", g_status);
		return false;
	}

	Anchors::Record("Stage table (widened)", reinterpret_cast<uintptr_t>(table));
	Anchors::Record("Stage list (widened)", reinterpret_cast<uintptr_t>(list));
	Anchors::Record("Stage bound caves", reinterpret_cast<uintptr_t>(caves));
	return true;
}

}

bool StageTable::Initialize()
{
	if (!ImageScanner::Initialize())
		return Refuse("MBTL.exe could not be read");

	const uint8_t* const parser = Only(ImageScanner::FunctionsReferencing(ImageScanner::FindString(Stages::kListAnchor)));

	Plan plan;

	if (!parser || !FindTables(parser, plan))
		return Refuse("the BgList parser was not recognised");

	Anchors::Record("Stage table", plan.table);
	Anchors::Record("Stage select list", plan.list);

	plan.tableSites = ImageScanner::FindReferencesTo(AsPointer(plan.table));
	plan.listSites = ImageScanner::FindReferencesTo(AsPointer(plan.list));

	if (!FindBounds(plan) || !FindArrays(plan))
		return Refuse("the stage accessors were not recognised");

	char reason[224] = {};
	const bool valid = Validate(plan, reason, sizeof(reason));
	LOG("StageTable: %s", reason);

	if (!valid)
		return Refuse("this build reads the stage table in ways the mod does not recognise");

	if (!Apply(plan))
		return false;

	g_lifted = true;
	sprintf_s(g_status, "extension table on: %d stage numbers and %d picker entries", kWideNumbers, kWideNumbers);
	LOG("StageTable: %s", g_status);
	return true;
}

bool StageTable::Lifted()
{
	return g_lifted;
}

int StageTable::Numbers()
{
	return g_lifted ? kWideNumbers : kStockNumbers;
}

int StageTable::ListEntries()
{
	return g_lifted ? kWideNumbers : kStockNumbers;
}

const char* StageTable::StatusText()
{
	return g_status;
}
