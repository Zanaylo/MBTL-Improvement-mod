#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace StageImage
{
	struct Bitmap
	{
		int width = 0;
		int height = 0;
		std::vector<uint8_t> bgra;
	};

	std::string Find(int number, const char* name);
	bool Load(const std::string& path, Bitmap& out);

	Bitmap Cover(const Bitmap& image, int width, int height);
	void Blur(Bitmap& image, int radius);
	void Brighten(Bitmap& image, int percent);
}
