#include "Stages/CharacterLight.h"

#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Game/Anchors.h"
#include "Game/GameOffsets.h"
#include "Hooks/HookManager.h"
#include "Hooks/ImageScanner.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace {

namespace Light = GameOffsets::Light;

using PixelReader_t = int(__fastcall*)(void*, void*, int, int, int*, int*, int*, int*);

struct Image
{
	const char* name;
	int target;
};

constexpr Image kImages[] = {
	{ Light::kColourImage, Light::kWhite },
	{ Light::kSpecularImage, Light::kNone },
	{ Light::kBokashiImage, Light::kNone },
};

constexpr size_t kImageCount = sizeof(kImages) / sizeof(kImages[0]);

struct Bank
{
	uintptr_t first = 0;
	uintptr_t end = 0;
	int target = 0;
	uint8_t* reader = nullptr;
};

PixelReader_t oReader = nullptr;
Bank g_banks[kImageCount];
bool g_hooked = false;
const char* g_status = "not found in this game version";

int Percent()
{
	return std::clamp(g_settings.lightStrength, 0, Light::kFullPercent);
}

const Bank* BankOf(const void* image)
{
	const uintptr_t at = reinterpret_cast<uintptr_t>(image) - Light::kImageOffset;

	for (const Bank& bank : g_banks)
	{
		if (at >= bank.first && at < bank.end && (at - bank.first) % Light::kBankStride == 0)
			return &bank;
	}

	return nullptr;
}

void Blend(int* value, int target, int percent)
{
	if (value != nullptr)
		*value = target + (*value - target) * percent / Light::kFullPercent;
}

int __fastcall HookedReader(void* image, void* edx, int x, int y, int* r, int* g, int* b, int* a)
{
	const int result = oReader(image, edx, x, y, r, g, b, a);
	const int percent = Percent();

	if (percent == Light::kFullPercent)
		return result;

	const Bank* const bank = BankOf(image);

	if (!bank)
		return result;

	Blend(r, bank->target, percent);
	Blend(g, bank->target, percent);
	Blend(b, bank->target, percent);
	return result;
}

std::vector<uint8_t*> Users(const char* name)
{
	return ImageScanner::FunctionsReferencing(ImageScanner::FindString(name));
}

uint8_t* ResolveLoader()
{
	std::vector<uint8_t*> common = Users(kImages[0].name);

	for (size_t i = 1; i < kImageCount; ++i)
	{
		const std::vector<uint8_t*> users = Users(kImages[i].name);

		common.erase(std::remove_if(common.begin(), common.end(), [&users](uint8_t* function) {
			return std::find(users.begin(), users.end(), function) == users.end();
		}), common.end());
	}

	if (common.size() == 1)
		return common.front();

	LOG("CharacterLight: the stage light loader has %u candidate(s), expected exactly one",
		static_cast<unsigned>(common.size()));
	return nullptr;
}

size_t PushOf(const uint8_t* loader, size_t length, const char* name)
{
	const std::vector<uint8_t*> strings = ImageScanner::FindString(name);

	for (size_t i = 0; i + Light::kPushLength <= length; ++i)
	{
		if (loader[i] != Light::kPushImmediate)
			continue;

		const auto pushed = reinterpret_cast<uint8_t*>(ImageScanner::ReadDword(loader + i + 1));

		if (std::find(strings.begin(), strings.end(), pushed) != strings.end())
			return i;
	}

	return length;
}

uintptr_t BankLoadedAfter(const uint8_t* loader, size_t length, size_t push, const uintptr_t* registers)
{
	uintptr_t ecx = 0;

	for (size_t at = push + Light::kPushLength; at + Light::kLeaLength <= length && loader[at] != Light::kCall; ++at)
	{
		if (loader[at] == Light::kLea && loader[at + 1] == Light::kLeaEcx)
			ecx = ImageScanner::ReadDword(loader + at + 2);

		if (loader[at] == Light::kMov && loader[at + 1] >= Light::kMovEcxFirst && loader[at + 1] <= Light::kMovEcxLast)
			ecx = registers[loader[at + 1] & Light::kRegisterMask];
	}

	return ImageScanner::InData(ecx) ? ecx : 0;
}

bool DescribeBank(uintptr_t first, Bank& out)
{
	const uint8_t pattern[] = { Light::kPushByte, Light::kBankStride, Light::kPushImmediate,
		static_cast<uint8_t>(first), static_cast<uint8_t>(first >> 8), static_cast<uint8_t>(first >> 16),
		static_cast<uint8_t>(first >> 24) };
	int matches = 0;

	for (const uint8_t* site : ImageScanner::FindBytes(ImageScanner::Code(), pattern, sizeof(pattern)))
	{
		const uint8_t* const head = site - Light::kVectorPushes;

		if (!ImageScanner::InCode(head, Light::kVectorPushes) || head[0] != Light::kPushImmediate ||
			head[Light::kCtorPushAt] != Light::kPushImmediate || head[Light::kCountPushAt] != Light::kPushByte)
		{
			continue;
		}

		const auto ctor = reinterpret_cast<const uint8_t*>(ImageScanner::ReadDword(head + Light::kCtorPushAt + 1));

		if (!ImageScanner::InCode(ctor, sizeof(Light::kSetVtable) + sizeof(uint32_t)) ||
			std::memcmp(ctor, Light::kSetVtable, sizeof(Light::kSetVtable)) != 0)
		{
			continue;
		}

		const auto vtable = reinterpret_cast<const uint8_t*>(ImageScanner::ReadDword(ctor + sizeof(Light::kSetVtable)));
		out.first = first;
		out.end = first + static_cast<uintptr_t>(head[Light::kCountPushAt + 1]) * Light::kBankStride;
		out.reader = reinterpret_cast<uint8_t*>(ImageScanner::ReadDword(vtable + Light::kReaderSlot * sizeof(uint32_t)));
		++matches;
	}

	return matches == 1 && ImageScanner::InCode(out.reader, 1);
}

bool Resolve()
{
	uint8_t* const loader = ResolveLoader();

	if (!loader)
		return false;

	const size_t length = ImageScanner::FunctionLength(loader);
	uintptr_t registers[Light::kRegisters] = {};

	for (size_t i = 0; i + Light::kLeaLength <= length; ++i)
	{
		if (loader[i] == Light::kLea && (loader[i + 1] & Light::kLeaEaxDisp32Mask) == Light::kLeaEaxDisp32)
			registers[(loader[i + 1] >> Light::kRegisterShift) & Light::kRegisterMask] = ImageScanner::ReadDword(loader + i + 2);
	}

	for (size_t i = 0; i < kImageCount; ++i)
	{
		const uintptr_t first = BankLoadedAfter(loader, length, PushOf(loader, length, kImages[i].name), registers);

		if (!first || !DescribeBank(first, g_banks[i]))
		{
			LOG("CharacterLight: the %s bank was not found", kImages[i].name);
			return false;
		}

		g_banks[i].target = kImages[i].target;

		if (g_banks[i].reader != g_banks[0].reader)
		{
			LOG("CharacterLight: the light banks do not share one pixel reader");
			return false;
		}
	}

	return true;
}

}

void CharacterLight::Install()
{
	const bool resolved = Resolve();

	Anchors::Record("Character colour bank", g_banks[0].first, "character light");
	Anchors::Record("Character specular bank", g_banks[1].first, "character light");
	Anchors::Record("Character bokashi bank", g_banks[2].first, "character light");
	Anchors::Record("Stage light pixel reader", reinterpret_cast<uintptr_t>(g_banks[0].reader), "character light");

	if (!resolved)
	{
		LOG("CharacterLight: %s", g_status);
		return;
	}

	g_hooked = HookManager::CreateHook(g_banks[0].reader, reinterpret_cast<void*>(&HookedReader),
		reinterpret_cast<void**>(&oReader), "stage light pixel reader");

	g_status = g_hooked ? "" : "could not be turned on";
	LOG("CharacterLight: %s", g_hooked ? "ready" : g_status);
}

bool CharacterLight::IsAvailable()
{
	return g_hooked;
}

const char* CharacterLight::StatusText()
{
	return g_status;
}
