#include "Music/MusicAnchors.h"

#include "Core/logger.h"
#include "Core/utils.h"
#include "Game/Anchors.h"
#include "Game/GameOffsets.h"
#include "Hooks/ImageScanner.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace {

namespace Music = GameOffsets::Music;

constexpr size_t kNotFound = static_cast<size_t>(-1);
constexpr size_t kOpcodeBytes = 2;
constexpr size_t kOperandBytes = 4;
constexpr uint32_t kStoredOne = 1;
constexpr uint32_t kStoredZero = 0;
constexpr uint8_t kStoreEax = 0xA3;
constexpr uint8_t kAddEcx[] = { 0x81, 0xC1 };
constexpr uint8_t kStoreConstant[] = { 0xC7, 0x05 };
constexpr uint8_t kStoreEcx[] = { 0x89, 0x0D };
constexpr uint8_t kStoreEdx[] = { 0x89, 0x15 };
constexpr uint8_t kClearTable[] = { 0x68, 0x00, 0x32, 0x00, 0x00 };
constexpr uint8_t kBoundCheck[] = { 0x81, 0x7D, 0x08, 0xC8, 0x00, 0x00, 0x00 };
constexpr uint8_t kFullVolumeCheck[] = { 0x81, 0x7D, 0x0C, 0x10, 0x27, 0x00, 0x00 };
constexpr uint8_t kGlobalGetterHead[] = { 0x55, 0x8B, 0xEC, 0xA1 };
constexpr uint8_t kGlobalGetterTail[] = { 0x5D, 0xC3 };
constexpr size_t kSetStop = 1;
constexpr size_t kSetPlay = 2;
constexpr size_t kSetStart = 3;
constexpr size_t kStopGetCurrent = 0;
constexpr size_t kStopMuted = 1;
constexpr size_t kStopSetVolume = 3;
constexpr size_t kStopCalls = 5;
constexpr int kMostAnchors = 16;

struct Body
{
	const uint8_t* start;
	size_t length;
};

MusicAddresses g_addresses;
MusicAnchor g_anchors[kMostAnchors] = {};
int g_anchorCount = 0;

uintptr_t AddressOf(const uint8_t* address)
{
	return reinterpret_cast<uintptr_t>(address);
}

Body BodyOf(const uint8_t* function)
{
	if (function == nullptr)
		return { nullptr, 0 };

	return { function, ImageScanner::FunctionLength(function) };
}

size_t Find(const Body& body, const uint8_t* bytes, size_t count, size_t from = 0)
{
	for (size_t i = from; i + count <= body.length; ++i)
	{
		if (std::memcmp(body.start + i, bytes, count) == 0)
			return i;
	}

	return kNotFound;
}

size_t After(size_t at, size_t skip)
{
	return at == kNotFound ? kNotFound : at + skip;
}

int Count(const Body& body, const uint8_t* bytes, size_t count)
{
	int found = 0;

	for (size_t at = Find(body, bytes, count); at != kNotFound; at = Find(body, bytes, count, at + 1))
		++found;

	return found;
}

bool Mentions(const Body& body, uintptr_t address)
{
	const uint32_t value = static_cast<uint32_t>(address);
	return address != 0 && Find(body, reinterpret_cast<const uint8_t*>(&value), sizeof(value)) != kNotFound;
}

uintptr_t DataAt(const Body& body, size_t at)
{
	if (at == kNotFound || at + kOperandBytes > body.length)
		return 0;

	const uintptr_t value = ImageScanner::ReadDword(body.start + at);
	return ImageScanner::InData(value) ? value : 0;
}

uintptr_t Confirmed(uintptr_t address, const Body& user, const char* what)
{
	if (Mentions(user, address))
		return address;

	LOG("Music: the %s store was not confirmed by the function that reads it, so it is not used", what);
	return 0;
}

size_t FindConstantStore(const Body& body, uint32_t value, size_t from = 0)
{
	for (size_t at = Find(body, kStoreConstant, sizeof(kStoreConstant), from); at != kNotFound;
		at = Find(body, kStoreConstant, sizeof(kStoreConstant), at + 1))
	{
		const size_t constant = at + kOpcodeBytes + kOperandBytes;

		if (constant + sizeof(value) <= body.length && std::memcmp(body.start + constant, &value, sizeof(value)) == 0)
			return at;
	}

	return kNotFound;
}

uintptr_t LastEaxStoreBefore(const Body& body, size_t before)
{
	for (size_t at = before; at-- > 0;)
	{
		if (body.start[at] != kStoreEax)
			continue;

		const uintptr_t address = DataAt(body, at + 1);

		if (address != 0)
			return address;
	}

	return 0;
}

uintptr_t GlobalGetterValue(const uint8_t* function)
{
	const size_t length = sizeof(kGlobalGetterHead) + kOperandBytes + sizeof(kGlobalGetterTail);

	if (!ImageScanner::InCode(function, length))
		return 0;

	if (std::memcmp(function, kGlobalGetterHead, sizeof(kGlobalGetterHead)) != 0)
		return 0;

	if (std::memcmp(function + sizeof(kGlobalGetterHead) + kOperandBytes, kGlobalGetterTail,
		sizeof(kGlobalGetterTail)) != 0)
	{
		return 0;
	}

	const uintptr_t value = ImageScanner::ReadDword(function + sizeof(kGlobalGetterHead));
	return ImageScanner::InData(value) ? value : 0;
}

uint8_t* Unique(const std::vector<uint8_t*>& functions, const char* what)
{
	if (functions.size() == 1)
		return functions[0];

	LOG("Music: %u candidate(s) for the %s, expected exactly one", static_cast<unsigned>(functions.size()), what);
	return nullptr;
}

void ResolveTable()
{
	g_addresses.loader = Unique(ImageScanner::FunctionsReferencing(ImageScanner::FindString(Music::kLoaderAnchor)),
		"bgm.txt loader");

	const Body loader = BodyOf(g_addresses.loader);

	if (loader.length == 0)
		return;

	if (Count(loader, kClearTable, sizeof(kClearTable)) == 0)
	{
		LOG("Music: the bgm.txt loader no longer clears a 200-slot table, so the table is not trusted");
		return;
	}

	g_addresses.table = DataAt(loader, After(Find(loader, kAddEcx, sizeof(kAddEcx)), kOpcodeBytes));
}

uint8_t* CalledAmong(const uint8_t* function, const std::vector<uint8_t*>& candidates)
{
	for (uint8_t* target : ImageScanner::CallTargets(function))
	{
		if (std::find(candidates.begin(), candidates.end(), target) != candidates.end())
			return target;
	}

	return nullptr;
}

void NoteExtensions(const std::vector<uint8_t*>& extensions)
{
	if (extensions.size() == 1)
		return;

	LOG("Music: %u function(s) reference the extension string", static_cast<unsigned>(extensions.size()));

	for (uint8_t* extension : extensions)
		LOG("Music:   %s", DescribeAddress(AddressOf(extension)).c_str());
}

void ResolvePlay()
{
	g_addresses.pathBuilder = Unique(ImageScanner::FunctionsReferencing(
		ImageScanner::FindWideString(Music::kPathBuilderAssert)), "BGM path builder");

	const std::vector<uint8_t*> extensions =
		ImageScanner::FunctionsReferencing(ImageScanner::FindString(Music::kExtensionAnchor));

	NoteExtensions(extensions);

	if (g_addresses.pathBuilder == nullptr || extensions.empty())
		return;

	std::vector<uint8_t*> players;

	for (uint8_t* caller : ImageScanner::CallerFunctions(g_addresses.pathBuilder))
	{
		if (CalledAmong(caller, extensions) != nullptr)
			players.push_back(caller);
	}

	uint8_t* const play = Unique(players, "PlayBgm");

	if (play == nullptr)
		return;

	if (Count(BodyOf(play), kBoundCheck, sizeof(kBoundCheck)) != 1)
	{
		LOG("Music: PlayBgm no longer checks its id against 200 exactly once, so it is not hooked");
		return;
	}

	g_addresses.extension = CalledAmong(play, extensions);
	g_addresses.play = play;
}

void ResolveCommands()
{
	const std::vector<uint8_t*> calls = ImageScanner::CallSequence(ImageScanner::NativeFunction(Music::kSetNative));

	if (g_addresses.play == nullptr || calls.size() <= kSetStart || calls[kSetPlay] != g_addresses.play)
	{
		LOG("Music: BGM_Set does not call stop, play, start as expected (%u call(s))",
			static_cast<unsigned>(calls.size()));
		return;
	}

	g_addresses.stop = calls[kSetStop];
	g_addresses.start = calls[kSetStart];

	const std::vector<uint8_t*> stopCalls = ImageScanner::CallSequence(g_addresses.stop);

	if (stopCalls.size() != kStopCalls)
	{
		LOG("Music: StopBgm makes %u call(s), expected %u", static_cast<unsigned>(stopCalls.size()),
			static_cast<unsigned>(kStopCalls));
		return;
	}

	g_addresses.getCurrent = stopCalls[kStopGetCurrent];
	g_addresses.muted = GlobalGetterValue(stopCalls[kStopMuted]);

	if (Count(BodyOf(stopCalls[kStopSetVolume]), kFullVolumeCheck, sizeof(kFullVolumeCheck)) != 1)
	{
		LOG("Music: SetBgmVolume does not compare its track volume with 10000, so it is not hooked");
		return;
	}

	g_addresses.setVolume = stopCalls[kStopSetVolume];
}

void ResolvePlayerGlobals()
{
	const Body play = BodyOf(g_addresses.play);
	const Body getCurrent = BodyOf(g_addresses.getCurrent);
	const size_t loadedAt = FindConstantStore(play, kStoredOne);

	if (loadedAt == kNotFound || loadedAt < kOperandBytes + 1 || play.start[loadedAt - kOperandBytes - 1] != kStoreEax)
	{
		LOG("Music: PlayBgm's closing stores were not found, so the player state is unknown");
		return;
	}

	g_addresses.loaded = DataAt(play, loadedAt + kOpcodeBytes);
	g_addresses.trackVolume = DataAt(play, loadedAt - kOperandBytes);
	g_addresses.stream = Confirmed(LastEaxStoreBefore(play, loadedAt - kOperandBytes - 1), getCurrent, "BGM stream");
	g_addresses.currentId = Confirmed(DataAt(play, After(Find(play, kStoreEcx, sizeof(kStoreEcx), loadedAt),
		kOpcodeBytes)), getCurrent, "current BGM id");

	const Body stop = BodyOf(g_addresses.stop);
	g_addresses.state = Confirmed(DataAt(stop, After(FindConstantStore(stop, kStoredZero), kOpcodeBytes)), getCurrent,
		"BGM state");

	const Body volume = BodyOf(g_addresses.setVolume);

	if (!Mentions(volume, g_addresses.trackVolume))
		return;

	g_addresses.baseVolume = DataAt(volume, After(Find(volume, kStoreEdx, sizeof(kStoreEdx)), kOpcodeBytes));
}

void Name(const char* name, uintptr_t address, const char* note)
{
	Anchors::Record(name, address, note);

	if (g_anchorCount >= kMostAnchors)
		return;

	g_anchors[g_anchorCount++] = { name, address, note };
}

void RecordAll()
{
	g_anchorCount = 0;

	Name("BGM loader", AddressOf(g_addresses.loader), "reads bgm.txt into the table");
	Name("BGM table", g_addresses.table, "200 slots of 0x40");
	Name("BGM path builder", AddressOf(g_addresses.pathBuilder), "");
	Name("BGM extension appender", AddressOf(g_addresses.extension), "");
	Name("PlayBgm", AddressOf(g_addresses.play), "the music hook");
	Name("StopBgm", AddressOf(g_addresses.stop), "needed for Stop and Play from the window");
	Name("StartBgm", AddressOf(g_addresses.start), "needed for Play from the window");
	Name("GetCurrentBgm", AddressOf(g_addresses.getCurrent), "");
	Name("SetBgmVolume", AddressOf(g_addresses.setVolume), "keeps the per-track volume");
	Name("BGM stream", g_addresses.stream, "");
	Name("BGM track volume", g_addresses.trackVolume, "");
	Name("BGM base volume", g_addresses.baseVolume, "needed to change a playing track's volume");
	Name("BGM loaded", g_addresses.loaded, "");
	Name("BGM current id", g_addresses.currentId, "");
	Name("BGM state", g_addresses.state, "");
	Name("BGM off", g_addresses.muted, "the game's music switch");
}

}

void MusicAnchors::Resolve()
{
	if (!ImageScanner::Initialize())
	{
		LOG("Music: MBTL.exe's headers could not be read, so music control is off");
		RecordAll();
		return;
	}

	ResolveTable();
	ResolvePlay();
	ResolveCommands();
	ResolvePlayerGlobals();
	RecordAll();

	LOG("Music: PlayBgm %s, table %s, stop and start %s, SetBgmVolume %s",
		g_addresses.play ? "found" : "missing", g_addresses.table ? "found" : "missing",
		g_addresses.stop ? "found" : "missing", g_addresses.setVolume ? "found" : "missing");
}

const MusicAddresses& MusicAnchors::Get()
{
	return g_addresses;
}

int MusicAnchors::Count()
{
	return g_anchorCount;
}

const MusicAnchor& MusicAnchors::At(int index)
{
	return g_anchors[index];
}
