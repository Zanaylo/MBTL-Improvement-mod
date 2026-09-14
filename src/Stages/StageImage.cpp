#include "Stages/StageImage.h"

#include "Core/DdsImage.h"
#include "Core/PngImage.h"
#include "Core/logger.h"
#include "Core/utils.h"
#include "Stages/StageLibrary.h"

#include <windows.h>

#include <algorithm>

namespace {

using Bitmap = StageImage::Bitmap;

constexpr const char* kExtensions[] = { ".png", ".dds" };
constexpr size_t kChannels = 4;
constexpr size_t kColourChannels = 3;
constexpr int kFullPercent = 100;
constexpr int kBrightest = 255;

size_t At(int width, int x, int y)
{
	return (static_cast<size_t>(y) * width + x) * kChannels;
}

void Average(const Bitmap& image, int left, int top, int right, int bottom, uint8_t* out)
{
	unsigned totals[kChannels] = {};
	unsigned count = 0;

	for (int y = top; y < bottom; ++y)
	{
		for (int x = left; x < right; ++x)
		{
			for (size_t channel = 0; channel < kChannels; ++channel)
				totals[channel] += image.bgra[At(image.width, x, y) + channel];

			++count;
		}
	}

	for (size_t channel = 0; channel < kChannels; ++channel)
		out[channel] = static_cast<uint8_t>(totals[channel] / count);
}

void Smooth(const std::vector<uint8_t>& in, int width, int height, int x, int y, int radius, int stepX, int stepY,
	uint8_t* out)
{
	unsigned totals[kChannels] = {};
	unsigned count = 0;

	for (int step = -radius; step <= radius; ++step)
	{
		const int sampleX = x + step * stepX;
		const int sampleY = y + step * stepY;

		if (sampleX < 0 || sampleY < 0 || sampleX >= width || sampleY >= height)
			continue;

		for (size_t channel = 0; channel < kChannels; ++channel)
			totals[channel] += in[At(width, sampleX, sampleY) + channel];

		++count;
	}

	for (size_t channel = 0; channel < kChannels; ++channel)
		out[channel] = static_cast<uint8_t>(totals[channel] / count);
}

void BlurPass(const std::vector<uint8_t>& in, std::vector<uint8_t>& out, int width, int height, int radius, int stepX,
	int stepY)
{
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
			Smooth(in, width, height, x, y, radius, stepX, stepY, &out[At(width, x, y)]);
	}
}

}

std::string StageImage::Find(int number, const char* name)
{
	const std::string stem = StageLibrary::FolderOf(number) + "\\" + name;

	for (const char* extension : kExtensions)
	{
		const std::string path = stem + extension;

		if (GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES)
			return path;
	}

	return std::string();
}

bool StageImage::Load(const std::string& path, Bitmap& out)
{
	std::vector<uint8_t> blob;

	if (ReadWholeFile(path, blob) &&
		(PngImage::Decode(blob, out.width, out.height, out.bgra) || DdsImage::Decode(blob, out.width, out.height, out.bgra)))
	{
		return true;
	}

	LOG("StageImage: %s is not a PNG or DDS the mod can read", path.c_str());
	return false;
}

Bitmap StageImage::Cover(const Bitmap& image, int width, int height)
{
	const bool wide = static_cast<int64_t>(image.width) * height > static_cast<int64_t>(image.height) * width;
	const int sourceWidth = wide ? (std::max)(static_cast<int>(static_cast<int64_t>(image.height) * width / height), 1)
		: image.width;
	const int sourceHeight = wide ? image.height
		: (std::max)(static_cast<int>(static_cast<int64_t>(image.width) * height / width), 1);
	const int sourceX = (image.width - sourceWidth) / 2;
	const int sourceY = (image.height - sourceHeight) / 2;

	Bitmap out;
	out.width = width;
	out.height = height;
	out.bgra.assign(static_cast<size_t>(width) * height * kChannels, 0);

	for (int row = 0; row < height; ++row)
	{
		const int top = sourceY + static_cast<int>(static_cast<int64_t>(row) * sourceHeight / height);
		const int bottom = (std::max)(sourceY + static_cast<int>(static_cast<int64_t>(row + 1) * sourceHeight / height), top + 1);

		for (int column = 0; column < width; ++column)
		{
			const int left = sourceX + static_cast<int>(static_cast<int64_t>(column) * sourceWidth / width);
			const int right = (std::max)(sourceX + static_cast<int>(static_cast<int64_t>(column + 1) * sourceWidth / width),
				left + 1);

			Average(image, left, top, right, bottom, &out.bgra[At(width, column, row)]);
		}
	}

	return out;
}

void StageImage::Blur(Bitmap& image, int radius)
{
	std::vector<uint8_t> across(image.bgra.size());

	BlurPass(image.bgra, across, image.width, image.height, radius, 1, 0);
	BlurPass(across, image.bgra, image.width, image.height, radius, 0, 1);
}

void StageImage::Brighten(Bitmap& image, int percent)
{
	for (size_t at = 0; at < image.bgra.size(); at += kChannels)
	{
		for (size_t channel = 0; channel < kColourChannels; ++channel)
			image.bgra[at + channel] = static_cast<uint8_t>((std::min)(image.bgra[at + channel] * percent / kFullPercent, kBrightest));
	}
}
