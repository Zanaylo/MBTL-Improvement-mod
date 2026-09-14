#include "Music/UserTracks.h"

#include "Core/logger.h"
#include "Core/utils.h"
#include "Game/ModFiles.h"
#include "Music/BgmCatalog.h"
#include "Music/BgmTable.h"
#include "Music/MusicIni.h"

#include <windows.h>

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

namespace Music = GameOffsets::Music;

constexpr const char* kSection = "Tracks";
constexpr const char* kFolder = "Mods\\Bgm";
constexpr const char* kExtension = ".ogg";
constexpr const char* kPrefix = "user_";
constexpr const char* kFallbackStem = "track";
constexpr char kSeparator = '|';
constexpr size_t kHeadBytes = 64;
constexpr size_t kSegmentCountAt = 26;
constexpr size_t kFirstPacketAt = 27;
constexpr size_t kSuffixBytes = 3;
constexpr int kFirstSuffix = 2;
constexpr int kLastSuffix = 99;
constexpr unsigned char kAsciiEnd = 0x80;
constexpr bool kDefaultLoop = true;
constexpr double kDefaultLoopPosition = 0.0;
constexpr const char kPageMagic[] = { 'O', 'g', 'g', 'S' };
constexpr const char kVorbisMagic[] = { 0x01, 'v', 'o', 'r', 'b', 'i', 's' };

std::vector<UserTracks::Track> g_tracks;
SRWLOCK g_lock = SRWLOCK_INIT;
volatile long g_version = 1;

std::string PathOf(const std::string& file)
{
	return UserTracks::Root() + "\\" + file + kExtension;
}

bool Exists(const std::string& path)
{
	return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

bool Report(char* status, size_t statusSize, const char* text)
{
	strncpy_s(status, statusSize, text, _TRUNCATE);
	LOG("UserTracks: %s", text);
	return false;
}

size_t ReadHead(const std::string& path, uint8_t* out, size_t size)
{
	FILE* handle = nullptr;

	if (fopen_s(&handle, path.c_str(), "rb") != 0 || handle == nullptr)
		return 0;

	const size_t read = fread(out, 1, size, handle);
	fclose(handle);
	return read;
}

const char* OggProblem(const std::string& path)
{
	uint8_t head[kHeadBytes] = {};
	const size_t read = ReadHead(path, head, sizeof(head));

	if (read < kFirstPacketAt)
		return "could not read that file, or it is too short";

	if (std::memcmp(head, kPageMagic, sizeof(kPageMagic)) != 0)
		return "that is not an OGG file. Convert it to OGG Vorbis first";

	const size_t packet = kFirstPacketAt + head[kSegmentCountAt];

	if (packet + sizeof(kVorbisMagic) > read || std::memcmp(head + packet, kVorbisMagic, sizeof(kVorbisMagic)) != 0)
		return "that OGG file is not Vorbis audio (Opus and FLAC do not play). Convert it to OGG Vorbis";

	return nullptr;
}

std::string Stem(const std::string& path)
{
	const size_t slash = path.find_last_of("\\/");
	const std::string leaf = slash == std::string::npos ? path : path.substr(slash + 1);
	const size_t dot = leaf.rfind('.');

	return dot == std::string::npos ? leaf : leaf.substr(0, dot);
}

std::string Sanitised(const std::string& stem, size_t most)
{
	std::string name;

	for (size_t i = 0; i < stem.size() && name.size() < most; ++i)
	{
		const unsigned char byte = static_cast<unsigned char>(stem[i]);
		const bool plain = byte < kAsciiEnd && (std::isalnum(byte) || byte == '_' || byte == '-');

		name.push_back(plain ? static_cast<char>(std::tolower(byte)) : '_');
	}

	return name.empty() ? std::string(kFallbackStem) : name;
}

bool Taken(const std::string& file)
{
	for (const UserTracks::Track& track : g_tracks)
	{
		if (_stricmp(track.file.c_str(), file.c_str()) == 0)
			return true;
	}

	return Exists(PathOf(file));
}

std::string FreeFileName(const std::string& source)
{
	const size_t room = Music::kSlotFileBytes - 1 - std::strlen(kPrefix);
	const std::string first = kPrefix + Sanitised(Stem(source), room);

	if (!Taken(first))
		return first;

	const std::string base = kPrefix + Sanitised(Stem(source), room - kSuffixBytes);

	for (int suffix = kFirstSuffix; suffix <= kLastSuffix; ++suffix)
	{
		char candidate[Music::kSlotFileBytes] = {};
		sprintf_s(candidate, "%s_%d", base.c_str(), suffix);

		if (!Taken(candidate))
			return candidate;
	}

	return std::string();
}

int FreeId()
{
	for (int id = Music::kSlotCount - 1; id >= UserTracks::kLowestId; --id)
	{
		if (UserTracks::Owns(id) || BgmTable::IsPresent(id))
			continue;

		return id;
	}

	return BgmTable::kNoTrack;
}

int IndexIn(const std::vector<UserTracks::Track>& tracks, int id)
{
	for (size_t i = 0; i < tracks.size(); ++i)
	{
		if (tracks[i].id == id)
			return static_cast<int>(i);
	}

	return -1;
}

int IndexOf(int id)
{
	return IndexIn(g_tracks, id);
}

void Save()
{
	std::vector<MusicIni::Entry> entries;

	for (const UserTracks::Track& track : g_tracks)
	{
		char value[96] = {};
		sprintf_s(value, "%s%c%d%c%.3f", track.file.c_str(), kSeparator, track.loop ? 1 : 0, kSeparator,
			track.loopPosition);
		entries.emplace_back(MusicIni::IdKey(track.id), value);
	}

	MusicIni::WriteSection(kSection, entries);
}

void Changed()
{
	InterlockedIncrement(&g_version);
	Save();
	BgmCatalog::Invalidate();
}

bool Parse(const MusicIni::Entry& entry, UserTracks::Track& out)
{
	const size_t first = entry.second.find(kSeparator);
	const size_t second = first == std::string::npos ? std::string::npos : entry.second.find(kSeparator, first + 1);

	if (second == std::string::npos)
		return false;

	out.id = atoi(entry.first.c_str());
	out.file = entry.second.substr(0, first);
	out.loop = atoi(entry.second.c_str() + first + 1) != 0;
	out.loopPosition = atof(entry.second.c_str() + second + 1);
	out.fileFound = Exists(PathOf(out.file));

	return out.id >= UserTracks::kLowestId && out.id < Music::kSlotCount && !out.file.empty() &&
		out.file.size() < Music::kSlotFileBytes;
}

}

void UserTracks::Load()
{
	std::vector<Track> loaded;

	for (const MusicIni::Entry& entry : MusicIni::ReadSection(kSection))
	{
		Track track = {};

		if (!Parse(entry, track) || IndexOf(track.id) >= 0)
			continue;

		loaded.push_back(track);

		if (!track.fileFound)
			LOG("UserTracks: BGM %03d expects %s, which is missing", track.id, PathOf(track.file).c_str());
	}

	AcquireSRWLockExclusive(&g_lock);
	g_tracks.swap(loaded);
	ReleaseSRWLockExclusive(&g_lock);

	InterlockedIncrement(&g_version);
	LOG("UserTracks: %u track(s) of your own", static_cast<unsigned>(g_tracks.size()));
}

bool UserTracks::Import(const std::string& source, char* status, size_t statusSize)
{
	if (!BgmTable::IsReady())
		return Report(status, statusSize, "adding music does not work on this game version");

	const char* const problem = OggProblem(source);

	if (problem != nullptr)
		return Report(status, statusSize, problem);

	const int id = FreeId();

	if (id == BgmTable::kNoTrack)
		return Report(status, statusSize, "music numbers 127 to 199 are all taken");

	const std::string file = FreeFileName(source);

	if (file.empty())
		return Report(status, statusSize, "no free file name for that track");

	CreateDirectoryTree(Root());

	if (!CopyFileA(source.c_str(), PathOf(file).c_str(), TRUE))
	{
		const DWORD error = GetLastError();
		sprintf_s(status, statusSize, "could not copy the file to %s (error %lu)", Root().c_str(), error);
		LOG("UserTracks: %s", status);
		return false;
	}

	const Track track = { id, file, kDefaultLoop, kDefaultLoopPosition, true };

	AcquireSRWLockExclusive(&g_lock);
	g_tracks.push_back(track);
	ReleaseSRWLockExclusive(&g_lock);

	Changed();
	ModFiles::Rescan();
	BgmTable::WriteTrack(id, file.c_str(), track.loop, track.loopPosition);

	sprintf_s(status, statusSize, "%s is track %03d, ready to play", file.c_str(), id);
	LOG("UserTracks: %s", status);
	return true;
}

void UserTracks::Remove(int id)
{
	const int index = IndexOf(id);

	if (index < 0)
		return;

	const Track removed = g_tracks[index];
	const bool live = IsLive(removed);

	AcquireSRWLockExclusive(&g_lock);
	g_tracks.erase(g_tracks.begin() + index);
	ReleaseSRWLockExclusive(&g_lock);

	Changed();

	if (live)
		BgmTable::Clear(id);

	if (!DeleteFileA(PathOf(removed.file).c_str()))
		LOG("UserTracks: %s could not be deleted (error %lu)", PathOf(removed.file).c_str(), GetLastError());

	ModFiles::Rescan();
	LOG("UserTracks: BGM %03d (%s) removed", id, removed.file.c_str());
}

void UserTracks::SetLoop(int id, bool loop, double loopPosition)
{
	const int index = IndexOf(id);

	if (index < 0)
		return;

	const double position = loopPosition < 0.0 ? 0.0 : loopPosition;
	const bool live = IsLive(g_tracks[index]);

	AcquireSRWLockExclusive(&g_lock);
	g_tracks[index].loop = loop;
	g_tracks[index].loopPosition = position;
	ReleaseSRWLockExclusive(&g_lock);

	Changed();

	if (live)
		BgmTable::WriteLoop(id, loop, position);
}

int UserTracks::Count()
{
	return static_cast<int>(g_tracks.size());
}

const UserTracks::Track& UserTracks::At(int index)
{
	return g_tracks[index];
}

bool UserTracks::Owns(int id)
{
	return IndexOf(id) >= 0;
}

bool UserTracks::IsLive(const Track& track)
{
	BgmTable::Slot slot = {};

	if (!BgmTable::Read(track.id, slot) || !slot.present)
		return false;

	return _stricmp(slot.file, track.file.c_str()) == 0;
}

std::string UserTracks::Root()
{
	return GetModRootPath(kFolder);
}

uint32_t UserTracks::Version()
{
	return static_cast<uint32_t>(g_version);
}

std::vector<UserTracks::Track> UserTracks::Snapshot()
{
	AcquireSRWLockShared(&g_lock);
	std::vector<Track> copy = g_tracks;
	ReleaseSRWLockShared(&g_lock);
	return copy;
}
