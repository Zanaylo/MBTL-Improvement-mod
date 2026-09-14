#include "Hooks/CodePatch.h"

#include <windows.h>

#include <cstring>

bool CodePatch::Write(uint8_t* at, const void* bytes, size_t size)
{
	DWORD previous = 0;

	if (!VirtualProtect(at, size, PAGE_EXECUTE_READWRITE, &previous))
		return false;

	std::memcpy(at, bytes, size);
	VirtualProtect(at, size, previous, &previous);
	FlushInstructionCache(GetCurrentProcess(), at, size);
	return true;
}
