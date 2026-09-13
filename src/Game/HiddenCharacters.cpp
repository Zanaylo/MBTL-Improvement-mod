#include "Game/HiddenCharacters.h"

#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Core/utils.h"
#include "D3D9/DeviceHooks.h"
#include "Game/Anchors.h"
#include "Game/GameOffsets.h"
#include "Hooks/ImageScanner.h"
#include "Training/GameState.h"

#include <cstdio>
#include <cstring>
#include <vector>

namespace {

namespace Roster = GameOffsets::Roster;

struct Saved
{
	int id;
	uint32_t unselect;
};

struct Cell
{
	uintptr_t address;
	int id;
};

uintptr_t g_table = 0;
uintptr_t g_networkFlag = 0;

Saved g_saved[Roster::kMostCharas] = {};
int g_savedCount = 0;

Cell g_cells[Roster::kMostCharas] = {};
int g_cellCount = 0;

char g_status[128] = "nothing unlocked";

uintptr_t GetterAfter(const uint8_t* function, size_t length, size_t from)
{
	for (size_t k = from; k + 5 <= length && k < from + Roster::kGetterWindow; ++k)
	{
		if (function[k] != Roster::kCall)
			continue;

		const int32_t relative = static_cast<int32_t>(ImageScanner::ReadDword(function + k + 1));
		const uintptr_t value = ImageScanner::GetterValue(function + k + 5 + relative);

		if (value != 0)
			return value;
	}

	return 0;
}

uintptr_t ResolveTable()
{
	const std::vector<uint8_t*> strings = ImageScanner::FindString(Roster::kTableAnchor);
	const std::vector<uint8_t*> loaders = ImageScanner::FunctionsReferencing(strings);

	if (loaders.size() != 1)
	{
		LOG("HiddenCharacters: the character table loader has %u candidate(s), expected one",
			static_cast<unsigned>(loaders.size()));
		return 0;
	}

	const uint8_t* const loader = loaders.front();
	const size_t length = ImageScanner::FunctionLength(loader);

	for (uint8_t* text : strings)
	{
		const uint8_t* const after = ImageScanner::AfterPushOf(loader, length, text);

		if (after != nullptr)
			return GetterAfter(loader, length, static_cast<size_t>(after - loader));
	}

	return 0;
}

uintptr_t NetworkObject(const uint8_t* native, size_t length)
{
	for (size_t i = 0; i + 5 <= length; ++i)
	{
		if (native[i] == Roster::kLoadEcx && ImageScanner::InData(ImageScanner::ReadDword(native + i + 1)))
			return ImageScanner::ReadDword(native + i + 1);
	}

	return 0;
}

uintptr_t FlagDisplacement(const uint8_t* function)
{
	const size_t length = ImageScanner::FunctionLength(function);

	for (size_t i = 0; i + sizeof(Roster::kByteLoad) + 4 <= length; ++i)
	{
		if (std::memcmp(function + i, Roster::kByteLoad, sizeof(Roster::kByteLoad)) == 0)
			return ImageScanner::ReadDword(function + i + sizeof(Roster::kByteLoad));
	}

	return 0;
}

uintptr_t ResolveNetworkFlag()
{
	const uint8_t* const native = ImageScanner::NativeFunction(Roster::kNetworkNative);

	if (native == nullptr)
		return 0;

	const uintptr_t object = NetworkObject(native, ImageScanner::FunctionLength(native));

	for (uint8_t* target : ImageScanner::CallTargets(native))
	{
		const size_t length = ImageScanner::FunctionLength(target);

		if (object == 0 || length == 0 || length >= Roster::kShortFunction ||
			!ImageScanner::Contains(target, length, Roster::kAddEcxNetwork, sizeof(Roster::kAddEcxNetwork)))
		{
			continue;
		}

		const std::vector<uint8_t*> inner = ImageScanner::CallTargets(target);
		const uintptr_t displacement = inner.empty() ? 0 : FlagDisplacement(inner.front());

		if (displacement != 0)
			return object + Roster::kNetworkBase + displacement;
	}

	LOG("HiddenCharacters: the network state flag was not found");
	return 0;
}

bool IsOffline()
{
	uint8_t network = 1;

	return g_networkFlag != 0 && GameState::IsOnlineKnown() && !GameState::IsOnline() &&
		TryRead(g_networkFlag, network) && network == 0;
}

bool IsSaved(int id)
{
	for (int i = 0; i < g_savedCount; ++i)
	{
		if (g_saved[i].id == id)
			return true;
	}

	return false;
}

bool ReadRecords(uintptr_t& outBegin, int& outCount)
{
	uint32_t begin = 0;
	uint32_t end = 0;

	if (!TryRead(g_table + Roster::kRecordsBegin, begin) || !TryRead(g_table + Roster::kRecordsEnd, end) ||
		begin == 0 || end <= begin)
	{
		return false;
	}

	const int count = static_cast<int>((end - begin) / Roster::kRecordBytes);

	outBegin = begin;
	outCount = count < Roster::kMostCharas ? count : Roster::kMostCharas;
	return true;
}

int CellsHolding(int id, uintptr_t* outEmpty)
{
	uint32_t columns = 0;
	uint32_t rows = 0;
	uint32_t grid = 0;
	int found = 0;

	*outEmpty = 0;

	if (!TryRead(g_table + Roster::kGridColumns, columns) || !TryRead(g_table + Roster::kGridRows, rows) ||
		!TryRead(g_table + Roster::kGridBegin, grid) || grid == 0 || columns > Roster::kMostGridSide ||
		rows > Roster::kMostGridSide)
	{
		return -1;
	}

	for (uint32_t row = 0; row < rows; ++row)
	{
		uint32_t cells = 0;

		if (!TryRead(grid + row * Roster::kGridRowBytes, cells) || cells == 0)
			continue;

		for (uint32_t column = 0; column < columns; ++column)
		{
			int32_t value = 0;
			const uintptr_t cell = cells + column * sizeof(int32_t);

			if (!TryRead(cell, value))
				continue;

			found += value == id ? 1 : 0;

			if (value == Roster::kEmptyCell && *outEmpty == 0)
				*outEmpty = cell;
		}
	}

	return found;
}

void PlaceOnGrid(int id)
{
	uintptr_t empty = 0;

	if (g_cellCount >= Roster::kMostCharas || CellsHolding(id, &empty) != 0 || empty == 0)
		return;

	if (TryWrite(empty, static_cast<int32_t>(id)))
		g_cells[g_cellCount++] = { empty, id };
}

void Unlock()
{
	uintptr_t begin = 0;
	int count = 0;

	if (!ReadRecords(begin, count))
		return;

	for (int id = 0; id < count && g_savedCount < Roster::kMostCharas; ++id)
	{
		const uintptr_t record = begin + id * Roster::kRecordBytes;
		uint32_t alive = 0;
		uint32_t unselect = 0;

		if (IsSaved(id) || !TryRead(record + Roster::kAlive, alive) || alive == 0 ||
			!TryRead(record + Roster::kUnselect, unselect) || unselect == 0)
		{
			continue;
		}

		if (!TryWrite(record + Roster::kUnselect, static_cast<uint32_t>(0)))
			continue;

		g_saved[g_savedCount++] = { id, unselect };
		LOG("HiddenCharacters: character %d can be picked offline", id);
	}

	for (int i = 0; i < g_savedCount; ++i)
		PlaceOnGrid(g_saved[i].id);

	sprintf_s(g_status, "%d character(s) unlocked", g_savedCount);
}

void Restore()
{
	uintptr_t begin = 0;
	int count = 0;

	for (int i = 0; i < g_cellCount; ++i)
	{
		int32_t value = 0;

		if (TryRead(g_cells[i].address, value) && value == g_cells[i].id)
			TryWrite(g_cells[i].address, Roster::kEmptyCell);
	}

	for (int i = 0; ReadRecords(begin, count) && i < g_savedCount; ++i)
	{
		if (g_saved[i].id < count)
			TryWrite(begin + g_saved[i].id * Roster::kRecordBytes + Roster::kUnselect, g_saved[i].unselect);
	}

	g_cellCount = 0;
	g_savedCount = 0;
	strncpy_s(g_status, "nothing unlocked", _TRUNCATE);
}

class RosterListener final : public IDeviceListener
{
public:
	void OnPresent(IDirect3DDevice9*) override
	{
		if (g_table == 0)
			return;

		if (g_settings.unlockHiddenCharacters && IsOffline())
		{
			Unlock();
			return;
		}

		if (g_savedCount != 0 || g_cellCount != 0)
			Restore();
	}
};

RosterListener g_listener;

}

void HiddenCharacters::Install()
{
	g_table = ResolveTable();
	g_networkFlag = ResolveNetworkFlag();

	Anchors::Record("Character table", g_table, "unplayable characters");
	Anchors::Record("Network state flag", g_networkFlag, "unplayable characters stay off online");

	if (g_table == 0 || g_networkFlag == 0)
		return;

	DeviceHooks::AddListener(&g_listener);
}

bool HiddenCharacters::IsAvailable()
{
	return g_table != 0 && g_networkFlag != 0;
}

const char* HiddenCharacters::StatusText()
{
	if (!IsAvailable())
		return "The character table or the online check was not found in this build.";

	if (g_settings.unlockHiddenCharacters && !IsOffline())
		return "Off while online.";

	return g_status;
}
