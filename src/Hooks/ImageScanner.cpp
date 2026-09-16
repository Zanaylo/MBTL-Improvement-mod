#include "Hooks/ImageScanner.h"

#include <windows.h>

#include <algorithm>
#include <cstring>
#include <cwchar>
#include <utility>

namespace {

constexpr size_t kMaxFunctionWalk = 0x4000;
constexpr size_t kMaxFunctionBody = 0x10000;
constexpr size_t kFunctionAlignment = 16;
constexpr uint8_t kPushEbp = 0x55;
constexpr uint8_t kMovEbpEsp[] = { 0x8B, 0xEC };
constexpr uint8_t kInt3 = 0xCC;
constexpr uint8_t kRet = 0xC3;
constexpr uint8_t kRetImm = 0xC2;
constexpr size_t kRetImmLength = 3;
constexpr uint8_t kCall = 0xE8;
constexpr size_t kCallLength = 5;
constexpr uint8_t kJumpNear = 0xE9;
constexpr size_t kJumpNearLength = 5;
constexpr uint8_t kJumpShort = 0xEB;
constexpr size_t kJumpShortLength = 2;
constexpr uint8_t kPushImm = 0x68;
constexpr size_t kPushLength = 5;
constexpr size_t kNativeWindow = 0x28;
constexpr uint8_t kStoreLocal = 0xC7;
constexpr uint8_t kStoreNearModRm = 0x45;
constexpr size_t kStoreNearValueAt = 3;
constexpr uint8_t kStoreFarModRm = 0x85;
constexpr size_t kStoreFarValueAt = 6;
constexpr size_t kStoreFarLength = 10;
constexpr uint8_t kMovEaxImm = 0xB8;
constexpr uint8_t kMovEaxMemory = 0xA1;
constexpr uint8_t kMovAlMemory = 0xA0;
constexpr uint8_t kFramedHead[] = { 0x55, 0x8B, 0xEC };
constexpr uint8_t kGetterTail[] = { 0x5D, 0xC3 };
constexpr size_t kBareGetterLength = 6;
constexpr size_t kFramedGetterLength = 10;
constexpr size_t kPrologueLength = 3;
constexpr size_t kSmallestFramedFunction = 16;
constexpr uint8_t kSehFrame[] = { 0x6A, 0xFF };
constexpr uint8_t kPushEcx = 0x51;
constexpr uint8_t kSubEspByte[] = { 0x83, 0xEC };
constexpr size_t kSubEspByteLength = 3;
constexpr uint8_t kSubEspDword[] = { 0x81, 0xEC };
constexpr size_t kSubEspDwordLength = 6;
constexpr uint8_t kMovEspEbp[] = { 0x8B, 0xE5 };
constexpr uint8_t kPopEbp = 0x5D;
constexpr uint8_t kSavedPushes[] = { 0x53, 0x56, 0x57 };
constexpr uint8_t kSavedPops[] = { 0x5B, 0x5E, 0x5F };

uint8_t* g_base = nullptr;
ImageSection g_code;
ImageSection g_rdata;
ImageSection g_data;
uint32_t g_timeDateStamp = 0;

using CallPair = std::pair<const uint8_t*, uint8_t*>;
std::vector<CallPair> g_calls;
std::vector<uint8_t*> g_starts;

IMAGE_NT_HEADERS32* HeadersOf(uint8_t* base)
{
	const auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
	if (dos->e_magic != IMAGE_DOS_SIGNATURE)
		return nullptr;

	const auto nt = reinterpret_cast<IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
	if (nt->Signature != IMAGE_NT_SIGNATURE)
		return nullptr;

	return nt;
}

bool Inside(const ImageSection& section, const uint8_t* address, size_t length)
{
	return section.begin && address >= section.begin && address + length <= section.begin + section.size;
}

void AddUnique(std::vector<uint8_t*>& list, uint8_t* value)
{
	if (std::find(list.begin(), list.end(), value) == list.end())
		list.push_back(value);
}

bool IsPrologue(const uint8_t* at)
{
	return at[0] == kPushEbp && at[1] == kMovEbpEsp[0] && at[2] == kMovEbpEsp[1];
}

bool FollowsBoundary(const uint8_t* at)
{
	if (!Inside(g_code, at - kJumpNearLength, kJumpNearLength))
		return false;

	return at[-1] == kInt3 || at[-1] == kRet || *(at - kRetImmLength) == kRetImm ||
		*(at - kJumpNearLength) == kJumpNear || *(at - kJumpShortLength) == kJumpShort;
}

uint8_t* CallTargetAt(const uint8_t* site)
{
	int32_t relative = 0;
	std::memcpy(&relative, site + 1, sizeof(relative));
	const auto target = const_cast<uint8_t*>(site + 5 + relative);

	return ImageScanner::InCode(target, 1) ? target : nullptr;
}

void BuildCallIndex()
{
	if (!g_calls.empty())
		return;

	const uint8_t* const last = g_code.begin + g_code.size - 5;

	for (const uint8_t* cursor = g_code.begin; cursor < last; ++cursor)
	{
		cursor = static_cast<const uint8_t*>(std::memchr(cursor, kCall, static_cast<size_t>(last - cursor)));
		if (!cursor)
			break;

		uint8_t* const target = CallTargetAt(cursor);
		if (target)
			g_calls.emplace_back(target, const_cast<uint8_t*>(cursor));
	}

	std::sort(g_calls.begin(), g_calls.end());
}

void BuildFunctionIndex()
{
	if (!g_starts.empty())
		return;

	BuildCallIndex();

	const uint8_t* const last = g_code.begin + g_code.size;

	for (uint8_t* cursor = g_code.begin + 1; cursor < last; ++cursor)
	{
		cursor = static_cast<uint8_t*>(std::memchr(cursor, kInt3, static_cast<size_t>(last - cursor)));
		if (!cursor)
			break;

		uint8_t* const start = cursor + 1;
		if (start >= last || *start == kInt3)
			continue;
		if ((reinterpret_cast<uintptr_t>(start) % kFunctionAlignment) != 0)
			continue;

		const uint8_t* run = cursor;
		while (run > g_code.begin && run[-1] == kInt3)
			--run;

		if (FollowsBoundary(run))
			g_starts.push_back(start);
	}

	for (const CallPair& call : g_calls)
	{
		uint8_t* const target = const_cast<uint8_t*>(call.first);

		if (FollowsBoundary(target))
			g_starts.push_back(target);
	}

	std::sort(g_starts.begin(), g_starts.end());
	g_starts.erase(std::unique(g_starts.begin(), g_starts.end()), g_starts.end());
}

uint8_t* NextStart(const uint8_t* start)
{
	BuildFunctionIndex();

	const auto next = std::upper_bound(g_starts.begin(), g_starts.end(), start);
	return next == g_starts.end() ? nullptr : *next;
}

uintptr_t GetterWith(const uint8_t* function, uint8_t opcode)
{
	if (!ImageScanner::InCode(function, kFramedGetterLength))
		return 0;

	if (function[0] == opcode && function[kBareGetterLength - 1] == kRet)
		return ImageScanner::ReadDword(function + 1);

	if (std::memcmp(function, kFramedHead, sizeof(kFramedHead)) != 0)
		return 0;

	if (function[kPrologueLength] != opcode ||
		std::memcmp(function + kFramedGetterLength - sizeof(kGetterTail), kGetterTail, sizeof(kGetterTail)) != 0)
	{
		return 0;
	}

	return ImageScanner::ReadDword(function + kPrologueLength + 1);
}

bool IsOneOf(uint8_t value, const uint8_t* values, size_t count)
{
	return std::find(values, values + count, value) != values + count;
}

struct Frame
{
	bool locals = false;
	size_t saved = 0;
};

Frame FrameOf(const uint8_t* function)
{
	Frame frame;
	size_t cursor = kPrologueLength;

	if (function[cursor] == kPushEcx)
	{
		frame.locals = true;
		cursor += sizeof(kPushEcx);
	}
	else if (std::memcmp(function + cursor, kSubEspByte, sizeof(kSubEspByte)) == 0)
	{
		frame.locals = true;
		cursor += kSubEspByteLength;
	}
	else if (std::memcmp(function + cursor, kSubEspDword, sizeof(kSubEspDword)) == 0)
	{
		frame.locals = true;
		cursor += kSubEspDwordLength;
	}

	while (IsOneOf(function[cursor + frame.saved], kSavedPushes, sizeof(kSavedPushes)))
		++frame.saved;

	return frame;
}

uint8_t* AddressIn(uint32_t value)
{
	const auto address = reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(value));
	return ImageScanner::InCode(address, 1) ? address : nullptr;
}

uint8_t* NativeBefore(const uint8_t* push)
{
	for (size_t i = 1; i <= kNativeWindow; ++i)
	{
		const uint8_t* const at = push - i;

		if (!Inside(g_code, at, kStoreFarLength))
			return nullptr;

		if (at[0] == kPushImm)
			return AddressIn(ImageScanner::ReadDword(at + 1));
		if (at[0] == kStoreLocal && at[1] == kStoreNearModRm)
			return AddressIn(ImageScanner::ReadDword(at + kStoreNearValueAt));
		if (at[0] == kStoreLocal && at[1] == kStoreFarModRm)
			return AddressIn(ImageScanner::ReadDword(at + kStoreFarValueAt));
	}

	return nullptr;
}

}

bool ImageScanner::Initialize()
{
	if (g_base)
		return true;

	const auto base = reinterpret_cast<uint8_t*>(GetModuleHandleA(nullptr));
	if (!base)
		return false;

	IMAGE_NT_HEADERS32* nt = HeadersOf(base);
	if (!nt)
		return false;

	IMAGE_SECTION_HEADER* section = IMAGE_FIRST_SECTION(nt);
	for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++section)
	{
		const ImageSection range{ base + section->VirtualAddress, section->Misc.VirtualSize };
		if (std::memcmp(section->Name, ".text", 6) == 0)
			g_code = range;
		if (std::memcmp(section->Name, ".rdata", 7) == 0)
			g_rdata = range;
		if (std::memcmp(section->Name, ".data", 6) == 0)
			g_data = range;
	}

	if (!g_code.begin || !g_rdata.begin || !g_data.begin)
		return false;

	g_timeDateStamp = nt->FileHeader.TimeDateStamp;
	g_base = base;
	return true;
}

void ImageScanner::ReleaseCallIndex()
{
	std::vector<CallPair>().swap(g_calls);
	std::vector<uint8_t*>().swap(g_starts);
}

uint8_t* ImageScanner::Base()
{
	return g_base;
}

ImageSection ImageScanner::Code()
{
	return g_code;
}

ImageSection ImageScanner::ReadOnlyData()
{
	return g_rdata;
}

uint32_t ImageScanner::TimeDateStamp()
{
	return g_timeDateStamp;
}

bool ImageScanner::InCode(const uint8_t* address, size_t length)
{
	return Inside(g_code, address, length);
}

bool ImageScanner::InData(uintptr_t address)
{
	const IMAGE_NT_HEADERS32* const nt = g_base ? HeadersOf(g_base) : nullptr;

	if (!nt)
		return false;

	const uintptr_t limit = reinterpret_cast<uintptr_t>(g_base) + nt->OptionalHeader.SizeOfImage;
	return address >= reinterpret_cast<uintptr_t>(g_data.begin) && address < limit;
}

uint32_t ImageScanner::ReadDword(const uint8_t* address)
{
	uint32_t value = 0;
	std::memcpy(&value, address, sizeof(value));
	return value;
}

std::vector<uint8_t*> ImageScanner::FindBytes(ImageSection where, const uint8_t* bytes, size_t length)
{
	std::vector<uint8_t*> hits;
	if (!where.begin || !bytes || length == 0 || where.size < length)
		return hits;

	const uint8_t* last = where.begin + where.size - length;
	for (uint8_t* cursor = where.begin; cursor <= last; ++cursor)
	{
		cursor = static_cast<uint8_t*>(std::memchr(cursor, bytes[0], static_cast<size_t>(last - cursor) + 1));
		if (!cursor)
			break;
		if (std::memcmp(cursor, bytes, length) == 0)
			hits.push_back(cursor);
	}

	return hits;
}

std::vector<uint8_t*> ImageScanner::FindString(const char* text)
{
	return FindBytes(g_rdata, reinterpret_cast<const uint8_t*>(text), std::strlen(text) + 1);
}

std::vector<uint8_t*> ImageScanner::FindWideString(const wchar_t* text)
{
	return FindBytes(g_rdata, reinterpret_cast<const uint8_t*>(text), (std::wcslen(text) + 1) * sizeof(wchar_t));
}

std::vector<uint8_t*> ImageScanner::FindReferencesTo(const uint8_t* address)
{
	const auto value = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(address));
	return FindBytes(g_code, reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

uint8_t* ImageScanner::FunctionStart(uint8_t* inside)
{
	if (!InCode(inside, 1))
		return nullptr;

	BuildFunctionIndex();

	const auto next = std::upper_bound(g_starts.begin(), g_starts.end(), inside);
	if (next == g_starts.begin())
		return nullptr;

	uint8_t* const start = *(next - 1);
	return static_cast<size_t>(inside - start) <= kMaxFunctionWalk ? start : nullptr;
}

size_t ImageScanner::FunctionLength(const uint8_t* start)
{
	const uint8_t* const next = NextStart(start);
	if (!next || static_cast<size_t>(next - start) > kMaxFunctionBody)
		return 0;

	const uint8_t* end = next;
	while (end > start && end[-1] == kInt3)
		--end;

	return static_cast<size_t>(end - start);
}

std::vector<uint8_t*> ImageScanner::CallSequence(const uint8_t* function)
{
	std::vector<uint8_t*> targets;
	const size_t length = FunctionLength(function);

	for (size_t i = 0; i + 5 <= length; ++i)
	{
		if (function[i] != kCall)
			continue;

		uint8_t* const target = CallTargetAt(function + i);
		if (target)
			targets.push_back(target);
	}

	return targets;
}

std::vector<uint8_t*> ImageScanner::CallTargets(const uint8_t* function)
{
	std::vector<uint8_t*> unique;

	for (uint8_t* target : CallSequence(function))
		AddUnique(unique, target);

	return unique;
}

std::vector<uint8_t*> ImageScanner::CallsCleanedBy(const uint8_t* function, const uint8_t* cleanup, size_t count)
{
	std::vector<uint8_t*> targets;
	const size_t length = FunctionLength(function);

	for (size_t i = 0; i + kCallLength + count <= length; ++i)
	{
		if (function[i] != kCall || std::memcmp(function + i + kCallLength, cleanup, count) != 0)
			continue;

		uint8_t* const target = CallTargetAt(function + i);

		if (target)
			targets.push_back(target);
	}

	return targets;
}

uint8_t* ImageScanner::CallTargetOf(const uint8_t* site)
{
	return InCode(site, kCallLength) && site[0] == kCall ? CallTargetAt(site) : nullptr;
}

std::vector<uint8_t*> ImageScanner::CallersOf(const uint8_t* target)
{
	BuildCallIndex();

	std::vector<uint8_t*> sites;
	const auto range = std::equal_range(g_calls.begin(), g_calls.end(), CallPair(target, nullptr),
		[](const CallPair& a, const CallPair& b) { return a.first < b.first; });

	for (auto it = range.first; it != range.second; ++it)
		sites.push_back(it->second);

	return sites;
}

std::vector<uint8_t*> ImageScanner::CallerFunctions(const uint8_t* target)
{
	std::vector<uint8_t*> functions;

	for (uint8_t* site : CallersOf(target))
	{
		uint8_t* const start = FunctionStart(site);
		if (start)
			AddUnique(functions, start);
	}

	return functions;
}

std::vector<uint8_t*> ImageScanner::FunctionsReferencing(const std::vector<uint8_t*>& addresses)
{
	std::vector<uint8_t*> functions;

	for (const uint8_t* address : addresses)
	{
		for (uint8_t* site : FindReferencesTo(address))
		{
			uint8_t* start = FunctionStart(site);
			if (start)
				AddUnique(functions, start);
		}
	}

	return functions;
}

uint8_t* ImageScanner::NativeFunction(const char* bindingName)
{
	const std::vector<uint8_t*> names = FindString(bindingName);
	if (names.size() != 1)
		return nullptr;

	uint8_t* native = nullptr;
	int found = 0;

	for (uint8_t* site : FindReferencesTo(names[0]))
	{
		if (*(site - 1) != kPushImm)
			continue;

		uint8_t* const address = NativeBefore(site - 1);
		if (!address)
			continue;

		native = address;
		++found;
	}

	return found == 1 ? native : nullptr;
}

uintptr_t ImageScanner::GetterValue(const uint8_t* function)
{
	return GetterWith(function, kMovEaxImm);
}

uintptr_t ImageScanner::MemoryGetterValue(const uint8_t* function)
{
	const uintptr_t address = GetterWith(function, kMovEaxMemory);
	return InData(address) ? address : 0;
}

uintptr_t ImageScanner::ByteGetterValue(const uint8_t* function)
{
	const uintptr_t address = GetterWith(function, kMovAlMemory);
	return InData(address) ? address : 0;
}

uint8_t* ImageScanner::ImportSlot(const char* library, const char* function)
{
	if (!g_base)
		return nullptr;

	const IMAGE_NT_HEADERS32* nt = HeadersOf(g_base);
	const IMAGE_DATA_DIRECTORY& directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	if (!directory.VirtualAddress)
		return nullptr;

	for (auto descriptor = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(g_base + directory.VirtualAddress);
		descriptor->Name; ++descriptor)
	{
		if (_stricmp(reinterpret_cast<const char*>(g_base + descriptor->Name), library) != 0)
			continue;
		if (!descriptor->OriginalFirstThunk)
			return nullptr;

		const auto names = reinterpret_cast<IMAGE_THUNK_DATA32*>(g_base + descriptor->OriginalFirstThunk);
		for (DWORD i = 0; names[i].u1.AddressOfData; ++i)
		{
			if (IMAGE_SNAP_BY_ORDINAL32(names[i].u1.Ordinal))
				continue;

			const auto byName = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(g_base + names[i].u1.AddressOfData);
			if (std::strcmp(reinterpret_cast<const char*>(byName->Name), function) == 0)
				return g_base + descriptor->FirstThunk + i * sizeof(IMAGE_THUNK_DATA32);
		}
	}

	return nullptr;
}

bool ImageScanner::Contains(const uint8_t* start, size_t length, const uint8_t* bytes, size_t count)
{
	for (size_t i = 0; i + count <= length; ++i)
	{
		if (std::memcmp(start + i, bytes, count) == 0)
			return true;
	}

	return false;
}

const uint8_t* ImageScanner::AfterPushOf(const uint8_t* function, size_t length, const uint8_t* value)
{
	uint8_t push[kPushLength] = { kPushImm };
	const auto address = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(value));
	std::memcpy(push + 1, &address, sizeof(address));

	for (size_t i = 0; i + kPushLength <= length; ++i)
	{
		if (std::memcmp(function + i, push, kPushLength) == 0)
			return function + i + kPushLength;
	}

	return nullptr;
}

bool ImageScanner::ReturnsWith(const uint8_t* function, uint16_t stackBytes)
{
	const size_t length = FunctionLength(function);
	uint16_t popped = 0;

	if (length == 0)
		return false;

	if (stackBytes == 0 && function[length - 1] == kRet)
		return true;

	if (length < kRetImmLength || function[length - kRetImmLength] != kRetImm)
		return false;

	std::memcpy(&popped, function + length - sizeof(popped), sizeof(popped));
	return popped == stackBytes;
}

uint8_t* ImageScanner::Epilogue(uint8_t* function)
{
	const size_t length = FunctionLength(function);

	if (length < kSmallestFramedFunction || !IsPrologue(function) ||
		std::memcmp(function + kPrologueLength, kSehFrame, sizeof(kSehFrame)) == 0)
	{
		return nullptr;
	}

	const size_t ret = function[length - kRetImmLength] == kRetImm ? length - kRetImmLength : length - 1;

	if ((function[ret] != kRet && function[ret] != kRetImm) || function[ret - 1] != kPopEbp)
		return nullptr;

	size_t exit = ret - 1;
	const bool restoresStack =
		std::memcmp(function + exit - sizeof(kMovEspEbp), kMovEspEbp, sizeof(kMovEspEbp)) == 0;

	if (restoresStack)
		exit -= sizeof(kMovEspEbp);

	const Frame frame = FrameOf(function);

	if (frame.locals && !restoresStack)
		return nullptr;

	for (size_t i = 0; i < frame.saved; ++i, --exit)
	{
		if (!IsOneOf(function[exit - 1], kSavedPops, sizeof(kSavedPops)))
			return nullptr;
	}

	return function + exit;
}

bool ImageScanner::CallsImport(const uint8_t* function, const char* library, const char* name)
{
	const uint8_t* const slot = ImportSlot(library, name);
	if (!slot)
		return false;

	const auto address = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(slot));
	const size_t length = FunctionLength(function);

	for (size_t i = 0; i + sizeof(address) <= length; ++i)
	{
		if (ReadDword(function + i) == address)
			return true;
	}

	return false;
}
