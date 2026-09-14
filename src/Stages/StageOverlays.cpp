#include "Stages/StageOverlays.h"

#include "Core/TextEncoding.h"
#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Core/utils.h"
#include "Game/FileOverlay.h"
#include "Game/ModFiles.h"
#include "Stages/BgListText.h"
#include "Stages/GameStages.h"
#include "Stages/HiddenStages.h"
#include "Stages/StageArchive.h"
#include "Stages/StageEntry.h"
#include "Stages/StageLibrary.h"
#include "Stages/StageNames.h"
#include "Stages/StagePicker.h"
#include "Stages/StageReplacements.h"
#include "Stages/StageRevision.h"
#include "Stages/StageTable.h"
#include "Stages/StageThumbs.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <string>
#include <vector>

namespace {

using Entry = StageLibrary::Entry;
using Span = BgListText::Span;

constexpr const char* kListKey = "bg\\bglist.txt";
constexpr const char* kNamesKey = "bg\\bglist_str.ini";
constexpr const char* kMusicKey = "bgm\\bgm.txt";
constexpr const char* kNamesSection = "[data]";
constexpr const char* kTrackHeader = "[BGM_";
constexpr int kTemplateNumber = 1;
constexpr int kFallbackCard = 1;

std::atomic<uint32_t> g_listBytes{ 0 };
std::atomic<int> g_listApplied{ 0 };
std::atomic<int> g_namesApplied{ 0 };
std::atomic<int> g_musicApplied{ 0 };

struct Edit
{
	size_t at;
	size_t length;
	std::string text;
};

struct Section
{
	int id;
	size_t start;
	size_t end;
};

std::string Text(const std::vector<uint8_t>& content)
{
	return std::string(content.begin(), content.end());
}

void Store(const std::string& text, std::vector<uint8_t>& content)
{
	content.assign(text.begin(), text.end());
}

void ApplyEdits(std::string& text, std::vector<Edit>& edits)
{
	std::sort(edits.begin(), edits.end(), [](const Edit& a, const Edit& b) { return a.at > b.at; });

	for (const Edit& edit : edits)
		text.replace(edit.at, edit.length, edit.text);
}

std::string BodyOf(const std::string& list, const Span& span)
{
	return list.substr(span.open + 1, span.end - span.open - 2);
}

std::string BlockOf(const std::string& list, const Span& span)
{
	return list.substr(span.start, span.end - span.start);
}

bool Flag(const std::string& body, const char* key)
{
	std::string value;

	return StageArchive::Field(body, key, value) && atoi(value.c_str()) != 0;
}

bool Holds(const std::vector<int>& numbers, int number)
{
	return std::find(numbers.begin(), numbers.end(), number) != numbers.end();
}

std::string OwnName(int number, const std::string& body)
{
	const char* const english = StageNames::Mbtl(number);

	if (english != nullptr)
		return english;

	std::string field;
	std::string utf8;

	if (StageArchive::Field(body, "Name", field))
	{
		const std::string raw = StageArchive::Unquoted(field);
		TextEncoding::ShiftJisToUtf8(raw.c_str(), raw.size(), utf8);
	}

	if (!utf8.empty())
		return utf8;

	char fallback[24] = {};
	sprintf_s(fallback, "Stage %d", number);

	return fallback;
}

bool Placed(const Entry& entry)
{
	return !entry.removed && entry.number < StageTable::Numbers() && !GameStages::Owns(entry.number);
}

void SnapshotPlaced(std::vector<Entry>& out)
{
	std::vector<Entry> entries;
	StageLibrary::Snapshot(entries);

	out.clear();

	for (const Entry& entry : entries)
	{
		if (Placed(entry))
			out.push_back(entry);
	}
}

const Span& TemplateOf(const std::vector<Span>& spans)
{
	const auto found = std::find_if(spans.begin(), spans.end(),
		[](const Span& span) { return span.number == kTemplateNumber; });

	return found == spans.end() ? spans.front() : *found;
}

void Learn(const std::string& list, const std::vector<Span>& spans, const std::vector<int>& listed,
	const Span& templateSpan)
{
	std::vector<GameStages::Own> own;

	for (const Span& span : spans)
	{
		const std::string body = BodyOf(list, span);
		own.push_back({ span.number, Holds(listed, span.number), Flag(body, "SelectDisable"), OwnName(span.number, body) });
	}

	std::string card;
	const int templateCard = StageArchive::Field(BodyOf(list, templateSpan), "StageSelTex", card)
		? atoi(card.c_str()) : kFallbackCard;

	GameStages::Learn(own, static_cast<int>(listed.size()), templateCard);
}

void List(std::vector<int>& order, int number)
{
	if (Holds(order, number))
		return;

	if (static_cast<int>(order.size()) >= StagePicker::Capacity())
	{
		LOG("StageOverlays: the stage picker is full, so stage %d is left out of it", number);
		return;
	}

	order.push_back(number);
}

std::string ReadNote(int number)
{
	std::vector<uint8_t> blob;
	ReadWholeFile(StageLibrary::NoteOf(number), blob);

	return Text(blob);
}

std::string Reworked(const std::string& list, const Span& span, const std::vector<StageThumbs::Card>& thumbs)
{
	std::string block = BlockOf(list, span);
	size_t keyAt = 0;
	size_t valueAt = 0;
	size_t valueEnd = 0;

	if (HiddenStages::Unlocked(span.number) && StageArchive::FieldSpan(block, "SelectDisable", keyAt, valueAt, valueEnd) &&
		atoi(block.c_str() + valueAt) != 0)
	{
		block.replace(valueAt, valueEnd - valueAt, "0");
	}

	const std::string note = ReadNote(span.number);
	std::string reworked = note.empty() ? block : StageEntry::Rework(block, note);
	const int card = StageThumbs::CardIn(thumbs, span.number);

	if (card >= 0)
		BgListText::SetValue(reworked, "StageSelTex", std::to_string(card));

	return reworked;
}

void ReworkOwn(const std::string& list, const std::vector<Span>& spans, const std::vector<StageThumbs::Card>& thumbs,
	std::vector<int>& order, std::vector<Edit>& edits)
{
	for (const Span& span : spans)
	{
		if (span.number == StageLibrary::kRandomStage)
			continue;

		const std::string block = Reworked(list, span, thumbs);

		if (list.compare(span.start, span.end - span.start, block) != 0)
			edits.push_back({ span.start, span.end - span.start, block });

		if (HiddenStages::Unlocked(span.number))
			List(order, span.number);
	}
}

int CardOf(const Entry& entry, const std::vector<StageThumbs::Card>& thumbs)
{
	const int thumb = StageThumbs::CardIn(thumbs, entry.number);

	return thumb >= 0 ? thumb : entry.card;
}

std::string AddInstalled(const std::string& list, const Span& templateSpan, const std::vector<StageThumbs::Card>& thumbs,
	std::vector<int>& order)
{
	const std::string templateBlock = BlockOf(list, templateSpan);
	const std::string newline = BgListText::Newline(list);

	std::vector<Entry> entries;
	SnapshotPlaced(entries);

	std::string added;

	for (const Entry& entry : entries)
	{
		added += newline + "\t" + StageEntry::Compose(templateBlock, ReadNote(entry.number), entry.number,
			StageEntry::ShiftJis(entry.name), CardOf(entry, thumbs)) + newline;

		if (entry.shown)
			List(order, entry.number);
	}

	return added;
}

bool LineOf(const std::string& names, int number, size_t& start, size_t& end)
{
	char wanted[8] = {};
	sprintf_s(wanted, "%03d", number);

	for (size_t line = 0; line < names.size(); line = end)
	{
		const size_t newline = names.find('\n', line);
		end = newline == std::string::npos ? names.size() : newline + 1;

		if (names.compare(line, 3, wanted) != 0)
			continue;

		const size_t equals = names.find_first_not_of(" \t", line + 3);

		if (equals == std::string::npos || names[equals] != '=')
			continue;

		start = line;
		return true;
	}

	return false;
}

size_t NamesEnd(const std::string& names)
{
	const size_t section = names.find(kNamesSection);
	const size_t next = section == std::string::npos ? std::string::npos : names.find("\n[", section + 1);

	return next == std::string::npos ? names.size() : next + 1;
}

void SetName(std::string& names, int number, const std::string& shiftJisName, const std::string& newline)
{
	char head[16] = {};
	sprintf_s(head, "%03d = ", number);

	const std::string line = head + shiftJisName + newline;
	size_t start = 0;
	size_t end = 0;

	if (LineOf(names, number, start, end))
	{
		names.replace(start, end - start, line);
		return;
	}

	const size_t at = NamesEnd(names);
	const bool joined = at == 0 || names[at - 1] == '\n';

	names.insert(at, joined ? line : newline + line);
}

void Sections(const std::string& text, std::vector<Section>& out)
{
	out.clear();

	const size_t length = strlen(kTrackHeader);

	for (size_t at = text.find(kTrackHeader); at != std::string::npos; at = text.find(kTrackHeader, at + length))
	{
		if (at > 0 && text[at - 1] != '\n')
			continue;

		const size_t next = text.find("\n[", at + 1);
		out.push_back({ atoi(text.c_str() + at + length), at, next == std::string::npos ? text.size() : next + 1 });
	}
}

void LearnTracks(const std::string& text, const std::vector<Section>& sections)
{
	std::vector<GameStages::Track> tracks;

	for (const Section& section : sections)
	{
		std::string file;
		StageArchive::Field(text.substr(section.start, section.end - section.start), "File", file);
		tracks.push_back({ section.id, StageArchive::Unquoted(file) });
	}

	GameStages::LearnTracks(tracks);
}

const Section* SectionFor(const std::vector<Section>& sections, int id)
{
	const auto found = std::find_if(sections.begin(), sections.end(), [id](const Section& section) { return section.id == id; });

	return found == sections.end() ? nullptr : &*found;
}

std::string Renamed(const std::string& text, const Section& donor, int number, const std::string& newline)
{
	std::string body = text.substr(donor.start, donor.end - donor.start);

	const size_t blank = body.find(newline + newline);

	if (blank != std::string::npos)
		body.erase(blank + newline.size());

	if (body.empty() || body.back() != '\n')
		body += newline;

	char header[16] = {};
	sprintf_s(header, "%s%03d]", kTrackHeader, number);

	body.replace(0, body.find(']') + 1, header);
	return body;
}

class BgListOverlay : public IFileOverlay
{
public:
	bool Covers(const std::string& key) const override { return key == kListKey; }
	uint32_t Version() const override { return StageRevision::Current(); }

	bool Apply(const std::string&, std::vector<uint8_t>& content) const override
	{
		std::string list = Text(content);
		std::vector<Span> spans;
		std::vector<int> listed;
		size_t listAt = 0;
		size_t listEnd = 0;

		BgListText::Blocks(list, spans);

		if (spans.empty() || !BgListText::SelectList(list, listed, listAt, listEnd))
		{
			LOG("StageOverlays: the stage list has no stage block or no BgSelectList, so it is left as it is");
			return false;
		}

		const Span& templateSpan = TemplateOf(spans);
		Learn(list, spans, listed, templateSpan);

		std::vector<int> order = listed;
		std::vector<Edit> edits;
		std::vector<StageThumbs::Card> thumbs;
		StageThumbs::Assign(thumbs);

		ReworkOwn(list, spans, thumbs, order, edits);

		const std::string added = AddInstalled(list, templateSpan, thumbs, order);

		if (!added.empty())
			edits.push_back({ spans.back().end, 0, added });

		if (order != listed)
			edits.push_back({ listAt, listEnd - listAt, BgListText::JoinList(order) });

		g_listBytes = static_cast<uint32_t>(list.size());

		if (edits.empty())
			return false;

		ApplyEdits(list, edits);
		Store(list, content);

		g_listBytes = static_cast<uint32_t>(list.size());
		++g_listApplied;

		LOG("StageOverlays: the stage list carries %d picker entries of %d, %u bytes", static_cast<int>(order.size()),
			StagePicker::Capacity(), static_cast<unsigned>(list.size()));
		return true;
	}
};

class BgNamesOverlay : public IFileOverlay
{
public:
	bool Covers(const std::string& key) const override { return key == kNamesKey; }
	uint32_t Version() const override { return StageRevision::Current(); }

	bool Apply(const std::string&, std::vector<uint8_t>& content) const override
	{
		std::vector<Entry> entries;
		SnapshotPlaced(entries);

		std::vector<StageReplacements::Replacement> replaced;
		StageReplacements::Snapshot(replaced);

		if (entries.empty() && replaced.empty())
			return false;

		std::string names = Text(content);
		const std::string newline = BgListText::Newline(names);

		for (const Entry& entry : entries)
			SetName(names, entry.number, StageEntry::ShiftJis(entry.name), newline);

		for (const StageReplacements::Replacement& replacement : replaced)
		{
			if (replacement.renamed)
				SetName(names, replacement.number, StageEntry::ShiftJis(replacement.name), newline);
		}

		Store(names, content);
		++g_namesApplied;
		return true;
	}
};

class StageMusicOverlay : public IFileOverlay
{
public:
	bool Covers(const std::string& key) const override { return key == kMusicKey; }
	uint32_t Version() const override { return StageRevision::Current(); }

	bool Apply(const std::string&, std::vector<uint8_t>& content) const override
	{
		std::string text = Text(content);
		const std::string newline = BgListText::Newline(text);

		std::vector<Section> sections;
		Sections(text, sections);
		LearnTracks(text, sections);

		std::vector<Entry> entries;
		SnapshotPlaced(entries);

		std::string added;

		for (const Entry& entry : entries)
		{
			const Section* const chosen = SectionFor(sections, entry.music);
			const Section* const donor = chosen != nullptr ? chosen : SectionFor(sections, StageLibrary::kDefaultMusic);

			if (SectionFor(sections, entry.number) != nullptr || donor == nullptr)
				continue;

			added += newline + Renamed(text, *donor, entry.number, newline);
		}

		if (added.empty())
			return false;

		if (!text.empty() && text.back() != '\n')
			text += newline;

		Store(text + added, content);
		++g_musicApplied;
		return true;
	}
};

struct StageImage
{
	const char* suffix;
	const uint8_t* bytes;
	size_t size;
};

constexpr uint8_t kNeutralColour[] = { 0, 0, 0, 0, 7, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
constexpr uint8_t kNeutralSpecular[] = { 0, 0, 0, 0, 7, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0 };
constexpr uint8_t kNeutralBokashi[] = { 0, 0, 0, 0, 7, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
	0x4E, 0x4E, 0x4E, 0xFF };

constexpr uint8_t kUnlitBokashi[] = { 0, 0, 0, 0, 7, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
	0x00, 0x00, 0x00, 0xFF };

constexpr StageImage kStageImages[] = {
	{ "\\stage_color.img", kNeutralColour, sizeof(kNeutralColour) },
	{ "\\stage_specular.img", kNeutralSpecular, sizeof(kNeutralSpecular) },
	{ "\\stage_bokashi_alpha.img", kNeutralBokashi, sizeof(kNeutralBokashi) },
};

constexpr StageImage kUnlitImages[] = {
	{ "\\stage_color.img", kNeutralColour, sizeof(kNeutralColour) },
	{ "\\stage_specular.img", kNeutralSpecular, sizeof(kNeutralSpecular) },
	{ "\\stage_bokashi_alpha.img", kUnlitBokashi, sizeof(kUnlitBokashi) },
};

constexpr const char* kStageFolderPrefix = "bg\\bg";
constexpr const char* kLightingFolder = "bg\\bg65535";
constexpr uint32_t kLitVersion = 1;
constexpr uint32_t kUnlitVersion = 2;

const StageImage* MatchSuffix(const StageImage* first, const StageImage* last, const std::string& key, size_t at)
{
	for (const StageImage* image = first; image != last; ++image)
	{
		if (key.compare(at, std::string::npos, image->suffix) == 0)
			return image;
	}

	return nullptr;
}

const StageImage* StageImageFor(const std::string& key)
{
	const size_t prefix = strlen(kStageFolderPrefix);

	if (key.compare(0, prefix, kStageFolderPrefix) != 0)
		return nullptr;

	size_t at = prefix;

	while (at < key.size() && isdigit(static_cast<unsigned char>(key[at])) != 0)
		++at;

	if (at == prefix)
		return nullptr;

	return MatchSuffix(std::begin(kStageImages), std::end(kStageImages), key, at);
}

const StageImage* UnlitImageFor(const std::string& key)
{
	const size_t prefix = strlen(kLightingFolder);

	if (key.compare(0, prefix, kLightingFolder) != 0)
		return nullptr;

	return MatchSuffix(std::begin(kUnlitImages), std::end(kUnlitImages), key, prefix);
}

class StageColourOverlay : public IFileOverlay
{
public:
	bool Covers(const std::string& key) const override { return StageImageFor(key) != nullptr; }
	uint32_t Version() const override { return 1; }

	bool Apply(const std::string& key, std::vector<uint8_t>& content) const override
	{
		const StageImage* const image = StageImageFor(key);

		if (!content.empty() || image == nullptr)
			return false;

		content.assign(image->bytes, image->bytes + image->size);
		LOG("StageOverlays: %s is missing, so a neutral one is served", key.c_str());
		return true;
	}
};

class StageLightingOverlay : public IFileOverlay
{
public:
	bool Covers(const std::string& key) const override { return UnlitImageFor(key) != nullptr; }
	uint32_t Version() const override { return g_settings.stageLighting ? kLitVersion : kUnlitVersion; }

	bool Apply(const std::string& key, std::vector<uint8_t>& content) const override
	{
		const StageImage* const image = UnlitImageFor(key);

		if (g_settings.stageLighting || image == nullptr)
			return false;

		content.assign(image->bytes, image->bytes + image->size);
		LOG("StageOverlays: %s is served without lighting", key.c_str());
		return true;
	}
};

struct ExportedFile
{
	const char* key;
	const char* path;
};

constexpr ExportedFile kExportedFiles[] = {
	{ kListKey, "Mods\\bg\\BgList.txt" },
	{ kNamesKey, "Mods\\bg\\BgList_str.ini" },
};

constexpr const char* kExportFolder = "Mods\\bg";

const ExportedFile* ExportFor(const std::string& key)
{
	for (const ExportedFile& file : kExportedFiles)
	{
		if (key == file.key)
			return &file;
	}

	return nullptr;
}

class StageTextExport : public IFileOverlay
{
public:
	bool Covers(const std::string& key) const override { return ExportFor(key) != nullptr; }
	uint32_t Version() const override { return 1; }

	bool Apply(const std::string& key, std::vector<uint8_t>& content) const override
	{
		const ExportedFile* const file = ExportFor(key);
		const std::string path = file != nullptr ? GetModRootPath(file->path) : std::string();

		if (content.empty() || path.empty() || GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES)
			return false;

		if (!CreateDirectoryTree(GetModRootPath(kExportFolder)) || !WriteWholeFile(path, content.data(), content.size()))
		{
			LOG("StageOverlays: the game's own %s could not be written to %s", key.c_str(), path.c_str());
			return false;
		}

		LOG("StageOverlays: the game's own %s was written to %s", key.c_str(), path.c_str());
		return false;
	}
};

StageTextExport g_exportOverlay;
BgListOverlay g_listOverlay;
BgNamesOverlay g_namesOverlay;
StageMusicOverlay g_musicOverlay;
StageColourOverlay g_colourOverlay;
StageLightingOverlay g_lightingOverlay;

}

void StageOverlays::Register()
{
	ModFiles::AddOverlay(&g_exportOverlay);
	ModFiles::AddOverlay(&g_listOverlay);
	ModFiles::AddOverlay(&g_namesOverlay);
	ModFiles::AddOverlay(&g_musicOverlay);
	ModFiles::AddOverlay(&g_colourOverlay);
	ModFiles::AddOverlay(&g_lightingOverlay);
}

StageOverlays::Stats StageOverlays::Snapshot()
{
	Stats stats = {};
	stats.listBytes = g_listBytes.load();
	stats.listApplied = g_listApplied.load();
	stats.namesApplied = g_namesApplied.load();
	stats.musicApplied = g_musicApplied.load();

	return stats;
}
