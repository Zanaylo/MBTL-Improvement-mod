#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

struct ImageSection
{
	uint8_t* begin = nullptr;
	size_t size = 0;
};

namespace ImageScanner
{
	bool Initialize();
	void ReleaseCallIndex();

	uint8_t* Base();
	ImageSection Code();
	ImageSection ReadOnlyData();
	uint32_t TimeDateStamp();

	bool InCode(const uint8_t* address, size_t length);
	bool InData(uintptr_t address);
	uint32_t ReadDword(const uint8_t* address);

	std::vector<uint8_t*> FindBytes(ImageSection where, const uint8_t* bytes, size_t length);
	std::vector<uint8_t*> FindString(const char* text);
	std::vector<uint8_t*> FindWideString(const wchar_t* text);
	std::vector<uint8_t*> FindReferencesTo(const uint8_t* address);

	uint8_t* FunctionStart(uint8_t* inside);
	size_t FunctionLength(const uint8_t* start);
	std::vector<uint8_t*> CallSequence(const uint8_t* function);
	std::vector<uint8_t*> CallTargets(const uint8_t* function);
	std::vector<uint8_t*> CallsCleanedBy(const uint8_t* function, const uint8_t* cleanup, size_t count);
	uint8_t* CallTargetOf(const uint8_t* site);
	std::vector<uint8_t*> CallersOf(const uint8_t* target);
	std::vector<uint8_t*> CallerFunctions(const uint8_t* target);
	std::vector<uint8_t*> FunctionsReferencing(const std::vector<uint8_t*>& addresses);

	uint8_t* NativeFunction(const char* bindingName);
	uintptr_t GetterValue(const uint8_t* function);
	uintptr_t MemoryGetterValue(const uint8_t* function);
	uintptr_t ByteGetterValue(const uint8_t* function);

	uint8_t* ImportSlot(const char* library, const char* function);
	bool Contains(const uint8_t* start, size_t length, const uint8_t* bytes, size_t count);
	const uint8_t* AfterPushOf(const uint8_t* function, size_t length, const uint8_t* value);
	bool ReturnsWith(const uint8_t* function, uint16_t stackBytes);
	uint8_t* Epilogue(uint8_t* function);
	bool CallsImport(const uint8_t* function, const char* library, const char* name);
}
