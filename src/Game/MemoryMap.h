#pragma once

#include <cstdint>

struct GameFunctions
{
	uint8_t* readerOpen = nullptr;
	uint8_t* readerFromMemory = nullptr;
	uint8_t* readerLoad = nullptr;
	uint8_t* fileExists = nullptr;
	uint8_t* readerCtor = nullptr;
	uint8_t* readerClose = nullptr;
	uint8_t* readerDtor = nullptr;
};

namespace MemoryMap
{
	bool Initialize();
	const GameFunctions& Functions();
	const char* Status();
}
