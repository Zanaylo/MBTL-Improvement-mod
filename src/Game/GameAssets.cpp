#include "Game/GameAssets.h"

#include "Core/utils.h"
#include "Game/GameOffsets.h"
#include "Game/MemoryMap.h"

#include <cstring>

namespace {

namespace Files = GameOffsets::Files;

using ReaderCall_t = void*(__fastcall*)(void*, void*);
using ReaderOpen_t = int(__fastcall*)(void*, void*, const char*, int, int, int);
using ReaderLoad_t = int(__fastcall*)(void*, void*, int, int, int);

bool OpenAndLoad(const GameFunctions& functions, uint8_t* reader, const char* path)
{
	const auto open = reinterpret_cast<ReaderOpen_t>(functions.readerOpen);
	const auto load = reinterpret_cast<ReaderLoad_t>(functions.readerLoad);

	return open(reader, nullptr, path, Files::kOpenDecrypt, Files::kOpenShare, Files::kOpenFlags) != 0 &&
		load(reader, nullptr, 0, 0, 0) != 0;
}

bool CopyOut(const uint8_t* reader, std::vector<uint8_t>& out)
{
	uint32_t data = 0;
	uint32_t size = 0;

	std::memcpy(&data, reader + Files::kReaderData, sizeof(data));
	std::memcpy(&size, reader + Files::kReaderSize, sizeof(size));

	if (data == 0 || size == 0)
		return false;

	out.resize(size);
	return TryReadMemory(out.data(), reinterpret_cast<const void*>(static_cast<uintptr_t>(data)), size);
}

}

bool GameAssets::IsAvailable()
{
	const GameFunctions& functions = MemoryMap::Functions();

	return functions.readerCtor && functions.readerOpen && functions.readerLoad && functions.readerClose &&
		functions.readerDtor;
}

bool GameAssets::Read(const char* path, std::vector<uint8_t>& out)
{
	out.clear();

	if (path == nullptr || !IsAvailable())
		return false;

	const GameFunctions& functions = MemoryMap::Functions();
	uint8_t reader[Files::kReaderBytes] = {};

	reinterpret_cast<ReaderCall_t>(functions.readerCtor)(reader, nullptr);

	const bool copied = OpenAndLoad(functions, reader, path) && CopyOut(reader, out);

	reinterpret_cast<ReaderCall_t>(functions.readerClose)(reader, nullptr);
	reinterpret_cast<ReaderCall_t>(functions.readerDtor)(reader, nullptr);
	return copied;
}
