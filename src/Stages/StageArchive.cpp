#include "Stages/StageArchive.h"

#include "Core/logger.h"
#include "Core/utils.h"
#include "Stages/FbGameFolder.h"
#include "Stages/FbxExLocal.h"
#include "Stages/FbxToFbxEx.h"
#include "Stages/GameArchive.h"
#include "Stages/UnielCipher.h"

#include <windows.h>

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <map>

namespace {

constexpr const char* kBgList = "BgList.txt";
constexpr const char* kEnglishNames = "BgList_str.txt";
constexpr const char* kModel = "bg.fbx.bin";
constexpr const char* kSourceModel = "bg.fbx";
constexpr const char* kBgFolder = "bg";
constexpr const char* kBgPrefix = "bg\\";
constexpr size_t kScoreSpan = 2048;
constexpr int kTextScore = 990;

struct Magic
{
	const char* extension;
	const char* bytes;
	size_t length;
};

const Magic kMagics[] = {
	{ "dds", "DDS \x7c\0\0\0", 8 },
	{ "bin", "fbxex\0\0\0\0\0\0\0\0\0\0\0", 16 },
	{ "pat", "PAniDataFile", 12 },
	{ "img", "\0\0\0\0\x07\0\0\0", 8 },
};

std::string Combine(const std::string& folder, const std::string& name)
{
	if (folder.empty() || folder.back() == '\\' || folder.back() == '/')
		return folder + name;

	return folder + "\\" + name;
}

std::string Lowered(const std::string& text)
{
	std::string out = text;

	for (char& c : out)
		c = static_cast<char>(tolower(static_cast<unsigned char>(c)));

	return out;
}

std::string Extension(const std::string& file)
{
	const size_t dot = file.rfind('.');

	return dot == std::string::npos ? std::string() : Lowered(file.substr(dot + 1));
}

const Magic* MagicFor(const std::string& file)
{
	const std::string extension = Extension(file);

	for (const Magic& magic : kMagics)
	{
		if (extension == magic.extension)
			return &magic;
	}

	return nullptr;
}

bool IsText(const std::string& file)
{
	const std::string extension = Extension(file);

	return extension == "txt" || extension == "ini" || extension == "csv";
}

bool LeadByte(uint8_t byte)
{
	return (byte >= 0x81 && byte <= 0x9f) || (byte >= 0xe0 && byte <= 0xfc);
}

bool TextByte(uint8_t byte)
{
	return byte == '\t' || byte == '\r' || byte == '\n' || (byte >= 0x20 && byte < 0x7f) || (byte >= 0x80 && byte <= 0xfc);
}

int TextScore(const std::vector<uint8_t>& data)
{
	const size_t span = data.size() < kScoreSpan ? data.size() : kScoreSpan;

	if (span == 0)
		return 0;

	size_t good = 0;

	for (size_t i = 0; i < span; ++i)
		good += TextByte(data[i]) ? 1 : 0;

	return static_cast<int>((good * 1000) / span);
}

size_t SkipComment(const std::string& text, size_t at)
{
	if (text[at] == '"')
	{
		const size_t close = text.find_first_of("\"\n", at + 1);
		return close == std::string::npos ? text.size() : close;
	}

	if (text[at] != '/' || at + 1 >= text.size())
		return at;

	if (text[at + 1] == '/')
	{
		const size_t line = text.find('\n', at);
		return line == std::string::npos ? text.size() : line;
	}

	if (text[at + 1] != '*')
		return at;

	const size_t close = text.find("*/", at + 2);
	return close == std::string::npos ? text.size() : close + 1;
}

size_t Skip(const std::string& text, size_t at, const char* of)
{
	while (at < text.size() && text[at] != '\0' && strchr(of, text[at]) != nullptr)
		++at;

	return at;
}

size_t ValueStart(const std::string& block, size_t after)
{
	const size_t sameLine = Skip(block, after, " \t");
	const size_t anyLine = Skip(block, after, " \t\r\n");

	return anyLine < block.size() && block[anyLine] == '[' ? anyLine : sameLine;
}

size_t ValueEnd(const std::string& block, size_t value)
{
	if (value < block.size() && block[value] == '[')
		return StageArchive::MatchPair(block, value);

	if (value < block.size() && block[value] == '"')
	{
		const size_t close = block.find('"', value + 1);
		return close == std::string::npos ? std::string::npos : close + 1;
	}

	size_t end = value;

	while (end < block.size() && block[end] != ',' && block[end] != '\n' && block[end] != '\r' &&
		!(block[end] == '/' && end + 1 < block.size() && block[end + 1] == '/'))
	{
		++end;
	}

	while (end > value && (block[end - 1] == ' ' || block[end - 1] == '\t'))
		--end;

	return end;
}

bool KeyAt(const std::string& text, size_t at, const char* key, size_t length)
{
	if (at + length > text.size() || text.compare(at, length, key) != 0)
		return false;

	const char before = at == 0 ? ' ' : text[at - 1];
	const char after = at + length >= text.size() ? ' ' : text[at + length];

	return isalnum(static_cast<unsigned char>(before)) == 0 && before != '_' &&
		isalnum(static_cast<unsigned char>(after)) == 0 && after != '_';
}

uint32_t BytesOf(const std::string& path)
{
	WIN32_FILE_ATTRIBUTE_DATA info = {};

	if (GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &info) == 0)
		return 0;

	return info.nFileSizeLow;
}

std::string NameFromList(const std::string& bgList, const std::string& stage)
{
	std::string block;
	std::string name;

	if (!StageArchive::Block(bgList, stage, block) || !StageArchive::Field(block, "Name", name))
		return std::string();

	return StageArchive::Unquoted(name);
}

class ArchiveSource : public StageArchive::Source
{
public:
	explicit ArchiveSource(const std::string& folder)
	{
		m_archive.Open(Combine(folder, "d"));
	}

	bool IsOpen() const { return m_archive.IsOpen(); }

	void Stages(std::vector<StageArchive::Stage>& out) override;
	void Files(const std::string& stage, std::vector<std::string>& out) override;
	bool Read(const std::string& stage, const std::string& file, std::vector<uint8_t>& out) override;
	bool BgList(std::string& out) override;

private:
	GameArchive m_archive;
};

void ArchiveSource::Stages(std::vector<StageArchive::Stage>& out)
{
	out.clear();

	std::string bgList;
	BgList(bgList);

	std::vector<std::string> folders;
	m_archive.Folders(folders);

	const size_t prefix = strlen(kBgPrefix);

	for (const std::string& folder : folders)
	{
		if (folder.size() <= prefix || _strnicmp(folder.c_str(), kBgPrefix, prefix) != 0)
			continue;

		std::vector<std::string> files;
		m_archive.List(folder.c_str(), files);

		bool model = false;

		for (const std::string& file : files)
			model = model || _stricmp(file.c_str(), kModel) == 0;

		if (!model)
			continue;

		StageArchive::Stage stage;
		stage.folder = folder.substr(prefix);
		stage.bytes = m_archive.FolderBytes(folder.c_str());
		stage.name = NameFromList(bgList, stage.folder);

		out.push_back(stage);
	}
}

void ArchiveSource::Files(const std::string& stage, std::vector<std::string>& out)
{
	m_archive.List((kBgPrefix + stage).c_str(), out);
}

bool ArchiveSource::Read(const std::string& stage, const std::string& file, std::vector<uint8_t>& out)
{
	return m_archive.Read((kBgPrefix + stage).c_str(), file.c_str(), out);
}

bool ArchiveSource::BgList(std::string& out)
{
	std::vector<uint8_t> blob;

	if (!m_archive.Read(kBgFolder, kBgList, blob))
		return false;

	out.assign(blob.begin(), blob.end());
	return true;
}

class FolderSource : public StageArchive::Source
{
public:
	explicit FolderSource(const std::string& folder)
		: m_bg(Combine(folder, kBgFolder))
	{
	}

	bool IsOpen() const
	{
		return GetFileAttributesA(Combine(m_bg, kBgList).c_str()) != INVALID_FILE_ATTRIBUTES;
	}

	void Stages(std::vector<StageArchive::Stage>& out) override;
	void Files(const std::string& stage, std::vector<std::string>& out) override;
	bool Read(const std::string& stage, const std::string& file, std::vector<uint8_t>& out) override;
	bool BgList(std::string& out) override;

protected:
	virtual const char* ModelName() const = 0;
	virtual std::string Exported(const std::string& name) const = 0;
	virtual bool Convert(std::vector<uint8_t>& data, const std::string& stage) const = 0;
	virtual void Decode(std::vector<uint8_t>&) const {}
	virtual std::string Named(int) { return std::string(); }

	bool Whole(const std::string& path, std::vector<uint8_t>& out) const;
	uint32_t FolderBytes(const std::string& stage) const;

	std::string m_bg;
};

bool FolderSource::Whole(const std::string& path, std::vector<uint8_t>& out) const
{
	if (!ReadWholeFile(path, out))
		return false;

	Decode(out);
	return true;
}

uint32_t FolderSource::FolderBytes(const std::string& stage) const
{
	const std::string folder = Combine(m_bg, stage);

	WIN32_FIND_DATAA found = {};
	const HANDLE search = FindFirstFileA(Combine(folder, "*").c_str(), &found);

	if (search == INVALID_HANDLE_VALUE)
		return 0;

	uint32_t bytes = 0;

	do
	{
		if ((found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			bytes += found.nFileSizeLow;
	}
	while (FindNextFileA(search, &found) != 0);

	FindClose(search);
	return bytes;
}

void FolderSource::Stages(std::vector<StageArchive::Stage>& out)
{
	out.clear();

	std::string bgList;
	BgList(bgList);

	WIN32_FIND_DATAA found = {};
	const HANDLE search = FindFirstFileA(Combine(m_bg, "bg*").c_str(), &found);

	if (search == INVALID_HANDLE_VALUE)
		return;

	do
	{
		const std::string name = found.cFileName;

		if ((found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 || name == "." || name == "..")
			continue;

		if (BytesOf(Combine(Combine(m_bg, name), ModelName())) == 0)
			continue;

		StageArchive::Stage stage;
		stage.folder = name;
		stage.bytes = FolderBytes(name);
		stage.name = Named(StageArchive::NumberOf(name));

		if (stage.name.empty())
			stage.name = NameFromList(bgList, name);

		out.push_back(stage);
	}
	while (FindNextFileA(search, &found) != 0);

	FindClose(search);
}

void FolderSource::Files(const std::string& stage, std::vector<std::string>& out)
{
	out.clear();

	WIN32_FIND_DATAA found = {};
	const HANDLE search = FindFirstFileA(Combine(Combine(m_bg, stage), "*").c_str(), &found);

	if (search == INVALID_HANDLE_VALUE)
		return;

	do
	{
		if ((found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
			continue;

		const std::string exported = Exported(found.cFileName);

		if (!exported.empty())
			out.push_back(exported);
	}
	while (FindNextFileA(search, &found) != 0);

	FindClose(search);
}

bool FolderSource::Read(const std::string& stage, const std::string& file, std::vector<uint8_t>& out)
{
	out.clear();

	const std::string folder = Combine(m_bg, stage);

	if (_stricmp(file.c_str(), kModel) != 0)
		return Whole(Combine(folder, file), out);

	return Whole(Combine(folder, ModelName()), out) && Convert(out, stage);
}

bool FolderSource::BgList(std::string& out)
{
	out.clear();

	std::vector<uint8_t> data;

	if (!Whole(Combine(m_bg, kBgList), data))
		return false;

	out.assign(data.begin(), data.end());
	return true;
}

class UnielSource : public FolderSource
{
public:
	using FolderSource::FolderSource;

protected:
	const char* ModelName() const override { return kModel; }

	std::string Exported(const std::string& name) const override
	{
		return _stricmp(name.c_str(), kSourceModel) == 0 ? std::string() : name;
	}

	bool Convert(std::vector<uint8_t>& data, const std::string& stage) const override;

	void Decode(std::vector<uint8_t>& data) const override
	{
		UnielCipher::Decrypt(data);
	}
};

bool UnielSource::Convert(std::vector<uint8_t>& data, const std::string& stage) const
{
	if (data.empty())
		return false;

	FbxExLocal::Report report = {};

	if (FbxExLocal::Apply(data, report) && report.nodes != 0)
		LOG("StageArchive: %s bakes each node's world matrix - %d node(s) and %d frame(s) put back onto their "
			"parents", stage.c_str(), report.nodes, report.frames);

	return true;
}

class DfciSource : public FolderSource
{
public:
	using FolderSource::FolderSource;

protected:
	const char* ModelName() const override { return kSourceModel; }

	std::string Exported(const std::string& name) const override
	{
		return _stricmp(name.c_str(), kSourceModel) == 0 ? std::string(kModel) : name;
	}

	bool Convert(std::vector<uint8_t>& data, const std::string& stage) const override;
	std::string Named(int number) override;

private:
	void LoadEnglish();

	std::map<int, std::string> m_english;
	bool m_loaded = false;
};

bool DfciSource::Convert(std::vector<uint8_t>& data, const std::string& stage) const
{
	std::vector<uint8_t> model;
	std::string error;

	if (FbxToFbxEx::Convert(data.data(), data.size(), model, error))
	{
		data.swap(model);
		return true;
	}

	LOG("StageArchive: %s\\%s did not convert - %s", stage.c_str(), kSourceModel, error.c_str());
	data.clear();
	return false;
}

std::string DfciSource::Named(int number)
{
	LoadEnglish();

	const std::map<int, std::string>::const_iterator english = m_english.find(number);

	return english == m_english.end() ? std::string() : english->second;
}

void DfciSource::LoadEnglish()
{
	if (m_loaded)
		return;

	m_loaded = true;

	std::vector<uint8_t> data;

	if (!Whole(Combine(m_bg, kEnglishNames), data))
		return;

	const std::string text(data.begin(), data.end());

	for (size_t at = 0; at < text.size();)
	{
		size_t end = text.find('\n', at);
		end = end == std::string::npos ? text.size() : end;

		const std::string line = text.substr(at, end - at);
		at = end + 1;

		const size_t equals = line.find('=');
		const size_t digits = Skip(line, 0, " \t");

		if (equals == std::string::npos || digits >= equals || isdigit(static_cast<unsigned char>(line[digits])) == 0)
			continue;

		const size_t value = Skip(line, equals + 1, " \t");
		size_t tail = line.size();

		while (tail > value && isspace(static_cast<unsigned char>(line[tail - 1])) != 0)
			--tail;

		if (tail > value)
			m_english[atoi(line.c_str() + digits)] = line.substr(value, tail - value);
	}
}

template <typename T>
std::unique_ptr<StageArchive::Source> Opened(const char* folder)
{
	std::unique_ptr<T> source(new T(folder));

	if (!source->IsOpen())
		return nullptr;

	return std::unique_ptr<StageArchive::Source>(source.release());
}

}

std::unique_ptr<StageArchive::Source> StageArchive::Open(const char* folder)
{
	switch (FbGameFolder::Detect(folder))
	{
	case FbGameFolder::Game_UNI2:
	case FbGameFolder::Game_UNI:
		return Opened<ArchiveSource>(folder);

	case FbGameFolder::Game_UNIEL:
		return Opened<UnielSource>(folder);

	case FbGameFolder::Game_DFCI:
		return Opened<DfciSource>(folder);

	default:
		return nullptr;
	}
}

bool StageArchive::MagicOk(const std::string& file, const std::vector<uint8_t>& data)
{
	if (data.empty())
		return false;

	const Magic* const magic = MagicFor(file);

	if (magic != nullptr)
		return data.size() >= magic->length && memcmp(data.data(), magic->bytes, magic->length) == 0;

	return !IsText(file) || TextScore(data) > kTextScore;
}

int StageArchive::NumberOf(const std::string& stage)
{
	size_t at = stage.size();

	while (at > 0 && isdigit(static_cast<unsigned char>(stage[at - 1])) != 0)
		--at;

	return at == stage.size() ? -1 : atoi(stage.c_str() + at);
}

size_t StageArchive::MatchPair(const std::string& text, size_t open)
{
	const char opener = open < text.size() ? text[open] : '\0';
	const char closer = opener == '{' ? '}' : (opener == '[' ? ']' : '\0');

	if (closer == '\0')
		return std::string::npos;

	int depth = 0;

	for (size_t at = open; at < text.size(); ++at)
	{
		const uint8_t byte = static_cast<uint8_t>(text[at]);

		if (LeadByte(byte) && at + 1 < text.size())
		{
			++at;
			continue;
		}

		const size_t skipped = SkipComment(text, at);

		if (skipped != at)
		{
			at = skipped;
			continue;
		}

		depth += byte == opener ? 1 : (byte == closer ? -1 : 0);

		if (depth == 0)
			return at + 1;
	}

	return std::string::npos;
}

bool StageArchive::FieldSpan(const std::string& block, const char* key, size_t& keyAt, size_t& valueAt,
	size_t& valueEnd)
{
	const size_t length = strlen(key);

	for (size_t at = 0; at + length < block.size(); ++at)
	{
		const size_t skipped = SkipComment(block, at);

		if (skipped != at)
		{
			at = skipped;
			continue;
		}

		if (!KeyAt(block, at, key, length))
			continue;

		const size_t equals = Skip(block, at + length, " \t");

		if (equals >= block.size() || block[equals] != '=')
			continue;

		keyAt = at;
		valueAt = ValueStart(block, equals + 1);
		valueEnd = ValueEnd(block, valueAt);

		return valueEnd != std::string::npos && valueEnd > valueAt;
	}

	return false;
}

bool StageArchive::Block(const std::string& bgList, const std::string& stage, std::string& out)
{
	const std::string named = "\"" + stage + "\"";
	const int number = NumberOf(stage);

	for (int pass = 0; pass < 2; ++pass)
	{
		if (pass == 1 && number < 0)
			return false;

		for (size_t at = bgList.find("Bg_"); at != std::string::npos; at = bgList.find("Bg_", at + 3))
		{
			size_t digits = at + 3;

			while (digits < bgList.size() && isdigit(static_cast<unsigned char>(bgList[digits])) != 0)
				++digits;

			if (digits == at + 3 || (pass == 1 && atoi(bgList.c_str() + at + 3) != number))
				continue;

			const size_t equals = Skip(bgList, digits, " \t");
			const size_t open = Skip(bgList, equals + 1, " \t\r\n");

			if (equals >= bgList.size() || bgList[equals] != '=' || open >= bgList.size() || bgList[open] != '{')
				continue;

			const size_t end = MatchPair(bgList, open);

			if (end == std::string::npos)
				return false;

			const std::string body = bgList.substr(open + 1, end - open - 2);
			std::string data;

			if (pass == 0 && (!Field(body, "DataFile", data) || _stricmp(data.c_str(), named.c_str()) != 0))
				continue;

			out = body;
			return true;
		}
	}

	return false;
}

bool StageArchive::Field(const std::string& block, const char* key, std::string& out)
{
	size_t keyAt = 0;
	size_t valueAt = 0;
	size_t valueEnd = 0;

	if (!FieldSpan(block, key, keyAt, valueAt, valueEnd))
		return false;

	out = block.substr(valueAt, valueEnd - valueAt);
	return true;
}

std::string StageArchive::Unquoted(const std::string& value)
{
	if (value.empty() || value.front() != '"')
		return value;

	return value.substr(1, value.size() - (value.size() > 1 && value.back() == '"' ? 2 : 1));
}
