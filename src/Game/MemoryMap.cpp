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

uint8_t* FirstCall(const uint8_t* function)
{
	const std::vector<uint8_t*> calls = ImageScanner::CallSequence(function);
	return calls.empty() ? nullptr : calls.front();
}

bool SetsReaderType(const uint8_t* start, size_t length, uint32_t type)
{
	for (size_t i = 0; i + 7 <= length; ++i)
	{
		if (start[i] != Files::kStoreDword || (start[i + 1] & 0xF8) != 0x40 || start[i + 2] != Files::kReaderTypeOffset)
			continue;

		if (std::memcmp(start + i + 3, &type, sizeof(type)) == 0)
			return true;
	}

	return false;
}

std::vector<uint8_t*> FunctionsSettingReaderType(uint32_t type)
{
	std::vector<uint8_t*> functions;
	const ImageSection code = ImageScanner::Code();

	uint8_t pattern[7] = { Files::kStoreDword, 0x40, Files::kReaderTypeOffset };
	std::memcpy(pattern + 3, &type, sizeof(type));

	for (uint8_t slot = 0x40; slot < 0x48; ++slot)
	{
		pattern[1] = slot;

		for (uint8_t* site : ImageScanner::FindBytes(code, pattern, sizeof(pattern)))
		{
			uint8_t* const function = ImageScanner::FunctionStart(site);

			if (function && std::find(functions.begin(), functions.end(), function) == functions.end())
				functions.push_back(function);
		}
	}

	return functions;
}

uint8_t* ResolveReaderOpen()
{
	const auto functions = ImageScanner::FunctionsReferencing(ImageScanner::FindString(Files::kReaderOpenAnchor));
	if (functions.size() != 1)
	{
		LOG("Reader open: %u candidate function(s), expected exactly one", static_cast<unsigned>(functions.size()));
		return nullptr;
	}

	if (!ImageScanner::ReturnsWith(functions[0], Files::kReaderOpenStackBytes))
	{
		LOG("Reader open: it no longer takes four arguments, refusing to hook it");
		return nullptr;
	}

	return functions[0];
}

uint8_t* ResolveReaderClose(const uint8_t* readerOpen)
{
	if (!readerOpen)
		return nullptr;

	uint8_t* const close = FirstCall(readerOpen);

	if (close && SetsReaderType(close, ImageScanner::FunctionLength(close), Files::kReaderTypeClosed))
		return close;

	LOG("Reader close: the first call in reader open does not close the reader");
	return nullptr;
}

uint8_t* ResolveReaderFromMemory(const uint8_t* readerClose)
{
	if (!readerClose)
		return nullptr;

	std::vector<uint8_t*> matches;

	for (uint8_t* function : FunctionsSettingReaderType(Files::kReaderTypeMemory))
	{
		const size_t length = ImageScanner::FunctionLength(function);

		if (length == 0 || length > Files::kMaxReaderFromMemoryLength)
			continue;
		if (!ImageScanner::ReturnsWith(function, Files::kReaderFromMemoryStackBytes))
			continue;
		if (FirstCall(function) != readerClose)
			continue;

		matches.push_back(function);
	}

	if (matches.size() != 1)
	{
		LOG("Memory reader: %u match(es), expected exactly one", static_cast<unsigned>(matches.size()));
		return nullptr;
	}

	return matches[0];
}

uint8_t* ResolveReaderCtor(const uint8_t* readerClose)
{
	if (!readerClose)
		return nullptr;

	std::vector<uint8_t*> matches;

	for (uint8_t* function : FunctionsSettingReaderType(Files::kReaderTypeClosed))
	{
		const size_t length = ImageScanner::FunctionLength(function);

		if (function == readerClose || length == 0 || length > Files::kMaxReaderFromMemoryLength)
			continue;
		if (ImageScanner::CallersOf(function).size() < static_cast<size_t>(Files::kLeastCtorCallers))
			continue;

		matches.push_back(function);
	}

	if (matches.size() != 1)
	{
		LOG("Reader constructor: %u match(es), expected exactly one", static_cast<unsigned>(matches.size()));
		return nullptr;
	}

	return matches[0];
}

uint8_t* CallTargetAt(const uint8_t* at)
{
	if (!ImageScanner::InCode(at, Files::kCallLength) || at[0] != Files::kCall)
		return nullptr;

	const auto target = const_cast<uint8_t*>(at + Files::kCallLength + static_cast<int32_t>(ImageScanner::ReadDword(at + 1)));
	return ImageScanner::InCode(target, 1) ? target : nullptr;
}

uint8_t* FirstCallAfter(uint8_t* site)
{
	for (size_t i = Files::kCallLength; i < Files::kLoadCallWindow; ++i)
	{
		uint8_t* const target = CallTargetAt(site + i);

		if (target)
			return target;
	}

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

	const auto best = std::max_element(counts.begin(), counts.end(),
		[](const std::pair<uint8_t* const, int>& a, const std::pair<uint8_t* const, int>& b) { return a.second < b.second; });

	if (best != counts.end() && best->second >= Files::kLeastLoadCallers)
		return best->first;

	LOG("Reader load: no consistent candidate (%d)", best == counts.end() ? 0 : best->second);
	return nullptr;
}

uint8_t* ResolveFileExists(const uint8_t* readerOpen)
{
	if (!readerOpen)
		return nullptr;

	const auto probers = ImageScanner::FunctionsReferencing(ImageScanner::FindString(Files::kTextureExtensionAnchor));
	const auto alsoProbers = ImageScanner::FunctionsReferencing(ImageScanner::FindString(Files::kTextureExtensionCheck));

	if (probers.size() != 1 || probers != alsoProbers)
	{
		LOG("File exists: %u texture loader(s), expected exactly one", static_cast<unsigned>(probers.size()));
		return nullptr;
	}

	const std::vector<uint8_t*> fromOpen = ImageScanner::CallTargets(readerOpen);
	std::vector<uint8_t*> matches;

	for (uint8_t* target : ImageScanner::CallTargets(probers[0]))
	{
		if (std::find(fromOpen.begin(), fromOpen.end(), target) == fromOpen.end())
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
	g_functions.readerClose = ResolveReaderClose(g_functions.readerOpen);
	g_functions.readerFromMemory = ResolveReaderFromMemory(g_functions.readerClose);
	g_functions.readerLoad = ResolveReaderLoad(g_functions.readerOpen);
	g_functions.fileExists = ResolveFileExists(g_functions.readerOpen);
	g_functions.readerCtor = ResolveReaderCtor(g_functions.readerClose);

	Anchors::Record("Reader open", AddressOf(g_functions.readerOpen));
	Anchors::Record("Reader constructor", AddressOf(g_functions.readerCtor), "reading the game's own font");
	Anchors::Record("Reader close", AddressOf(g_functions.readerClose), "reading the game's own font");
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
