#include "Stages/StagePictures.h"

#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Game/FileOverlay.h"
#include "Game/GameOffsets.h"
#include "Game/ModFiles.h"
#include "Stages/StageImage.h"
#include "Stages/StageRevision.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <string>
#include <vector>

namespace {

namespace Pictures = GameOffsets::Pictures;

constexpr size_t kChannels = 4;
constexpr size_t kColourChannels = 3;
constexpr int kFullPercent = 100;

struct Region
{
	int x;
	int y;
	int width;
	int height;
	int blur;
	int brightness;
};

struct Screen
{
	const char* prefix;
	const char* picture;
	int side;
	const Region* regions;
	size_t count;
};

constexpr Region kVsRegions[] = {
	{ 0, 0, 1024, 512, 0, kFullPercent },
	{ 0, 512, 1024, 512, 4, kFullPercent },
};

constexpr Region kMenuRegions[] = {
	{ 0, 0, 1280, 720, 0, kFullPercent },
	{ 0, 1024, 1280, 720, 1, 124 },
};

constexpr Screen kScreens[] = {
	{ Pictures::kVsPrefix, "vs_background", Pictures::kVsSide, kVsRegions, std::size(kVsRegions) },
	{ Pictures::kMenuPrefix, "menu_background", Pictures::kMenuSide, kMenuRegions, std::size(kMenuRegions) },
};

std::atomic<int> g_lent{ 0 };
std::atomic<int> g_painted{ 0 };

int NumberOf(const std::string& key, const char* prefix)
{
	const size_t length = strlen(prefix);

	if (key.compare(0, length, prefix) != 0)
		return -1;

	size_t at = length;

	while (at < key.size() && isdigit(static_cast<unsigned char>(key[at])) != 0)
		++at;

	if (at == length || key.compare(at, std::string::npos, Pictures::kSuffix) != 0)
		return -1;

	return atoi(key.c_str() + length);
}

const Screen* ScreenOf(const std::string& key, int& number)
{
	for (const Screen& screen : kScreens)
	{
		number = NumberOf(key, screen.prefix);

		if (number > 0)
			return &screen;
	}

	return nullptr;
}

std::string WithNumber(const char* requested, int number)
{
	std::string path = requested;
	const size_t dot = path.rfind('.');
	size_t digits = dot;

	while (digits != std::string::npos && digits > 0 && isdigit(static_cast<unsigned char>(path[digits - 1])) != 0)
		--digits;

	if (dot == std::string::npos || digits == dot)
		return path;

	char text[16] = {};
	sprintf_s(text, "%02d", number);

	path.replace(digits, dot - digits, text);
	return path;
}

uint32_t Read32(const std::vector<uint8_t>& data, size_t at)
{
	uint32_t value = 0;
	std::memcpy(&value, data.data() + at, sizeof(value));

	return value;
}

uint8_t* TexturePixels(std::vector<uint8_t>& content, int side)
{
	const auto tag = std::search(content.begin(), content.end(), std::begin(Pictures::kTextureTag), std::end(Pictures::kTextureTag));
	const size_t at = static_cast<size_t>(tag - content.begin());
	const size_t bytes = static_cast<size_t>(side) * side * kChannels;
	const size_t pixels = at + Pictures::kTextureDdsAt + Pictures::kDdsHeader;

	if (tag == content.end() || pixels + bytes > content.size())
		return nullptr;

	const bool valid = Read32(content, at + Pictures::kTextureBytesAt) == bytes + Pictures::kDdsHeader &&
		Read32(content, at + Pictures::kTextureWidthAt) == static_cast<uint32_t>(side) &&
		Read32(content, at + Pictures::kTextureHeightAt) == static_cast<uint32_t>(side) &&
		Read32(content, at + Pictures::kTextureFormatAt) == Pictures::kFormatBgra &&
		std::memcmp(content.data() + at + Pictures::kTextureDdsAt, Pictures::kDdsMagic, sizeof(Pictures::kDdsMagic)) == 0;

	return valid ? content.data() + pixels : nullptr;
}

void Paint(uint8_t* pixels, int side, const StageImage::Bitmap& image, const Region& region)
{
	StageImage::Bitmap picture = StageImage::Cover(image, region.width, region.height);

	if (region.blur > 0)
		StageImage::Blur(picture, region.blur);

	if (region.brightness != kFullPercent)
		StageImage::Brighten(picture, region.brightness);

	for (int row = 0; row < region.height; ++row)
	{
		uint8_t* const line = pixels + (static_cast<size_t>(region.y + row) * side + region.x) * kChannels;
		const uint8_t* const source = &picture.bgra[static_cast<size_t>(row) * region.width * kChannels];

		for (int column = 0; column < region.width; ++column)
			std::memcpy(line + column * kChannels, source + column * kChannels, kColourChannels);
	}
}

class PictureOverlay : public IFileOverlay
{
public:
	bool Covers(const std::string& key) const override
	{
		int number = 0;

		return ScreenOf(key, number) != nullptr;
	}

	std::string Redirect(const std::string& key, const char* requested) const override
	{
		int number = 0;
		const Screen* const screen = ScreenOf(key, number);
		const int chosen = g_settings.menuBackgroundStage;

		if (screen == nullptr || screen->prefix != Pictures::kMenuPrefix || chosen <= 0 || chosen == number)
			return requested;

		return WithNumber(requested, chosen);
	}

	bool Changes(const std::string& key) const override
	{
		int number = 0;
		const Screen* const screen = ScreenOf(key, number);

		return screen != nullptr && !StageImage::Find(number, screen->picture).empty();
	}

	std::string BasePath(const std::string& key, const char* requested) const override
	{
		int number = 0;

		if (ScreenOf(key, number) == nullptr || number == Pictures::kDonorNumber)
			return requested;

		++g_lent;
		return WithNumber(requested, Pictures::kDonorNumber);
	}

	bool Apply(const std::string& key, std::vector<uint8_t>& content) const override
	{
		int number = 0;
		const Screen* const screen = ScreenOf(key, number);
		const std::string path = screen ? StageImage::Find(number, screen->picture) : std::string();

		if (path.empty())
			return false;

		uint8_t* const pixels = TexturePixels(content, screen->side);
		StageImage::Bitmap image;

		if (pixels == nullptr)
		{
			LOG("StagePictures: %s has no %dx%d texture where the mod expects one, so %s is not used", key.c_str(),
				screen->side, screen->side, path.c_str());
			return false;
		}

		if (!StageImage::Load(path, image))
			return false;

		for (size_t i = 0; i < screen->count; ++i)
			Paint(pixels, screen->side, image, screen->regions[i]);

		++g_painted;
		LOG("StagePictures: %s shows %s", key.c_str(), path.c_str());
		return true;
	}

	uint32_t Version() const override { return StageRevision::Current(); }
};

PictureOverlay g_overlay;

}

void StagePictures::Register()
{
	ModFiles::AddOverlay(&g_overlay);
}

int StagePictures::Lent()
{
	return g_lent.load();
}

int StagePictures::Painted()
{
	return g_painted.load();
}
