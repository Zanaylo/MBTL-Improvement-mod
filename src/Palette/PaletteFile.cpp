#include "Palette/PaletteFile.h"

#include "Core/logger.h"
#include "Core/utils.h"

#include <cstring>
#include <vector>

namespace {

constexpr uint32_t kHeader[4] = { 0x0000FFFF, 1, 0, 1 };
constexpr size_t kHeaderBytes = sizeof(kHeader);
constexpr size_t kCountAt = 12;
constexpr size_t kBareCount = 4;

constexpr char kTrailerMagic[8] = { 'M', 'B', 'T', 'L', 'I', 'M', 'P', 'L' };
constexpr char kForeignTrailerMagic[8] = { 'U', 'N', 'I', '2', 'I', 'M', 'P', 'L' };
constexpr char kEffectMagic[4] = { 'M', 'B', 'T', 'E' };
constexpr char kForeignEffectMagic[4] = { 'U', 'I', '2', 'E' };
constexpr char kSubMagic[4] = { 'M', 'B', 'T', 'S' };
constexpr uint8_t kOpaque = 0xFF;
constexpr size_t kRecordBytes = 4;
constexpr size_t kSubRecordBytes = 1 + PaletteFile::kBytes;

struct Trailer
{
	char magic[8];
	PaletteFile::Info info;
};

void NameFromPath(const std::string& path, char* out, size_t size)
{
	const size_t slash = path.find_last_of("\\/");
	const size_t start = slash == std::string::npos ? 0 : slash + 1;
	const size_t dot = path.find_last_of('.');
	const size_t end = dot == std::string::npos || dot < start ? path.size() : dot;

	strncpy_s(out, size, path.substr(start, end - start).c_str(), _TRUNCATE);
}

size_t PaletteStart(const std::vector<uint8_t>& file)
{
	uint32_t count = 0;

	if (file.size() >= kBareCount + PaletteFile::kBytes)
	{
		std::memcpy(&count, file.data(), sizeof(count));

		if (count > 0 && count * static_cast<size_t>(PaletteFile::kBytes) + kBareCount <= file.size())
			return kBareCount;
	}

	if (file.size() < kHeaderBytes + PaletteFile::kBytes)
		return 0;

	std::memcpy(&count, file.data() + kCountAt, sizeof(count));
	return count > 0 && count * static_cast<size_t>(PaletteFile::kBytes) + kHeaderBytes <= file.size() ? kHeaderBytes : 0;
}

bool IsEffectMagic(const uint8_t* at)
{
	return std::memcmp(at, kEffectMagic, sizeof(kEffectMagic)) == 0 ||
		std::memcmp(at, kForeignEffectMagic, sizeof(kForeignEffectMagic)) == 0;
}

size_t ReadEffectBlock(const std::vector<uint8_t>& file, size_t from, PaletteFile::Content& out)
{
	uint16_t count = 0;

	if (from + sizeof(kEffectMagic) + sizeof(count) > file.size() || !IsEffectMagic(file.data() + from))
		return 0;

	std::memcpy(&count, file.data() + from + sizeof(kEffectMagic), sizeof(count));

	const size_t records = from + sizeof(kEffectMagic) + sizeof(count);

	if (records + count * kRecordBytes > file.size())
		return 0;

	for (uint16_t i = 0; i < count; ++i)
	{
		const uint8_t* const record = file.data() + records + i * kRecordBytes;
		uint8_t* const entry = out.effects + record[0] * 4;

		std::memcpy(entry, record + 1, 3);
		entry[3] = kOpaque;
	}

	out.hasEffects = count > 0;
	return sizeof(kEffectMagic) + sizeof(count) + count * kRecordBytes;
}

size_t ReadSubBlock(const std::vector<uint8_t>& file, size_t from, PaletteFile::Content& out)
{
	if (from + sizeof(kSubMagic) + 1 > file.size() || std::memcmp(file.data() + from, kSubMagic, sizeof(kSubMagic)) != 0)
		return 0;

	const uint8_t count = file[from + sizeof(kSubMagic)];
	const size_t records = from + sizeof(kSubMagic) + 1;

	if (records + count * kSubRecordBytes > file.size())
		return 0;

	for (uint8_t i = 0; i < count; ++i)
	{
		const uint8_t* const record = file.data() + records + i * kSubRecordBytes;

		if (record[0] == 0 || record[0] >= PaletteFile::kSubPalettes)
			continue;

		std::memcpy(out.pages[record[0]], record + 1, PaletteFile::kBytes);
		out.subMask |= static_cast<uint8_t>(1u << record[0]);
	}

	return sizeof(kSubMagic) + 1 + count * kSubRecordBytes;
}

void ReadTrailer(const std::vector<uint8_t>& file, size_t from, PaletteFile::Info& info)
{
	Trailer trailer = {};

	if (from + sizeof(trailer) > file.size())
		return;

	std::memcpy(&trailer, file.data() + from, sizeof(trailer));

	if (std::memcmp(trailer.magic, kTrailerMagic, sizeof(kTrailerMagic)) != 0 &&
		std::memcmp(trailer.magic, kForeignTrailerMagic, sizeof(kForeignTrailerMagic)) != 0)
	{
		return;
	}

	info = trailer.info;
	info.name[PaletteFile::kNameLength - 1] = '\0';
	info.creator[PaletteFile::kCreatorLength - 1] = '\0';
	info.description[PaletteFile::kDescriptionLength - 1] = '\0';
}

void AppendEffectBlock(std::vector<uint8_t>& out, const uint8_t* effects)
{
	uint16_t count = 0;

	for (int i = 1; i < PaletteFile::kColors; ++i)
		count += effects[i * 4 + 3] == kOpaque ? 1 : 0;

	if (count == 0)
		return;

	out.insert(out.end(), kEffectMagic, kEffectMagic + sizeof(kEffectMagic));
	out.push_back(static_cast<uint8_t>(count));
	out.push_back(static_cast<uint8_t>(count >> 8));

	for (int i = 1; i < PaletteFile::kColors; ++i)
	{
		if (effects[i * 4 + 3] != kOpaque)
			continue;

		out.push_back(static_cast<uint8_t>(i));
		out.insert(out.end(), effects + i * 4, effects + i * 4 + 3);
	}
}

void AppendSubBlock(std::vector<uint8_t>& out, const PaletteFile::Content& content)
{
	uint8_t count = 0;

	for (int sub = 1; sub < PaletteFile::kSubPalettes; ++sub)
		count += (content.subMask & (1u << sub)) != 0 ? 1 : 0;

	if (count == 0)
		return;

	out.insert(out.end(), kSubMagic, kSubMagic + sizeof(kSubMagic));
	out.push_back(count);

	for (int sub = 1; sub < PaletteFile::kSubPalettes; ++sub)
	{
		if ((content.subMask & (1u << sub)) == 0)
			continue;

		out.push_back(static_cast<uint8_t>(sub));
		out.insert(out.end(), content.pages[sub], content.pages[sub] + PaletteFile::kBytes);
	}
}

}

bool PaletteFile::Load(const std::string& path, Content& out)
{
	std::memset(&out, 0, sizeof(out));

	std::vector<uint8_t> file;

	if (!ReadWholeFile(path, file))
		return false;

	const size_t start = file.size() == static_cast<size_t>(kBytes) ? 0 : PaletteStart(file);

	if (start == 0 && file.size() != static_cast<size_t>(kBytes))
	{
		LOG("palette file: '%s' is %u bytes and is not a palette", path.c_str(), static_cast<unsigned>(file.size()));
		return false;
	}

	std::memcpy(out.pages[0], file.data() + start, kBytes);
	out.subMask = 1;

	size_t at = start + kBytes;
	at += ReadEffectBlock(file, at, out);
	at += ReadSubBlock(file, at, out);

	ReadTrailer(file, at, out.info);

	if (out.info.name[0] == '\0')
		NameFromPath(path, out.info.name, sizeof(out.info.name));

	return true;
}

bool PaletteFile::Save(const std::string& path, const Content& content)
{
	std::vector<uint8_t> out(kHeaderBytes);
	std::memcpy(out.data(), kHeader, kHeaderBytes);
	out.insert(out.end(), content.pages[0], content.pages[0] + kBytes);

	if (content.hasEffects)
		AppendEffectBlock(out, content.effects);

	AppendSubBlock(out, content);

	Trailer trailer = {};
	std::memcpy(trailer.magic, kTrailerMagic, sizeof(kTrailerMagic));
	trailer.info = content.info;

	const uint8_t* const bytes = reinterpret_cast<const uint8_t*>(&trailer);
	out.insert(out.end(), bytes, bytes + sizeof(trailer));

	return WriteWholeFile(path, out.data(), out.size());
}
