#include "Core/DdsImage.h"

#include <cstring>
#include <utility>

namespace {

constexpr const char* kMagic = "DDS ";
constexpr size_t kMagicBytes = 4;
constexpr size_t kHeader = 128;
constexpr size_t kHeightAt = 12;
constexpr size_t kWidthAt = 16;
constexpr size_t kPixelFlagsAt = 80;
constexpr size_t kFourCcAt = 84;
constexpr size_t kBitsAt = 88;
constexpr size_t kRedMaskAt = 92;
constexpr uint32_t kAlphaFlag = 0x1;
constexpr uint32_t kFourCcFlag = 0x4;
constexpr uint32_t kDxt1 = 0x31545844;
constexpr uint32_t kDxt5 = 0x35545844;
constexpr uint32_t kRedLow = 0xFF;
constexpr uint32_t kBits = 32;
constexpr uint32_t kLargest = 8192;
constexpr int kBlockSide = 4;
constexpr size_t kChannels = 4;
constexpr size_t kColourBlock = 8;
constexpr size_t kAlphaBlock = 8;
constexpr uint8_t kOpaque = 0xFF;

struct Block
{
	uint8_t colours[4][kChannels];
	uint8_t alpha[8];
	uint32_t colourBits;
	uint64_t alphaBits;
	bool five;
};

uint32_t Read32(const uint8_t* at)
{
	uint32_t value = 0;
	std::memcpy(&value, at, sizeof(value));

	return value;
}

void Rgb565(uint16_t packed, uint8_t* out)
{
	out[2] = static_cast<uint8_t>((((packed >> 11) & 0x1F) * 255) / 31);
	out[1] = static_cast<uint8_t>((((packed >> 5) & 0x3F) * 255) / 63);
	out[0] = static_cast<uint8_t>(((packed & 0x1F) * 255) / 31);
}

void ColourTable(const uint8_t* colour, bool five, Block& block)
{
	const uint16_t first = static_cast<uint16_t>(colour[0] | (colour[1] << 8));
	const uint16_t second = static_cast<uint16_t>(colour[2] | (colour[3] << 8));
	const bool four = five || first > second;
	uint8_t (*const table)[kChannels] = block.colours;

	Rgb565(first, table[0]);
	Rgb565(second, table[1]);

	for (int channel = 0; channel < 3; ++channel)
	{
		table[2][channel] = static_cast<uint8_t>(four ? (2 * table[0][channel] + table[1][channel]) / 3
			: (table[0][channel] + table[1][channel]) / 2);
		table[3][channel] = static_cast<uint8_t>(four ? (table[0][channel] + 2 * table[1][channel]) / 3 : 0);
	}

	table[0][3] = kOpaque;
	table[1][3] = kOpaque;
	table[2][3] = kOpaque;
	table[3][3] = four ? kOpaque : 0;
	block.colourBits = Read32(colour + 4);
}

void AlphaTable(const uint8_t* at, Block& block)
{
	uint8_t* const alpha = block.alpha;
	alpha[0] = at[0];
	alpha[1] = at[1];
	block.alphaBits = 0;

	for (int i = 0; i < 6; ++i)
		block.alphaBits |= static_cast<uint64_t>(at[2 + i]) << (8 * i);

	if (alpha[0] > alpha[1])
	{
		for (int i = 1; i < 7; ++i)
			alpha[i + 1] = static_cast<uint8_t>(((7 - i) * alpha[0] + i * alpha[1]) / 7);

		return;
	}

	for (int i = 1; i < 5; ++i)
		alpha[i + 1] = static_cast<uint8_t>(((5 - i) * alpha[0] + i * alpha[1]) / 5);

	alpha[6] = 0;
	alpha[7] = kOpaque;
}

void PutBlock(const Block& block, int blockX, int blockY, int width, int height, std::vector<uint8_t>& out)
{
	for (int i = 0; i < kBlockSide * kBlockSide; ++i)
	{
		const int x = blockX * kBlockSide + i % kBlockSide;
		const int y = blockY * kBlockSide + i / kBlockSide;

		if (x >= width || y >= height)
			continue;

		uint8_t* const pixel = &out[(static_cast<size_t>(y) * width + x) * kChannels];
		std::memcpy(pixel, block.colours[(block.colourBits >> (2 * i)) & 3], kChannels);

		if (block.five)
			pixel[3] = block.alpha[(block.alphaBits >> (3 * i)) & 7];
	}
}

bool Compressed(const std::vector<uint8_t>& blob, int width, int height, bool five, std::vector<uint8_t>& out)
{
	const int blocksX = (width + kBlockSide - 1) / kBlockSide;
	const int blocksY = (height + kBlockSide - 1) / kBlockSide;
	const size_t stride = five ? kAlphaBlock + kColourBlock : kColourBlock;

	if (blob.size() < kHeader + static_cast<size_t>(blocksX) * blocksY * stride)
		return false;

	out.assign(static_cast<size_t>(width) * height * kChannels, 0);

	for (int blockY = 0; blockY < blocksY; ++blockY)
	{
		for (int blockX = 0; blockX < blocksX; ++blockX)
		{
			const uint8_t* const at = blob.data() + kHeader + (static_cast<size_t>(blockY) * blocksX + blockX) * stride;
			Block block = {};
			block.five = five;

			if (five)
				AlphaTable(at, block);

			ColourTable(five ? at + kAlphaBlock : at, five, block);
			PutBlock(block, blockX, blockY, width, height, out);
		}
	}

	return true;
}

bool Uncompressed(const std::vector<uint8_t>& blob, int width, int height, std::vector<uint8_t>& out)
{
	const size_t need = static_cast<size_t>(width) * height * kChannels;

	if (Read32(blob.data() + kBitsAt) != kBits || blob.size() < kHeader + need)
		return false;

	out.assign(blob.begin() + kHeader, blob.begin() + kHeader + need);

	const bool swapped = Read32(blob.data() + kRedMaskAt) == kRedLow;
	const bool alpha = (Read32(blob.data() + kPixelFlagsAt) & kAlphaFlag) != 0;

	for (size_t at = 0; at < need; at += kChannels)
	{
		if (swapped)
			std::swap(out[at], out[at + 2]);

		if (!alpha)
			out[at + 3] = kOpaque;
	}

	return true;
}

}

bool DdsImage::Decode(const std::vector<uint8_t>& blob, int& outWidth, int& outHeight, std::vector<uint8_t>& outBgra)
{
	if (blob.size() < kHeader || std::memcmp(blob.data(), kMagic, kMagicBytes) != 0)
		return false;

	const uint32_t width = Read32(blob.data() + kWidthAt);
	const uint32_t height = Read32(blob.data() + kHeightAt);

	if (width == 0 || height == 0 || width > kLargest || height > kLargest)
		return false;

	const uint32_t fourCc = Read32(blob.data() + kFourCcAt);
	const bool compressed = (Read32(blob.data() + kPixelFlagsAt) & kFourCcFlag) != 0;

	if (compressed && fourCc != kDxt1 && fourCc != kDxt5)
		return false;

	const bool decoded = compressed ? Compressed(blob, static_cast<int>(width), static_cast<int>(height), fourCc == kDxt5, outBgra)
		: Uncompressed(blob, static_cast<int>(width), static_cast<int>(height), outBgra);

	if (!decoded)
		return false;

	outWidth = static_cast<int>(width);
	outHeight = static_cast<int>(height);
	return true;
}
