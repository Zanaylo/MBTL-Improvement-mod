#pragma once

#include <windows.h>

#include <cstdint>
#include <string>
#include <vector>

void SetModModuleHandle(HMODULE module);
HMODULE GetModModuleHandle();

std::string GetGameDirectory();
std::string GetModRootPath(const char* relative = "");
std::string GetSystemDirectoryPath();
void CreateModDirectories();

bool CreateDirectoryTree(const std::string& folder);
bool ReadWholeFile(const std::string& path, std::vector<uint8_t>& out, size_t minimumSize = 0);
bool WriteWholeFile(const std::string& path, const uint8_t* data, size_t size);
bool WriteWholeFile(const std::string& path, const std::string& text);
uint32_t ReadLittle32(const std::vector<uint8_t>& blob, size_t at);
std::string ResourceFileName(const char* name);

uintptr_t GetGameBaseAddress();
size_t GetGameModuleSize();
bool IsInGameModule(uintptr_t address);
std::string DescribeAddress(uintptr_t address);

bool TryReadMemory(void* destination, const void* source, size_t size);
bool TryWriteMemory(void* destination, const void* source, size_t size);

template <typename T>
bool TryRead(uintptr_t address, T& out)
{
	return TryReadMemory(&out, reinterpret_cast<const void*>(address), sizeof(T));
}

template <typename T>
bool TryWrite(uintptr_t address, const T& value)
{
	return TryWriteMemory(reinterpret_cast<void*>(address), &value, sizeof(T));
}
