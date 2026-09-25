#include "Game/ColourSlots.h"

#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Core/utils.h"
#include "D3D9/DeviceHooks.h"
#include "Game/Anchors.h"
#include "Game/GameOffsets.h"
#include "Hooks/CodePatch.h"
#include "Hooks/ImageScanner.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {

namespace Asserts = GameOffsets::Asserts;
namespace Colours = GameOffsets::Colours;

constexpr int kExtraSlots = Colours::kPickerSlots - Colours::kStockSlots;

uintptr_t g_table = 0;
int g_charas = 0;
std::vector<uint8_t*> g_dispatch;

bool g_widened = false;
bool g_granted = false;
uint8_t g_previous[Colours::kCharas][kExtraSlots] = {};

char g_status[96] = "";

uint8_t* DispatchIn(uint8_t* function)
{
	const size_t length = ImageScanner::FunctionLength(function);
	uint8_t* found = nullptr;

	for (size_t i = Colours::kLoadAddressLength; i + Colours::kCompareLength <= length; ++i)
	{
		if (std::memcmp(function + i, Colours::kCompareEax, sizeof(Colours::kCompareEax)) != 0 ||
			function[i + Colours::kRangeAt] != Colours::kNarrowRange ||
			function[i + Colours::kJumpAt] != Colours::kJumpAbove ||
			function[i - Colours::kLoadAddressLength] != Colours::kLoadAddress)
		{
			continue;
		}

		if (found != nullptr)
			return nullptr;

		found = function + i;
	}

	return found;
}

void TablesIn(const uint8_t* function, std::vector<uintptr_t>& out)
{
	const size_t length = ImageScanner::FunctionLength(function);

	for (size_t i = 0; i + Colours::kLoadByteLength + sizeof(uint32_t) <= length; ++i)
	{
		if (function[i] != Colours::kLoadByte)
			continue;

		const uint8_t modRm = function[i + 1];

		if ((modRm & Colours::kModRmMask) != Colours::kDisplaced)
			continue;

		const bool scaled = (modRm & Colours::kRegisterMask) == Colours::kScaledIndex;
		const uintptr_t base = ImageScanner::ReadDword(function + i + (scaled ? 3 : 2));

		if (ImageScanner::InData(base) && std::find(out.begin(), out.end(), base) == out.end())
			out.push_back(base);
	}
}

uintptr_t ResolveTable(const std::vector<uint8_t*>& functions)
{
	std::vector<uintptr_t> shared;

	for (const uint8_t* function : functions)
	{
		std::vector<uintptr_t> tables;
		TablesIn(function, tables);

		if (tables.empty())
			continue;

		if (shared.empty())
		{
			shared.swap(tables);
			continue;
		}

		shared.erase(std::remove_if(shared.begin(), shared.end(),
			[&tables](uintptr_t base) { return std::find(tables.begin(), tables.end(), base) == tables.end(); }),
			shared.end());
	}

	if (shared.size() == 1)
		return shared.front();

	LOG("ColourSlots: the unlock table has %u candidate(s), expected one", static_cast<unsigned>(shared.size()));
	return 0;
}

int ResolveCharas(const std::vector<uint8_t*>& functions)
{
	std::vector<int> counts;

	for (const uint8_t* function : functions)
	{
		const size_t length = ImageScanner::FunctionLength(function);

		for (size_t i = 0; i + Colours::kCompareImmediateLength <= length; ++i)
		{
			if (function[i] != Colours::kCompareEaxImmediate)
				continue;

			const uint32_t span = ImageScanner::ReadDword(function + i + 1);

			if (span == 0 || span % Colours::kSlots != 0)
				continue;

			const int charas = static_cast<int>(span / Colours::kSlots);

			if (charas <= Colours::kMostCharas && std::find(counts.begin(), counts.end(), charas) == counts.end())
				counts.push_back(charas);
		}
	}

	if (counts.size() == 1 && counts.front() <= Colours::kCharas)
		return counts.front();

	LOG("ColourSlots: the character count has %u candidate(s), expected one", static_cast<unsigned>(counts.size()));
	return 0;
}

void SetRange(uint8_t range, const char* what)
{
	size_t written = 0;

	for (uint8_t* at : g_dispatch)
		written += CodePatch::Write(at + Colours::kRangeAt, &range, sizeof(range)) ? 1 : 0;

	if (written != g_dispatch.size())
		LOG("ColourSlots: %u of %u slot check(s) %s", static_cast<unsigned>(written),
			static_cast<unsigned>(g_dispatch.size()), what);
}

bool ReadTable(uint8_t* out)
{
	return TryReadMemory(out, reinterpret_cast<const void*>(g_table),
		static_cast<size_t>(g_charas) * Colours::kSlots);
}

bool Missing()
{
	uint8_t table[Colours::kCharas * Colours::kSlots] = {};

	if (!ReadTable(table))
		return false;

	for (int chara = 0; chara < g_charas; ++chara)
	{
		for (int slot = Colours::kStockSlots; slot < Colours::kPickerSlots; ++slot)
		{
			if (table[chara * Colours::kSlots + slot] != Colours::kGranted)
				return true;
		}
	}

	return false;
}

void Grant()
{
	uint8_t table[Colours::kCharas * Colours::kSlots] = {};

	if (!ReadTable(table))
		return;

	for (int chara = 0; chara < g_charas; ++chara)
	{
		for (int slot = Colours::kStockSlots; slot < Colours::kPickerSlots; ++slot)
		{
			const uint8_t held = table[chara * Colours::kSlots + slot];

			if (!g_granted)
				g_previous[chara][slot - Colours::kStockSlots] = held;

			if (held != Colours::kGranted)
				TryWrite(g_table + chara * Colours::kSlots + slot, Colours::kGranted);
		}
	}

	g_granted = true;
}

void Revoke()
{
	for (int chara = 0; chara < g_charas; ++chara)
	{
		for (int slot = Colours::kStockSlots; slot < Colours::kPickerSlots; ++slot)
			TryWrite(g_table + chara * Colours::kSlots + slot, g_previous[chara][slot - Colours::kStockSlots]);
	}

	g_granted = false;
}

class ColourListener final : public IDeviceListener
{
public:
	void OnPresent(IDirect3DDevice9*) override
	{
		if (g_settings.unlockColourSlots)
		{
			if (!g_widened)
			{
				SetRange(Colours::kWideRange, "widened");
				g_widened = true;
				sprintf_s(g_status, "%d slot(s) added, %d in all", kExtraSlots, Colours::kPickerSlots);
			}

			if (Missing())
				Grant();

			return;
		}

		if (g_granted)
			Revoke();

		if (!g_widened)
			return;

		SetRange(Colours::kNarrowRange, "narrowed");
		g_widened = false;
		strncpy_s(g_status, "the stock slots only", _TRUNCATE);
	}
};

ColourListener g_listener;

}

void ColourSlots::Install()
{
	const std::vector<uint8_t*> functions =
		ImageScanner::FunctionsReferencing(ImageScanner::FindWideString(Asserts::kUnlockedColourCheck));

	for (uint8_t* function : functions)
	{
		uint8_t* const dispatch = DispatchIn(function);

		if (dispatch != nullptr)
			g_dispatch.push_back(dispatch);
	}

	g_charas = ResolveCharas(functions);
	g_table = g_dispatch.empty() || g_charas == 0 ? 0 : ResolveTable(functions);

	Anchors::Record("Colour unlock table", g_table, "unused colour slots");

	if (g_table == 0)
	{
		LOG("ColourSlots: %u colour function(s) and %u slot check(s), nothing is unlocked",
			static_cast<unsigned>(functions.size()), static_cast<unsigned>(g_dispatch.size()));
		return;
	}

	LOG("ColourSlots: %u slot check(s) in %u colour function(s), %d characters",
		static_cast<unsigned>(g_dispatch.size()), static_cast<unsigned>(functions.size()), g_charas);
	strncpy_s(g_status, "the stock slots only", _TRUNCATE);
	DeviceHooks::AddListener(&g_listener);
}

bool ColourSlots::IsAvailable()
{
	return g_table != 0 && !g_dispatch.empty();
}

const char* ColourSlots::StatusText()
{
	if (!IsAvailable())
		return "Not supported on this game version.";

	return g_status;
}
