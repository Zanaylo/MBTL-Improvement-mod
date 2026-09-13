#pragma once

#include <string>
#include <utility>
#include <vector>

namespace MusicIni
{
	using Entry = std::pair<std::string, std::string>;

	const std::string& Path();
	std::string IdKey(int id);

	std::vector<Entry> ReadSection(const char* section);
	void WriteSection(const char* section, const std::vector<Entry>& entries);

	int ReadInt(const char* section, const char* key, int fallback);
	void WriteInt(const char* section, const char* key, int value);
}
