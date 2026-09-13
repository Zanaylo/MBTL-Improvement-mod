#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace PngPalette
{
	constexpr int kEntries = 256;
	constexpr size_t kBytes = kEntries * 4;

	bool Read(const std::string& path, uint8_t* outRgba, std::string& outError);
	bool Write(const std::string& path, const uint8_t* rgba, std::string& outError);
	bool Recolour(const std::string& path, const uint8_t* basePng, size_t baseSize, const uint8_t* rgba,
		std::string& outError);
}
