#include "Game/MemoryMap.h"

#include "Core/logger.h"
#include "Game/Anchors.h"
#include "Game/GameOffsets.h"
#include "Hooks/ImageScanner.h"

#include <algorithm>
#include <cstring>
#include <map>
#include <vector>

namespace {

namespace Files = GameOffsets::Files;

GameFunctions g_functions;
char g_status[160] = "not initialized";

uintptr_t AddressOf(const uint8_t* address)
{
	return reinterpret_cast<uintptr_t>(address);
}

bool StartsWith(const uint8_t* address, const uint8_t* bytes, size_t length)
{
	return ImageScanner::InCode(address, length) && std::memcmp(address, bytes, length) == 0;
}

bool ReferencesAny(const uint8_t* start, size_t length, const std::vector<uint8_t*>& targets)
{
	for (size_t i = 0; i + 4 <= length; ++i)
	{
		const uint32_t value = ImageScanner::ReadDword(start + i);

		for (const uint8_t* target : targets)
		{
			if (value == static_cast<uint32_t>(AddressOf(target)))
				return true;
		}
	}

	return false;
}

bool SetsReaderType(const uint8_t* start, size_t length, uint32_t type)
{
	for (size_t i = 0; i + 7 <= length; ++i)
	{
		if (start[i] != 0xC7 || (start[i + 1] & 0xF8) != 0x40 || start[i + 2] != Files::kReaderTypeOffset)
			continue;

		if (std::memcmp(start + i + 3, &type, sizeof(type)) == 0)
			return true;
	}

	return false;
}

uint8_t* ResolveReaderOpen()
{
	const auto functions = ImageScanner::FunctionsReferencing(ImageScanner::FindString(Files::kReaderOpenAnchor));
	if (functions.size() != 1)
	{
		LOG("Reader open: %u candidate function(s), expected exactly one", static_cast<unsigned>(functions.size()));
		return nullptr;
	}

	if (!StartsWith(functions[0], Files::kReaderOpenPrologue, sizeof(Files::kReaderOpenPrologue)))
	{
		LOG("Reader open: unexpected first bytes, refusing to hook it");
		return nullptr;
	}

	return functions[0];
}

uint8_t* ResolveReaderFromMemory()
{
	const auto sizeStrings = ImageScanner::FindWideString(Files::kFileSizeAssert);
	const auto candidates = ImageScanner::FunctionsReferencing(ImageScanner::FindWideString(Files::kFileBufferAssert));

	std::vector<uint8_t*> matches;
	for (uint8_t* start : candidates)
	{
		const size_t length = ImageScanner::FunctionLength(start);
		if (length == 0 || length > Files::kMaxReaderFromMemoryLength)
			continue;
		if (!SetsReaderType(start, length, Files::kReaderTypeMemory) || !ReferencesAny(start, length, sizeStrings))
			continue;

		matches.push_back(start);
	}

	if (matches.size() != 1)
	{
		LOG("Memory reader: %u match(es) among %u candidate(s), expected exactly one",
			static_cast<unsigned>(matches.size()), static_cast<unsigned>(candidates.size()));
		return nullptr;
	}

	return matches[0];
}

uint8_t* CallTargetAt(const uint8_t* at)
{
	if (!ImageScanner::InCode(at, 5) || at[0] != 0xE8)
		return nullptr;

	const auto target = const_cast<uint8_t*>(at + 5 + static_cast<int32_t>(ImageScanner::ReadDword(at + 1)));
	return ImageScanner::InCode(target, 1) ? target : nullptr;
}

uint8_t* FirstCallAfter(uint8_t* site)
{
	for (size_t i = 5; i < Files::kLoadCallWindow; ++i)
	{
		uint8_t* const target = CallTargetAt(site + i);

		if (target)
			return target;
	}

	return nullptr;
}

uint8_t* MostVoted(const std::map<uint8_t*, int>& votes, int least, const char* what)
{
	const auto best = std::max_element(votes.begin(), votes.end(),
		[](const std::pair<uint8_t* const, int>& a, const std::pair<uint8_t* const, int>& b) { return a.second < b.second; });

	if (best != votes.end() && best->second >= least)
		return best->first;

	LOG("%s: no consistent candidate (%d)", what, best == votes.end() ? 0 : best->second);
	return nullptr;
}

uint8_t* ResolveReaderClose(const uint8_t* readerOpen)
{
	if (!readerOpen)
		return nullptr;

	const size_t length = ImageScanner::FunctionLength(readerOpen);
	size_t seen = 0;

	for (size_t i = 0; i + 5 <= length; ++i)
	{
		uint8_t* const target = CallTargetAt(readerOpen + i);

		if (!target || seen++ != Files::kCloseCallIndex)
			continue;

		const bool loadsReader = i >= Files::kLoadEcxFromFrameLength &&
			std::memcmp(readerOpen + i - Files::kLoadEcxFromFrameLength, Files::kLoadEcxFromFrame, sizeof(Files::kLoadEcxFromFrame)) == 0;

		if (loadsReader && SetsReaderType(target, ImageScanner::FunctionLength(target), Files::kReaderTypeClosed))
			return target;

		break;
	}

	LOG("Reader close: the second call in reader open is not the close");
	return nullptr;
}

size_t FrameLeaBefore(const uint8_t* site)
{
	if (std::memcmp(site - Files::kFarFrameLeaLength, Files::kFarFrameLea, sizeof(Files::kFarFrameLea)) == 0)
		return Files::kFarFrameLeaLength;

	if (std::memcmp(site - Files::kNearFrameLeaLength, Files::kNearFrameLea, sizeof(Files::kNearFrameLea)) == 0)
		return Files::kNearFrameLeaLength;

	return 0;
}

void VoteForCtor(uint8_t* openSite, const uint8_t* readerClose, std::map<uint8_t*, int>& votes)
{
	const size_t leaLength = FrameLeaBefore(openSite);
	const uint8_t* const start = leaLength ? ImageScanner::FunctionStart(openSite) : nullptr;

	if (!start)
		return;

	const uint8_t* const lea = openSite - leaLength;

	for (const uint8_t* at = start + leaLength; at < openSite; ++at)
	{
		uint8_t* const target = CallTargetAt(at);

		if (!target || target == readerClose || std::memcmp(at - leaLength, lea, leaLength) != 0)
			continue;

		++votes[target];
	}
}

uint8_t* ResolveReaderCtor(const uint8_t* readerOpen, const uint8_t* readerClose)
{
	if (!readerOpen || !readerClose)
		return nullptr;

	std::map<uint8_t*, int> votes;

	for (uint8_t* site : ImageScanner::CallersOf(readerOpen))
		VoteForCtor(site, readerClose, votes);

	uint8_t* const ctor = MostVoted(votes, Files::kLeastCtorVotes, "Reader constructor");

	if (!ctor || !StartsWith(ctor, Files::kCtorPrologue, sizeof(Files::kCtorPrologue)))
		return nullptr;

	return SetsReaderType(ctor, ImageScanner::FunctionLength(ctor), Files::kReaderTypeClosed) ? ctor : nullptr;
}

uint8_t* ResolveReaderDtor(const uint8_t* readerClose)
{
	if (!readerClose)
		return nullptr;

	std::vector<uint8_t*> matches;

	for (uint8_t* function : ImageScanner::CallerFunctions(readerClose))
	{
		const size_t length = ImageScanner::FunctionLength(function);

		if (length == 0 || length > Files::kDtorMaxLength || ImageScanner::CallSequence(function).size() != 1)
			continue;

		if (ImageScanner::CallersOf(function).size() >= Files::kLeastDtorCallers)
			matches.push_back(function);
	}

	if (matches.size() == 1)
		return matches.front();

	LOG("Reader destructor: %u match(es), expected exactly one", static_cast<unsigned>(matches.size()));
	return nullptr;
}

uint8_t* ResolveReaderLoad(uint8_t* readerOpen)
{
	if (!readerOpen)
		return nullptr;

	std::map<uint8_t*, int> counts;

	for (uint8_t* site : ImageScanner::CallersOf(readerOpen))
	{
		uint8_t* const next = FirstCallAfter(site);
		if (next)
			++counts[next];
	}

	return MostVoted(counts, Files::kLeastLoadCallers, "Reader load");
}

uint8_t* ResolveFileExists()
{
	const auto objectLoaders = ImageScanner::FunctionsReferencing(ImageScanner::FindString(Files::kObjectListAnchor));
	const auto stageLists = ImageScanner::FunctionsReferencing(ImageScanner::FindString(Files::kStageListAnchor));
	if (objectLoaders.size() != 1 || stageLists.size() != 1)
	{
		LOG("File exists: %u object loader(s) and %u stage list loader(s), expected one of each",
			static_cast<unsigned>(objectLoaders.size()), static_cast<unsigned>(stageLists.size()));
		return nullptr;
	}

	const auto fromStageList = ImageScanner::CallTargets(stageLists[0]);
	std::vector<uint8_t*> matches;

	for (uint8_t* target : ImageScanner::CallTargets(objectLoaders[0]))
	{
		if (std::find(fromStageList.begin(), fromStageList.end(), target) == fromStageList.end())
			continue;
		if (!StartsWith(target, Files::kFileExistsPrologue, sizeof(Files::kFileExistsPrologue)))
			continue;
		if (!ImageScanner::CallsImport(target, Files::kKernelLibrary, Files::kFileAttributesImport))
			continue;

		matches.push_back(target);
	}

	if (matches.size() != 1)
	{
		LOG("File exists: %u match(es), expected exactly one", static_cast<unsigned>(matches.size()));
		return nullptr;
	}

	return matches[0];
}

void SetStatus(const char* status)
{
	strncpy_s(g_status, status, _TRUNCATE);
	LOG("MemoryMap: %s", g_status);
}

}

bool MemoryMap::Initialize()
{
	if (!ImageScanner::Initialize())
	{
		SetStatus("could not read MBTL.exe's headers");
		return false;
	}

	LOG("MBTL.exe build stamp 0x%08X", ImageScanner::TimeDateStamp());

	g_functions.readerOpen = ResolveReaderOpen();
	g_functions.readerFromMemory = ResolveReaderFromMemory();
	g_functions.readerLoad = ResolveReaderLoad(g_functions.readerOpen);
	g_functions.fileExists = ResolveFileExists();
	g_functions.readerClose = ResolveReaderClose(g_functions.readerOpen);
	g_functions.readerCtor = ResolveReaderCtor(g_functions.readerOpen, g_functions.readerClose);
	g_functions.readerDtor = ResolveReaderDtor(g_functions.readerClose);

	Anchors::Record("Reader open", AddressOf(g_functions.readerOpen));
	Anchors::Record("Reader constructor", AddressOf(g_functions.readerCtor), "reading the game's own font");
	Anchors::Record("Reader close", AddressOf(g_functions.readerClose), "reading the game's own font");
	Anchors::Record("Reader destructor", AddressOf(g_functions.readerDtor), "reading the game's own font");
	Anchors::Record("Reader from memory", AddressOf(g_functions.readerFromMemory));
	Anchors::Record("Reader load", AddressOf(g_functions.readerLoad), "needed to extend the game's own tables");
	Anchors::Record("File exists", AddressOf(g_functions.fileExists), "needed for new textures");

	if (!g_functions.readerOpen || !g_functions.readerFromMemory)
	{
		SetStatus("the game's file reader was not found, so Mods cannot load on this game version");
		return false;
	}

	if (!g_functions.fileExists)
	{
		SetStatus("ready, but without the file exists check new textures and object.txt will not load");
		return true;
	}

	SetStatus(g_functions.readerLoad ? "ready" : "ready, but the game's own tables cannot be extended");
	return true;
}

const GameFunctions& MemoryMap::Functions()
{
	return g_functions;
}

const char* MemoryMap::Status()
{
	return g_status;
}
