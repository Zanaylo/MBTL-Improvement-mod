#pragma once

#include <cstdint>
#include <string>

namespace PaletteFile
{
	constexpr int kColors = 256;
	constexpr int kBytes = kColors * 4;
	constexpr int kSubPalettes = 4;

	constexpr int kNameLength = 32;
	constexpr int kCreatorLength = 32;
	constexpr int kDescriptionLength = 64;

	constexpr const char* kExtension = ".pal";

	struct Info
	{
		char name[kNameLength];
		char creator[kCreatorLength];
		char description[kDescriptionLength];
	};

	struct Content
	{
		uint8_t pages[kSubPalettes][kBytes];
		uint8_t subMask;
		uint8_t effects[kBytes];
		bool hasEffects;
		Info info;
	};

	bool Load(const std::string& path, Content& out);
	bool Save(const std::string& path, const Content& content);
}
