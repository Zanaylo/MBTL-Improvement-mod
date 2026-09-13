#include "Music/MusicIni.h"

#include "Core/utils.h"

#include <windows.h>

#include <cstdio>
#include <cstring>

namespace {

constexpr const char* kFolder = "Music";
constexpr const char* kFile = "Music\\music.ini";
constexpr size_t kFirstBytes = 8192;
constexpr size_t kTerminatorBytes = 2;

std::string g_path;

std::vector<char> SectionBytes(const char* section)
{
	std::vector<char> buffer(kFirstBytes, '\0');

	for (;;)
	{
		const DWORD read = GetPrivateProfileSectionA(section, buffer.data(), static_cast<DWORD>(buffer.size()),
			MusicIni::Path().c_str());

		if (read < buffer.size() - kTerminatorBytes)
			return buffer;

		buffer.assign(buffer.size() * 2, '\0');
	}
}

}

const std::string& MusicIni::Path()
{
	if (!g_path.empty())
		return g_path;

	CreateDirectoryTree(GetModRootPath(kFolder));
	g_path = GetModRootPath(kFile);
	return g_path;
}

std::string MusicIni::IdKey(int id)
{
	char key[16] = {};
	sprintf_s(key, "%03d", id);
	return key;
}

std::vector<MusicIni::Entry> MusicIni::ReadSection(const char* section)
{
	const std::vector<char> buffer = SectionBytes(section);
	std::vector<Entry> entries;

	for (const char* line = buffer.data(); *line != '\0'; line += std::strlen(line) + 1)
	{
		const char* const separator = std::strchr(line, '=');

		if (separator == nullptr || separator == line)
			continue;

		entries.emplace_back(std::string(line, separator), std::string(separator + 1));
	}

	return entries;
}

void MusicIni::WriteSection(const char* section, const std::vector<Entry>& entries)
{
	WritePrivateProfileStringA(section, nullptr, nullptr, Path().c_str());

	if (entries.empty())
		return;

	std::string block;

	for (const Entry& entry : entries)
	{
		block += entry.first;
		block.push_back('=');
		block += entry.second;
		block.push_back('\0');
	}

	block.push_back('\0');
	WritePrivateProfileSectionA(section, block.data(), Path().c_str());
}

int MusicIni::ReadInt(const char* section, const char* key, int fallback)
{
	return static_cast<int>(GetPrivateProfileIntA(section, key, fallback, Path().c_str()));
}

void MusicIni::WriteInt(const char* section, const char* key, int value)
{
	WritePrivateProfileStringA(section, key, std::to_string(value).c_str(), Path().c_str());
}
