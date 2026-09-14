#include "Game/ModFiles.h"

#include "Core/FileIndex.h"
#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Core/utils.h"
#include "D3D9/DeviceHooks.h"
#include "Game/FileCache.h"
#include "Game/GameOffsets.h"
#include "Game/MemoryMap.h"
#include "Game/ModPacks.h"
#include "Hooks/HookManager.h"

#include <windows.h>

#include <cstdio>
#include <unordered_set>

namespace {

namespace Files = GameOffsets::Files;

using ReaderOpen_t = int(__fastcall*)(void*, void*, const char*, int, int, int);
using ReaderFromMemory_t = int(__fastcall*)(void*, void*, const uint8_t*, uint32_t);
using ReaderLoad_t = int(__fastcall*)(void*, void*, int, int, int);
using FileExists_t = int(__cdecl*)(const char*);

constexpr DWORD kSettleMs = 500;
constexpr int kWatchedFolders = 2;
constexpr uint64_t kVersionMix = 0x100000001B3ull;
constexpr const char* kComposedSuffix = "|composed";

struct OpenRequest
{
	void* reader;
	const char* path;
	int first;
	int second;
	int third;
};

ReaderOpen_t oReaderOpen = nullptr;
FileExists_t oFileExists = nullptr;
ReaderFromMemory_t g_fromMemory = nullptr;
ReaderLoad_t g_load = nullptr;

FileIndex g_files;
SRWLOCK g_filesLock = SRWLOCK_INIT;
std::string g_root;
FileCache g_cache;
std::vector<IFileOverlay*> g_overlays;

SRWLOCK g_missedLock = SRWLOCK_INIT;
std::unordered_set<std::string> g_missed;

volatile long g_served = 0;
volatile long g_own = 0;
char g_status[160] = "not started";

HANDLE g_watch[kWatchedFolders] = { INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE };
DWORD g_stirredAt = 0;
bool g_stirred = false;
bool g_packsStirred = false;

class FrameListener : public IDeviceListener
{
public:
	void OnPresent(IDirect3DDevice9*) override { ModFiles::OnFrame(); }
};

FrameListener g_listener;

void EraseAll(std::string& text, const char* pattern, size_t keep)
{
	const size_t length = std::strlen(pattern);

	for (size_t at = text.find(pattern); at != std::string::npos; at = text.find(pattern, at))
		text.erase(at + keep, length - keep);
}

std::string Normalise(const char* path)
{
	std::string key = FileIndex::Key(path, std::strlen(path));

	EraseAll(key, "\\.\\", 1);
	EraseAll(key, "\\\\", 1);

	while (key.compare(0, 2, ".\\") == 0)
		key.erase(0, 2);

	while (!key.empty() && key.front() == '\\')
		key.erase(0, 1);

	return key;
}

bool FindOnDisk(const std::string& key, std::string& out)
{
	AcquireSRWLockShared(&g_filesLock);

	const std::string* const found = g_files.Find(key);

	if (found)
		out = *found;

	ReleaseSRWLockShared(&g_filesLock);
	return found != nullptr;
}

std::vector<const IFileOverlay*> OverlaysFor(const std::string& key)
{
	std::vector<const IFileOverlay*> covering;

	for (const IFileOverlay* overlay : g_overlays)
	{
		if (overlay->Covers(key))
			covering.push_back(overlay);
	}

	return covering;
}

bool FolderIsModded(const std::string& key)
{
	const size_t slash = key.rfind('\\');

	if (slash == std::string::npos)
		return false;

	AcquireSRWLockShared(&g_filesLock);

	bool modded = false;

	for (const FileIndex::Map::value_type& entry : g_files.Entries())
	{
		if (entry.first.compare(0, slash + 1, key, 0, slash + 1) != 0)
			continue;

		modded = true;
		break;
	}

	ReleaseSRWLockShared(&g_filesLock);
	return modded;
}

void NoteMissing(const std::string& key, const char* requested)
{
	if (!g_settings.logMissingFiles || !FolderIsModded(key))
		return;

	AcquireSRWLockExclusive(&g_missedLock);
	const bool first = g_missed.insert(key).second;
	ReleaseSRWLockExclusive(&g_missedLock);

	if (first)
		LOG("Not in Mods: %s (the game asked for \"%s\")", key.c_str(), requested);
}

bool ReadGameFile(const OpenRequest& request, const std::string& path, std::vector<uint8_t>& out)
{
	if (!g_load || !oReaderOpen(request.reader, nullptr, path.c_str(), request.first, request.second, request.third))
		return false;

	if (!g_load(request.reader, nullptr, 0, 0, 0))
		return false;

	uint32_t data = 0;
	uint32_t size = 0;
	const uintptr_t reader = reinterpret_cast<uintptr_t>(request.reader);

	if (!TryRead(reader + Files::kReaderData, data) || !TryRead(reader + Files::kReaderSize, size) || !data || !size)
		return false;

	out.resize(size);
	return TryReadMemory(out.data(), reinterpret_cast<const void*>(static_cast<uintptr_t>(data)), size);
}

uint64_t VersionOf(const std::vector<const IFileOverlay*>& overlays, uint64_t seed)
{
	uint64_t version = seed;

	for (const IFileOverlay* overlay : overlays)
		version = (version ^ overlay->Version()) * kVersionMix;

	return version;
}

const std::vector<uint8_t>* Compose(const OpenRequest& request, const std::string& key,
	const std::vector<const IFileOverlay*>& overlays, const std::string* disk)
{
	const std::string cacheKey = key + kComposedSuffix;
	const uint64_t stamp = VersionOf(overlays, disk ? FileCache::StampOf(*disk) : 1);
	const std::vector<uint8_t>* const cached = g_cache.Find(cacheKey, stamp);

	if (cached)
		return cached;

	std::vector<uint8_t> content;
	const bool based = disk ? ReadWholeFile(*disk, content)
		: ReadGameFile(request, overlays.front()->BasePath(key, request.path), content);

	if (!based)
		LOG("ModFiles: %s has no base content to extend", key.c_str());

	bool changed = false;

	for (const IFileOverlay* overlay : overlays)
		changed = overlay->Apply(key, content) || changed;

	if (content.empty())
		return nullptr;

	if (g_settings.logServedFiles)
		LOG("Serving %s built from %s (%u bytes, %s)", key.c_str(), disk ? disk->c_str() : "the game's own",
			static_cast<unsigned>(content.size()), changed ? "extended" : "unchanged");

	return g_cache.Keep(cacheKey, stamp, std::move(content));
}

int Serve(void* reader, const std::vector<uint8_t>* data)
{
	InterlockedIncrement(&g_served);
	return g_fromMemory(reader, nullptr, data->data(), static_cast<uint32_t>(data->size()));
}

bool AnyChanges(const std::vector<const IFileOverlay*>& overlays, const std::string& key)
{
	for (const IFileOverlay* overlay : overlays)
	{
		if (overlay->Changes(key))
			return true;
	}

	return false;
}

int OpenOrLend(const OpenRequest& request, void* unused, const std::string& key, const IFileOverlay& overlay)
{
	const int opened = oReaderOpen(request.reader, unused, request.path, request.first, request.second, request.third);

	if (opened)
		return opened;

	const std::string base = overlay.BasePath(key, request.path);

	if (base == request.path)
		return opened;

	return oReaderOpen(request.reader, unused, base.c_str(), request.first, request.second, request.third);
}

int __fastcall HookedReaderOpen(void* reader, void* unused, const char* path, int first, int second, int third)
{
	if (!path || !*path)
		return oReaderOpen(reader, unused, path, first, second, third);

	const std::string key = Normalise(path);
	std::string disk;
	const bool onDisk = FindOnDisk(key, disk);
	const std::vector<const IFileOverlay*> overlays = OverlaysFor(key);

	if (!onDisk && overlays.empty())
	{
		NoteMissing(key, path);
		return oReaderOpen(reader, unused, path, first, second, third);
	}

	const std::string redirected = overlays.empty() ? std::string(path) : overlays.front()->Redirect(key, path);

	if (redirected != path)
		return HookedReaderOpen(reader, unused, redirected.c_str(), first, second, third);

	const OpenRequest request = { reader, path, first, second, third };

	if (!onDisk && !AnyChanges(overlays, key))
		return OpenOrLend(request, unused, key, *overlays.front());
	const std::vector<uint8_t>* const data = overlays.empty() ? g_cache.Get(key, disk)
		: Compose(request, key, overlays, onDisk ? &disk : nullptr);

	if (!data || data->empty())
		return oReaderOpen(reader, unused, path, first, second, third);

	return Serve(reader, data);
}

int __cdecl HookedFileExists(const char* path)
{
	if (!path || !*path)
		return oFileExists(path);

	const std::string key = Normalise(path);
	std::string disk;

	if (FindOnDisk(key, disk) || !OverlaysFor(key).empty())
		return 1;

	const int found = oFileExists(path);

	if (!found)
		NoteMissing(key, path);

	return found;
}

void Rebuild()
{
	FileIndex own;
	own.Walk(g_root);

	FileIndex built;
	ModPacks::Layer(built);

	for (const FileIndex::Map::value_type& entry : own.Entries())
		built.Add(entry.first, entry.second);

	InterlockedExchange(&g_own, own.Count());

	AcquireSRWLockExclusive(&g_filesLock);
	g_files.Swap(built);
	const int count = g_files.Count();
	ReleaseSRWLockExclusive(&g_filesLock);

	AcquireSRWLockExclusive(&g_missedLock);
	g_missed.clear();
	ReleaseSRWLockExclusive(&g_missedLock);

	sprintf_s(g_status, "%d file(s) in use", count);
}

void Watch(int which, const std::string& folder)
{
	g_watch[which] = FindFirstChangeNotificationA(folder.c_str(), TRUE, FILE_NOTIFY_CHANGE_FILE_NAME |
		FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE);

	if (g_watch[which] == INVALID_HANDLE_VALUE)
		LOG("ModFiles: %s cannot be watched, so a new file there needs Look again", folder.c_str());
}

bool Stirred()
{
	bool stirred = false;

	for (int i = 0; i < kWatchedFolders; ++i)
	{
		if (g_watch[i] == INVALID_HANDLE_VALUE || WaitForSingleObject(g_watch[i], 0) != WAIT_OBJECT_0)
			continue;

		FindNextChangeNotification(g_watch[i]);
		g_packsStirred = g_packsStirred || i == 1;
		stirred = true;
	}

	if (!stirred)
		return false;

	g_stirredAt = GetTickCount();
	g_stirred = true;
	return true;
}

}

bool ModFiles::Initialize()
{
	const GameFunctions& functions = MemoryMap::Functions();
	if (!functions.readerOpen || !functions.readerFromMemory)
	{
		LOG("ModFiles: the game's file reader is unknown, so Mods are not loaded");
		return false;
	}

	g_root = GetModRootPath("Mods");
	CreateDirectoryA(g_root.c_str(), nullptr);

	ModPacks::Scan();
	Rebuild();

	Watch(0, g_root);
	Watch(1, ModPacks::Root());

	g_fromMemory = reinterpret_cast<ReaderFromMemory_t>(functions.readerFromMemory);
	g_load = reinterpret_cast<ReaderLoad_t>(functions.readerLoad);

	if (!HookManager::CreateHook(functions.readerOpen, reinterpret_cast<void*>(&HookedReaderOpen),
		reinterpret_cast<void**>(&oReaderOpen), "reader open"))
	{
		return false;
	}

	DeviceHooks::AddListener(&g_listener);
	LOG("ModFiles: %s, from %s", g_status, g_root.c_str());

	if (!functions.fileExists)
		return true;

	HookManager::CreateHook(functions.fileExists, reinterpret_cast<void*>(&HookedFileExists),
		reinterpret_cast<void**>(&oFileExists), "file exists");
	return true;
}

void ModFiles::AddOverlay(IFileOverlay* overlay)
{
	if (overlay == nullptr)
		return;

	g_overlays.push_back(overlay);
}

bool ModFiles::Find(const char* gamePath, std::string& out)
{
	return gamePath != nullptr && FindOnDisk(Normalise(gamePath), out);
}

void ModFiles::Rescan()
{
	g_stirred = false;

	if (g_packsStirred)
	{
		g_packsStirred = false;
		ModPacks::Scan();
	}

	Rebuild();
	LOG("ModFiles: %s", g_status);
}

void ModFiles::OnFrame()
{
	if (Stirred())
		return;

	if (!g_stirred || GetTickCount() - g_stirredAt < kSettleMs)
		return;

	LOG("ModFiles: a mod folder changed");
	Rescan();
}

int ModFiles::Count()
{
	AcquireSRWLockShared(&g_filesLock);
	const int count = g_files.Count();
	ReleaseSRWLockShared(&g_filesLock);
	return count;
}

int ModFiles::OwnCount()
{
	return static_cast<int>(g_own);
}

int ModFiles::Served()
{
	return static_cast<int>(g_served);
}

const char* ModFiles::Root()
{
	return g_root.c_str();
}

const char* ModFiles::StatusText()
{
	return g_status;
}
