#include "Core/utils.h"

#include "Core/info.h"

#include <cstdio>
#include <cstring>

namespace {

HMODULE g_modModule = nullptr;

size_t ImageSize(uintptr_t base)
{
	const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
	if (dos->e_magic != IMAGE_DOS_SIGNATURE)
		return 0;

	const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
	if (nt->Signature != IMAGE_NT_SIGNATURE)
		return 0;

	return nt->OptionalHeader.SizeOfImage;
}

}

void SetModModuleHandle(HMODULE module)
{
	g_modModule = module;
}

HMODULE GetModModuleHandle()
{
	return g_modModule;
}

std::string GetGameDirectory()
{
	char path[MAX_PATH] = {};
	const DWORD length = GetModuleFileNameA(nullptr, path, MAX_PATH);
	const std::string full(path, length);
	const size_t slash = full.find_last_of("\\/");

	if (slash == std::string::npos)
		return ".";

	return full.substr(0, slash);
}

std::string GetModRootPath(const char* relative)
{
	std::string path = GetGameDirectory() + "\\" MBTL_IM_ROOT_FOLDER;

	if (relative && *relative)
		path += std::string("\\") + relative;

	return path;
}

std::string GetSystemDirectoryPath()
{
	char path[MAX_PATH] = {};
	const UINT length = GetSystemDirectoryA(path, MAX_PATH);

	if (length == 0 || length >= MAX_PATH)
		return {};

	return std::string(path, length) + "\\";
}

void CreateModDirectories()
{
	CreateDirectoryA(GetModRootPath().c_str(), nullptr);
	CreateDirectoryA(GetModRootPath("Logs").c_str(), nullptr);
	CreateDirectoryA(GetModRootPath("Mods").c_str(), nullptr);
	CreateDirectoryA(GetModRootPath("Packs").c_str(), nullptr);
}

bool CreateDirectoryTree(const std::string& folder)
{
	if (folder.empty())
		return false;

	for (size_t at = folder.find_first_of("\\/", 3); at != std::string::npos;
		at = folder.find_first_of("\\/", at + 1))
	{
		CreateDirectoryA(folder.substr(0, at).c_str(), nullptr);
	}

	CreateDirectoryA(folder.c_str(), nullptr);

	const DWORD attributes = GetFileAttributesA(folder.c_str());
	return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool ReadWholeFile(const std::string& path, std::vector<uint8_t>& out, size_t minimumSize)
{
	out.clear();

	FILE* handle = nullptr;
	if (fopen_s(&handle, path.c_str(), "rb") != 0 || handle == nullptr)
		return false;

	fseek(handle, 0, SEEK_END);
	const long bytes = ftell(handle);
	fseek(handle, 0, SEEK_SET);

	if (bytes <= 0 || static_cast<size_t>(bytes) < minimumSize)
	{
		fclose(handle);
		return false;
	}

	out.resize(static_cast<size_t>(bytes));
	const size_t read = fread(out.data(), 1, out.size(), handle);
	fclose(handle);

	if (read == out.size())
		return true;

	out.clear();
	return false;
}

bool WriteWholeFile(const std::string& path, const uint8_t* data, size_t size)
{
	FILE* handle = nullptr;
	if (fopen_s(&handle, path.c_str(), "wb") != 0 || handle == nullptr)
		return false;

	const size_t written = size == 0 ? 0 : fwrite(data, 1, size, handle);
	fclose(handle);

	return written == size;
}

bool WriteWholeFile(const std::string& path, const std::string& text)
{
	return WriteWholeFile(path, reinterpret_cast<const uint8_t*>(text.data()), text.size());
}

uint32_t ReadLittle32(const std::vector<uint8_t>& blob, size_t at)
{
	if (at + sizeof(uint32_t) > blob.size())
		return 0;

	uint32_t value = 0;
	std::memcpy(&value, blob.data() + at, sizeof(value));
	return value;
}

std::string ResourceFileName(const char* name)
{
	if (name == nullptr)
		return std::string();

	std::string text = name;

	if (text.size() >= 2 && text.front() == '"' && text.back() == '"')
		text = text.substr(1, text.size() - 2);

	for (char& character : text)
		character = static_cast<char>(tolower(static_cast<unsigned char>(character)));

	return text;
}

uintptr_t GetGameBaseAddress()
{
	return reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
}

size_t GetGameModuleSize()
{
	static size_t size = ImageSize(GetGameBaseAddress());
	return size;
}

bool IsInGameModule(uintptr_t address)
{
	const uintptr_t base = GetGameBaseAddress();
	return address >= base && address < base + GetGameModuleSize();
}

std::string DescribeAddress(uintptr_t address)
{
	char text[MAX_PATH + 32] = {};

	HMODULE module = nullptr;
	const DWORD flags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;

	if (!GetModuleHandleExA(flags, reinterpret_cast<LPCSTR>(address), &module))
	{
		sprintf_s(text, "0x%08X", static_cast<unsigned>(address));
		return text;
	}

	char path[MAX_PATH] = {};
	GetModuleFileNameA(module, path, MAX_PATH);
	const char* const slash = std::strrchr(path, '\\');

	sprintf_s(text, "%s+0x%X", slash ? slash + 1 : path,
		static_cast<unsigned>(address - reinterpret_cast<uintptr_t>(module)));
	return text;
}

bool TryWriteMemory(void* destination, const void* source, size_t size)
{
	return TryReadMemory(destination, source, size);
}

bool TryReadMemory(void* destination, const void* source, size_t size)
{
	if (destination == nullptr || source == nullptr)
		return false;

	__try
	{
		std::memcpy(destination, source, size);
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
}
