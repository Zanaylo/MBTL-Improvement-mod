#include "Palette/PngPalette.h"

#include "Core/utils.h"

#include <cstring>
#include <vector>

namespace {

constexpr uint8_t kSignature[8] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
constexpr size_t kChunkFrame = 12;
constexpr size_t kChunkHeader = 8;
constexpr size_t kTypeLength = 4;
constexpr int kGridSide = 16;
constexpr size_t kStoredBlock = 65535;
constexpr uint32_t kCrcPolynomial = 0xEDB88320u;
constexpr uint32_t kAdlerModulus = 65521u;
constexpr uint8_t kIndexedColour = 3;
constexpr uint8_t kEightBits = 8;

uint32_t ReadBig32(const uint8_t* bytes)
{
	return (static_cast<uint32_t>(bytes[0]) << 24) | (static_cast<uint32_t>(bytes[1]) << 16) |
		(static_cast<uint32_t>(bytes[2]) << 8) | bytes[3];
}

void WriteBig32(std::vector<uint8_t>& out, uint32_t value)
{
	out.push_back(static_cast<uint8_t>(value >> 24));
	out.push_back(static_cast<uint8_t>(value >> 16));
	out.push_back(static_cast<uint8_t>(value >> 8));
	out.push_back(static_cast<uint8_t>(value));
}

void PutBig32(uint8_t* at, uint32_t value)
{
	at[0] = static_cast<uint8_t>(value >> 24);
	at[1] = static_cast<uint8_t>(value >> 16);
	at[2] = static_cast<uint8_t>(value >> 8);
	at[3] = static_cast<uint8_t>(value);
}

uint32_t Crc32(const uint8_t* data, size_t size)
{
	static uint32_t table[256] = {};
	static bool built = false;

	if (!built)
	{
		for (uint32_t n = 0; n < 256; ++n)
		{
			uint32_t c = n;

			for (int k = 0; k < 8; ++k)
				c = (c & 1) != 0 ? kCrcPolynomial ^ (c >> 1) : c >> 1;

			table[n] = c;
		}

		built = true;
	}

	uint32_t crc = 0xFFFFFFFFu;

	for (size_t i = 0; i < size; ++i)
		crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);

	return crc ^ 0xFFFFFFFFu;
}

uint32_t Adler32(const uint8_t* data, size_t size)
{
	uint32_t a = 1;
	uint32_t b = 0;

	for (size_t i = 0; i < size; ++i)
	{
		a = (a + data[i]) % kAdlerModulus;
		b = (b + a) % kAdlerModulus;
	}

	return (b << 16) | a;
}

void WriteChunk(std::vector<uint8_t>& out, const char* type, const uint8_t* data, size_t size)
{
	WriteBig32(out, static_cast<uint32_t>(size));

	const size_t typeAt = out.size();
	out.insert(out.end(), type, type + kTypeLength);

	if (size > 0)
		out.insert(out.end(), data, data + size);

	WriteBig32(out, Crc32(out.data() + typeAt, kTypeLength + size));
}

std::vector<uint8_t> ZlibStore(const std::vector<uint8_t>& raw)
{
	std::vector<uint8_t> out = { 0x78, 0x01 };

	for (size_t at = 0; at < raw.size();)
	{
		const size_t chunk = raw.size() - at < kStoredBlock ? raw.size() - at : kStoredBlock;
		const uint16_t length = static_cast<uint16_t>(chunk);
		const uint16_t inverse = static_cast<uint16_t>(~length);

		out.push_back(at + chunk >= raw.size() ? 1 : 0);
		out.push_back(static_cast<uint8_t>(length));
		out.push_back(static_cast<uint8_t>(length >> 8));
		out.push_back(static_cast<uint8_t>(inverse));
		out.push_back(static_cast<uint8_t>(inverse >> 8));
		out.insert(out.end(), raw.begin() + at, raw.begin() + at + chunk);

		at += chunk;
	}

	WriteBig32(out, Adler32(raw.data(), raw.size()));
	return out;
}

bool IsPng(const uint8_t* data, size_t size)
{
	return data != nullptr && size >= sizeof(kSignature) && std::memcmp(data, kSignature, sizeof(kSignature)) == 0;
}

size_t FindPalette(const std::vector<uint8_t>& file, uint32_t& outLength)
{
	for (size_t at = sizeof(kSignature); at + kChunkFrame <= file.size();)
	{
		const uint32_t length = ReadBig32(file.data() + at);
		const size_t body = at + kChunkHeader;

		if (body + length + kTypeLength > file.size())
			return 0;

		if (std::memcmp(file.data() + at + kTypeLength, "PLTE", kTypeLength) == 0)
		{
			outLength = length;
			return at;
		}

		if (std::memcmp(file.data() + at + kTypeLength, "IDAT", kTypeLength) == 0)
			return 0;

		at = body + length + kTypeLength;
	}

	return 0;
}

bool Fail(std::string& outError, const char* message)
{
	outError = message;
	return false;
}

}

bool PngPalette::Read(const std::string& path, uint8_t* outRgba, std::string& outError)
{
	std::vector<uint8_t> file;

	if (!ReadWholeFile(path, file))
		return Fail(outError, "could not read the file");

	if (!IsPng(file.data(), file.size()))
		return Fail(outError, "not a PNG file");

	uint32_t length = 0;
	const size_t chunk = FindPalette(file, length);

	if (chunk == 0)
		return Fail(outError, "this PNG has no palette - save it as an indexed (8-bit) image, not RGB");

	const int entries = static_cast<int>(length / 3) < kEntries ? static_cast<int>(length / 3) : kEntries;
	const uint8_t* const colours = file.data() + chunk + kChunkHeader;

	for (int i = 0; i < kEntries; ++i)
	{
		outRgba[i * 4 + 0] = i < entries ? colours[i * 3 + 0] : 0;
		outRgba[i * 4 + 1] = i < entries ? colours[i * 3 + 1] : 0;
		outRgba[i * 4 + 2] = i < entries ? colours[i * 3 + 2] : 0;
		outRgba[i * 4 + 3] = 0xFF;
	}

	return true;
}

bool PngPalette::Write(const std::string& path, const uint8_t* rgba, std::string& outError)
{
	uint8_t header[13] = {};
	header[3] = kGridSide;
	header[7] = kGridSide;
	header[8] = kEightBits;
	header[9] = kIndexedColour;

	uint8_t palette[kEntries * 3] = {};

	for (int i = 0; i < kEntries; ++i)
		std::memcpy(palette + i * 3, rgba + i * 4, 3);

	std::vector<uint8_t> raw;

	for (int y = 0; y < kGridSide; ++y)
	{
		raw.push_back(0);

		for (int x = 0; x < kGridSide; ++x)
			raw.push_back(static_cast<uint8_t>(y * kGridSide + x));
	}

	const std::vector<uint8_t> data = ZlibStore(raw);
	std::vector<uint8_t> file(kSignature, kSignature + sizeof(kSignature));

	WriteChunk(file, "IHDR", header, sizeof(header));
	WriteChunk(file, "PLTE", palette, sizeof(palette));
	WriteChunk(file, "IDAT", data.data(), data.size());
	WriteChunk(file, "IEND", nullptr, 0);

	return WriteWholeFile(path, file.data(), file.size()) || Fail(outError, "could not write the file");
}

bool PngPalette::Recolour(const std::string& path, const uint8_t* basePng, size_t baseSize, const uint8_t* rgba,
	std::string& outError)
{
	if (rgba == nullptr || !IsPng(basePng, baseSize))
		return Fail(outError, "the base image is not a PNG");

	std::vector<uint8_t> file(basePng, basePng + baseSize);
	uint32_t length = 0;
	const size_t chunk = FindPalette(file, length);

	if (chunk == 0)
		return Fail(outError, "the base image has no palette to replace");

	const int entries = static_cast<int>(length / 3) < kEntries ? static_cast<int>(length / 3) : kEntries;
	const size_t body = chunk + kChunkHeader;

	for (int i = 1; i < entries; ++i)
		std::memcpy(file.data() + body + i * 3, rgba + i * 4, 3);

	PutBig32(file.data() + body + length, Crc32(file.data() + chunk + kTypeLength, kTypeLength + length));

	return WriteWholeFile(path, file.data(), file.size()) || Fail(outError, "could not write the file");
}
