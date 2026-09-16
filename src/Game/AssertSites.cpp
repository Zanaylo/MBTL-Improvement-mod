#include "Game/AssertSites.h"

#include "Game/GameOffsets.h"
#include "Hooks/ImageScanner.h"

#include <cstdio>
#include <cstring>
#include <cwchar>

namespace {

namespace Asserts = GameOffsets::Asserts;

constexpr size_t kNameBytes = 96;
constexpr wchar_t kFirstNonAscii = 0x80;

bool InReadOnlyData(uintptr_t address)
{
	const ImageSection rdata = ImageScanner::ReadOnlyData();
	const auto begin = reinterpret_cast<uintptr_t>(rdata.begin);

	return address >= begin && address < begin + rdata.size;
}

const wchar_t* TextPushedBy(const uint8_t* push)
{
	if (push[0] != Asserts::kPushImm)
		return nullptr;

	const uintptr_t address = ImageScanner::ReadDword(push + 1);
	return InReadOnlyData(address) ? reinterpret_cast<const wchar_t*>(address) : nullptr;
}

bool ReadLine(uint8_t* call, AssertSite& site)
{
	uint8_t* const wide = call - Asserts::kPushLength * 3;

	if (wide[0] == Asserts::kPushImm && ImageScanner::ReadDword(wide + 1) <= Asserts::kLongestLine)
	{
		site.block = wide;
		site.line = ImageScanner::ReadDword(wide + 1);
		return true;
	}

	uint8_t* const narrow = call - Asserts::kPushLength * 2 - Asserts::kPushByteLength;

	if (narrow[0] != Asserts::kPushByte)
		return false;

	site.block = narrow;
	site.line = narrow[1];
	return true;
}

int EntryBound(const AssertSite& site)
{
	const uint8_t* const check = site.block - Asserts::kCheckLength;

	if (!ImageScanner::InCode(check, Asserts::kCheckLength) || check[0] != Asserts::kCompareRegister ||
		(check[1] & Asserts::kCompareModRmMask) != Asserts::kCompareModRm)
	{
		return -1;
	}

	const uint8_t jump = check[Asserts::kJumpAt];

	if ((jump != Asserts::kJumpBelow && jump != Asserts::kJumpLess) || check[Asserts::kJumpDistanceAt] != site.length)
		return -1;

	return check[Asserts::kBoundAt];
}

bool Parse(uint8_t* call, AssertSite& site)
{
	if (!ImageScanner::InCode(call - Asserts::kPushLength * 3, Asserts::kLongestBlock) ||
		std::memcmp(call + Asserts::kCallLength, Asserts::kCleanup, sizeof(Asserts::kCleanup)) != 0 ||
		!ReadLine(call, site))
	{
		return false;
	}

	site.expression = TextPushedBy(call - Asserts::kPushLength);
	site.file = TextPushedBy(call - Asserts::kPushLength * 2);

	if (!site.expression || !site.file)
		return false;

	site.length = static_cast<size_t>(call + Asserts::kCallLength + sizeof(Asserts::kCleanup) - site.block);
	site.bound = EntryBound(site);
	return true;
}

}

uint8_t* AssertSites::ImportSlot()
{
	return ImageScanner::ImportSlot(Asserts::kRuntimeLibrary, Asserts::kAssertImport);
}

std::vector<AssertSite> AssertSites::Find()
{
	std::vector<AssertSite> sites;
	const uint8_t* const slot = ImportSlot();

	if (!slot)
		return sites;

	uint8_t call[Asserts::kCallLength] = {};
	const auto address = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(slot));
	std::memcpy(call, Asserts::kCallSlot, sizeof(Asserts::kCallSlot));
	std::memcpy(call + sizeof(Asserts::kCallSlot), &address, sizeof(address));

	for (uint8_t* at : ImageScanner::FindBytes(ImageScanner::Code(), call, sizeof(call)))
	{
		AssertSite site;

		if (Parse(at, site))
			sites.push_back(site);
	}

	return sites;
}

bool AssertSites::FileIs(const wchar_t* path, const wchar_t* name)
{
	const size_t pathLength = std::wcslen(path);
	const size_t nameLength = std::wcslen(name);

	return pathLength >= nameLength && _wcsicmp(path + pathLength - nameLength, name) == 0;
}

void AssertSites::Narrow(const wchar_t* text, char* out, size_t size)
{
	if (size == 0)
		return;

	size_t i = 0;

	for (; text && text[i] != L'\0' && i + 1 < size; ++i)
		out[i] = text[i] < kFirstNonAscii ? static_cast<char>(text[i]) : '?';

	out[i] = '\0';
}

void AssertSites::Describe(const wchar_t* file, unsigned line, char* out, size_t size)
{
	const wchar_t* const slash = file ? std::wcsrchr(file, L'\\') : nullptr;
	char name[kNameBytes] = {};

	Narrow(slash ? slash + 1 : file, name, sizeof(name));
	std::snprintf(out, size, "%s:%u", name, line);
}
