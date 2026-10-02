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
constexpr uint8_t kPushRegister = 0x50;
constexpr uint8_t kPopRegister = 0x58;
constexpr uint8_t kOpcodeRegisterMask = 0xF8;
constexpr uint8_t kRegisterMask = 0x07;
constexpr int kRegisterShift = 3;
constexpr int kModShift = 6;
constexpr uint8_t kStackRegister = 4;
constexpr uint8_t kFrameRegister = 5;
constexpr uint8_t kModNoDisplacement = 0;
constexpr uint8_t kModDisp8 = 1;
constexpr uint8_t kModDisp32 = 2;
constexpr uint8_t kModRegister = 3;
constexpr uint8_t kSibFollows = 4;
constexpr uint8_t kMovRegister = 0x8B;
constexpr size_t kOpcodeLength = 1;
constexpr uint8_t kMovEspEbp[] = { 0x8B, 0xE5 };
constexpr uint8_t kSubEspByte[] = { 0x83, 0xEC };
constexpr uint8_t kAddEspByte[] = { 0x83, 0xC4 };
constexpr size_t kEspByteLength = 3;
constexpr uint8_t kSubEspDword[] = { 0x81, 0xEC };
constexpr uint8_t kAddEspDword[] = { 0x81, 0xC4 };
constexpr size_t kEspDwordLength = 6;
constexpr size_t kMovEaxImmLength = 5;
constexpr size_t kMostEntrySteps = 8;

uint8_t* g_base = nullptr;
ImageSection g_code;
ImageSection g_rdata;
ImageSection g_data;
ImageSection g_initialisedData;
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

struct EntryStep
{
	uint8_t undo[kEspDwordLength] = {};
	size_t undoLength = 0;
	size_t length = 0;
};

size_t ModRmLength(const uint8_t* modRm)
{
	const auto mod = static_cast<uint8_t>(modRm[0] >> kModShift);
	const auto rm = static_cast<uint8_t>(modRm[0] & kRegisterMask);

	if (mod == kModRegister)
		return sizeof(uint8_t);

	const bool sib = rm == kSibFollows;
	const size_t addressing = sizeof(uint8_t) + (sib ? sizeof(uint8_t) : 0);

	if (mod == kModDisp8)
		return addressing + sizeof(uint8_t);
	if (mod == kModDisp32)
		return addressing + sizeof(uint32_t);

	const auto memoryBase = static_cast<uint8_t>(sib ? modRm[1] & kRegisterMask : rm);
	return mod == kModNoDisplacement && memoryBase == kFrameRegister ? addressing + sizeof(uint32_t) : addressing;
}

EntryStep Undone(const uint8_t* undo, size_t undoLength, size_t length)
{
	EntryStep step;
	std::memcpy(step.undo, undo, undoLength);
	step.undoLength = undoLength;
	step.length = length;
	return step;
}

EntryStep ReadEntryStep(const uint8_t* at)
{
	const auto pushed = static_cast<uint8_t>(at[0] & kRegisterMask);

	if ((at[0] & kOpcodeRegisterMask) == kPushRegister && pushed != kStackRegister)
	{
		const auto pop = static_cast<uint8_t>(kPopRegister | pushed);
		return Undone(&pop, sizeof(pop), kOpcodeLength);
	}

	if (std::memcmp(at, kMovEbpEsp, sizeof(kMovEbpEsp)) == 0)
		return Undone(kMovEspEbp, sizeof(kMovEspEbp), sizeof(kMovEbpEsp));

	if (std::memcmp(at, kSubEspByte, sizeof(kSubEspByte)) == 0)
	{
		const uint8_t add[kEspByteLength] = { kAddEspByte[0], kAddEspByte[1], at[sizeof(kSubEspByte)] };
		return Undone(add, sizeof(add), kEspByteLength);
	}

	if (std::memcmp(at, kSubEspDword, sizeof(kSubEspDword)) == 0)
	{
		uint8_t add[kEspDwordLength] = { kAddEspDword[0], kAddEspDword[1] };
		std::memcpy(add + sizeof(kAddEspDword), at + sizeof(kSubEspDword), sizeof(uint32_t));
		return Undone(add, sizeof(add), kEspDwordLength);
	}

	EntryStep neutral;
	const auto target = static_cast<uint8_t>((at[1] >> kRegisterShift) & kRegisterMask);

	if (at[0] == kMovRegister && target != kStackRegister && target != kFrameRegister)
		neutral.length = kOpcodeLength + ModRmLength(at + 1);

	return neutral;
}

size_t ReadEntry(const uint8_t* function, const uint8_t* from, EntryStep* steps)
{
	size_t count = 0;
	const uint8_t* at = function;

	while (at < from)
	{
		const EntryStep step = ReadEntryStep(at);

		if (step.length == 0 || (step.undoLength != 0 && count == kMostEntrySteps))
			return 0;

		if (step.undoLength != 0)
			steps[count++] = step;

		at += step.length;
	}

	return at == from ? count : 0;
}

bool UndoesBefore(const uint8_t* function, size_t length, size_t tail, const EntryStep& first)
{
	return length >= tail + first.undoLength &&
		std::memcmp(function + length - tail - first.undoLength, first.undo, first.undoLength) == 0;
}

size_t FinalReturnLength(const uint8_t* function, size_t length, const EntryStep& first)
{
	if (length >= kRetImmLength && function[length - kRetImmLength] == kRetImm &&
		UndoesBefore(function, length, kRetImmLength, first))
	{
		return kRetImmLength;
	}

	if (function[length - 1] == kRet && UndoesBefore(function, length, sizeof(kRet), first))
		return sizeof(kRet);

	return 0;
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
		{
			g_data = range;
			g_initialisedData = ImageSection{ range.begin, section->SizeOfRawData };
		}
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

ImageSection ImageScanner::InitialisedData()
{
	return g_initialisedData;
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

size_t ImageScanner::EarlyReturn(const uint8_t* function, const uint8_t* from, uint32_t result, uint8_t* out,
	size_t capacity)
{
	const size_t length = FunctionLength(function);

	if (length == 0 || from < function || from >= function + length)
		return 0;

	EntryStep steps[kMostEntrySteps];
	const size_t count = ReadEntry(function, from, steps);

	if (count == 0)
		return 0;

	const size_t returnLength = FinalReturnLength(function, length, steps[0]);
	size_t needed = kMovEaxImmLength + returnLength;

	for (size_t i = 0; i < count; ++i)
		needed += steps[i].undoLength;

	if (returnLength == 0 || needed > capacity)
		return 0;

	size_t written = 0;

	for (size_t i = count; i-- > 0;)
	{
		std::memcpy(out + written, steps[i].undo, steps[i].undoLength);
		written += steps[i].undoLength;
	}

	out[written] = kMovEaxImm;
	std::memcpy(out + written + sizeof(kMovEaxImm), &result, sizeof(result));
	written += kMovEaxImmLength;

	std::memcpy(out + written, function + length - returnLength, returnLength);
	return written + returnLength;
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
