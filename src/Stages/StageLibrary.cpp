#include "Stages/StageLibrary.h"

#include "Core/Settings.h"
#include "Core/logger.h"
#include "Core/utils.h"
#include "Stages/GameStages.h"
#include "Stages/StageArchive.h"
#include "Stages/StageRevision.h"
#include "Stages/StageTable.h"

#include <windows.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

using Entry = StageLibrary::Entry;

constexpr const char* kKeyPrefix = "Lib";
constexpr const char* kFolderPrefix = "bg";
constexpr const char* kNote = "stage.txt";
constexpr const char* kModel = "bg.fbx.bin";
constexpr char kSeparator = '|';

SRWLOCK g_lock = SRWLOCK_INIT;
std::vector<Entry> g_entries;

std::string KeyOf(int number)
{
	char key[16] = {};
	sprintf_s(key, "%s%d", kKeyPrefix, number);

	return key;
}

std::string Clean(const std::string& text)
{
	std::string out = text;
	std::replace(out.begin(), out.end(), kSeparator, ' ');

	return out;
}

void Save(const Entry& entry)
{
	char head[64] = {};
	sprintf_s(head, "%d%c%d%c%d%c%d%c", entry.shown ? 1 : 0, kSeparator, entry.removed ? 1 : 0, kSeparator,
		entry.card, kSeparator, entry.music, kSeparator);

	const std::string value = head + Clean(entry.game) + kSeparator + Clean(entry.folder) + kSeparator +
		Clean(entry.name);

	Settings::SaveString(StageLibrary::kSection, KeyOf(entry.number).c_str(), value.c_str());
}

void Forget(int number)
{
	Settings::SaveString(StageLibrary::kSection, KeyOf(number).c_str(), nullptr);
}

std::string NextField(const std::string& text, size_t& at)
{
	const size_t bar = text.find(kSeparator, at);
	const std::string field = bar == std::string::npos ? text.substr(at < text.size() ? at : text.size())
		: text.substr(at, bar - at);

	at = bar == std::string::npos ? text.size() : bar + 1;
	return field;
}

int NumberAfter(const std::string& text, const char* prefix)
{
	const size_t length = strlen(prefix);

	if (text.size() <= length || _strnicmp(text.c_str(), prefix, length) != 0)
		return -1;

	for (size_t at = length; at < text.size(); ++at)
	{
		if (isdigit(static_cast<unsigned char>(text[at])) == 0)
			return -1;
	}

	return atoi(text.c_str() + length);
}

int ClampCard(int card)
{
	return card < StageLibrary::kTemplateCard ? StageLibrary::kTemplateCard : card;
}

bool Parse(const std::string& line, Entry& out)
{
	const size_t equals = line.find('=');

	if (equals == std::string::npos)
		return false;

	out.number = NumberAfter(line.substr(0, equals), kKeyPrefix);

	if (out.number <= 0)
		return false;

	const std::string value = line.substr(equals + 1);
	size_t at = 0;

	out.shown = atoi(NextField(value, at).c_str()) != 0;
	out.removed = atoi(NextField(value, at).c_str()) != 0;
	out.card = ClampCard(atoi(NextField(value, at).c_str()));
	out.music = atoi(NextField(value, at).c_str());
	out.music = out.music > 0 ? out.music : StageLibrary::kDefaultMusic;
	out.game = NextField(value, at);
	out.folder = NextField(value, at);
	out.name = at < value.size() ? value.substr(at) : std::string();

	return true;
}

bool FolderExists(int number)
{
	const DWORD attributes = GetFileAttributesA(StageLibrary::FolderOf(number).c_str());

	return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool Known(const std::vector<Entry>& entries, int number)
{
	return std::any_of(entries.begin(), entries.end(), [number](const Entry& entry) { return entry.number == number; });
}

void ReadEntries(std::vector<Entry>& out)
{
	const std::string section = Settings::ReadSection(StageLibrary::kSection);

	for (const char* at = section.c_str(); at < section.c_str() + section.size() && *at != '\0'; at += strlen(at) + 1)
	{
		Entry entry = {};

		if (Parse(at, entry) && !Known(out, entry.number))
			out.push_back(entry);
	}
}

void ReadNote(Entry& entry)
{
	std::vector<uint8_t> blob;

	if (!ReadWholeFile(StageLibrary::NoteOf(entry.number), blob))
		return;

	const std::string note(blob.begin(), blob.end());
	std::string value;

	if (StageArchive::Field(note, "Name", value))
		entry.name = StageArchive::Unquoted(value);

	if (StageArchive::Field(note, "From", value))
		entry.game = StageArchive::Unquoted(value);

	if (StageArchive::Field(note, "Source", value))
		entry.folder = StageArchive::Unquoted(value);
}

void Sweep(std::vector<Entry>& entries)
{
	std::vector<Entry> kept;

	for (const Entry& entry : entries)
	{
		if (entry.removed)
			StageLibrary::DeleteFiles(entry.number);

		if (!entry.removed && FolderExists(entry.number))
		{
			kept.push_back(entry);
			continue;
		}

		Forget(entry.number);
		LOG("StageLibrary: stage %d '%s' %s", entry.number, entry.name.c_str(),
			entry.removed ? "was removed and its files are deleted" : "is no longer on disk and left the list");
	}

	entries.swap(kept);
}

void Adopt(std::vector<Entry>& entries)
{
	WIN32_FIND_DATAA found = {};
	const HANDLE search = FindFirstFileA((StageLibrary::Root() + "\\bg*").c_str(), &found);

	if (search == INVALID_HANDLE_VALUE)
		return;

	do
	{
		if ((found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			continue;

		const int number = NumberAfter(found.cFileName, kFolderPrefix);

		if (number <= 0 || StageLibrary::Reserved(number) || GameStages::Owns(number) || Known(entries, number))
			continue;

		if (GetFileAttributesA((StageLibrary::FolderOf(number) + "\\" + kModel).c_str()) == INVALID_FILE_ATTRIBUTES)
			continue;

		Entry entry = {};
		entry.number = number;
		entry.shown = true;
		entry.card = StageLibrary::kTemplateCard;
		entry.music = StageLibrary::kDefaultMusic;
		entry.name = found.cFileName;

		ReadNote(entry);
		Save(entry);
		entries.push_back(entry);

		LOG("StageLibrary: adopted bg%03d, '%s', which the list did not mention", number, entry.name.c_str());
	}
	while (FindNextFileA(search, &found) != 0);

	FindClose(search);
}

void SortByNumber(std::vector<Entry>& entries)
{
	std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) { return a.number < b.number; });
}

void CollectFree(const std::vector<int>& taken, size_t wanted, std::vector<int>& out)
{
	const int numbers = StageTable::Numbers();

	AcquireSRWLockShared(&g_lock);
	const std::vector<Entry> entries = g_entries;
	ReleaseSRWLockShared(&g_lock);

	for (int number = 1; number < numbers && out.size() < wanted; ++number)
	{
		if (StageLibrary::Reserved(number) || Known(entries, number) || GameStages::Owns(number))
			continue;

		if (std::find(taken.begin(), taken.end(), number) != taken.end() || FolderExists(number))
			continue;

		out.push_back(number);
	}
}

template <typename Fn>
bool Change(int number, Fn change)
{
	AcquireSRWLockExclusive(&g_lock);

	bool changed = false;

	for (Entry& entry : g_entries)
	{
		if (entry.number != number)
			continue;

		changed = change(entry);

		if (changed)
			Save(entry);

		break;
	}

	ReleaseSRWLockExclusive(&g_lock);

	if (changed)
		StageRevision::Bump();

	return changed;
}

}

void StageLibrary::Load()
{
	std::vector<Entry> entries;

	ReadEntries(entries);
	Sweep(entries);
	Adopt(entries);
	SortByNumber(entries);

	AcquireSRWLockExclusive(&g_lock);
	g_entries.swap(entries);
	ReleaseSRWLockExclusive(&g_lock);

	LOG("StageLibrary: %d stage(s) installed in %s", Count(), Root().c_str());
}

void StageLibrary::Snapshot(std::vector<Entry>& out)
{
	AcquireSRWLockShared(&g_lock);
	out = g_entries;
	ReleaseSRWLockShared(&g_lock);
}

bool StageLibrary::Of(int number, Entry& out)
{
	AcquireSRWLockShared(&g_lock);

	const auto found = std::find_if(g_entries.begin(), g_entries.end(),
		[number](const Entry& entry) { return entry.number == number; });
	const bool known = found != g_entries.end();

	if (known)
		out = *found;

	ReleaseSRWLockShared(&g_lock);

	return known;
}

bool StageLibrary::Installed(int number)
{
	Entry entry = {};

	return Of(number, entry) && !entry.removed;
}

int StageLibrary::Count()
{
	AcquireSRWLockShared(&g_lock);
	const int count = static_cast<int>(std::count_if(g_entries.begin(), g_entries.end(),
		[](const Entry& entry) { return !entry.removed; }));
	ReleaseSRWLockShared(&g_lock);

	return count;
}

int StageLibrary::ShownCount()
{
	AcquireSRWLockShared(&g_lock);
	const int count = static_cast<int>(std::count_if(g_entries.begin(), g_entries.end(),
		[](const Entry& entry) { return entry.shown && !entry.removed; }));
	ReleaseSRWLockShared(&g_lock);

	return count;
}

bool StageLibrary::Reserved(int number)
{
	return number <= kRandomStage || number == kTrainingStage || number == kDebugStage;
}

int StageLibrary::FreeNumber(const std::vector<int>& taken)
{
	std::vector<int> found;
	CollectFree(taken, 1, found);

	return found.empty() ? -1 : found.front();
}

void StageLibrary::FreeNumbers(std::vector<int>& out)
{
	out.clear();
	CollectFree(std::vector<int>(), static_cast<size_t>(StageTable::Numbers()), out);
}

void StageLibrary::Put(const Entry& entry)
{
	AcquireSRWLockExclusive(&g_lock);

	g_entries.erase(std::remove_if(g_entries.begin(), g_entries.end(),
		[&entry](const Entry& known) { return known.number == entry.number; }), g_entries.end());
	g_entries.push_back(entry);
	SortByNumber(g_entries);
	Save(entry);

	ReleaseSRWLockExclusive(&g_lock);

	StageRevision::Bump();
}

void StageLibrary::Erase(int number)
{
	AcquireSRWLockExclusive(&g_lock);

	g_entries.erase(std::remove_if(g_entries.begin(), g_entries.end(),
		[number](const Entry& entry) { return entry.number == number; }), g_entries.end());
	Forget(number);

	ReleaseSRWLockExclusive(&g_lock);

	StageRevision::Bump();
}

bool StageLibrary::Show(int number, bool shown)
{
	return Change(number, [shown](Entry& entry)
	{
		if (entry.shown == shown)
			return false;

		entry.shown = shown;
		return true;
	});
}

bool StageLibrary::SetCard(int number, int card)
{
	const int clamped = ClampCard(card);

	return Change(number, [clamped](Entry& entry)
	{
		if (entry.card == clamped)
			return false;

		entry.card = clamped;
		return true;
	});
}

bool StageLibrary::SetMusic(int number, int music)
{
	return Change(number, [music](Entry& entry)
	{
		if (music <= 0 || entry.music == music)
			return false;

		entry.music = music;
		return true;
	});
}

void StageLibrary::DeleteFiles(int number)
{
	const std::string folder = FolderOf(number);

	WIN32_FIND_DATAA found = {};
	const HANDLE search = FindFirstFileA((folder + "\\*").c_str(), &found);

	if (search != INVALID_HANDLE_VALUE)
	{
		do
		{
			if ((found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
				DeleteFileA((folder + "\\" + found.cFileName).c_str());
		}
		while (FindNextFileA(search, &found) != 0);

		FindClose(search);
	}

	RemoveDirectoryA(folder.c_str());
}

std::string StageLibrary::Root()
{
	return GetModRootPath("Mods\\bg");
}

std::string StageLibrary::FolderOf(int number)
{
	char leaf[16] = {};
	sprintf_s(leaf, "\\bg%03d", number);

	return Root() + leaf;
}

std::string StageLibrary::NoteOf(int number)
{
	return FolderOf(number) + "\\" + kNote;
}
