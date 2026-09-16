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
constexpr size_t kOperandBytes = 4;
constexpr uint32_t kStoredOne = 1;
constexpr uint32_t kStoredZero = 0;
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

bool Mentions(const Body& body, uintptr_t address)
{
	if (address == 0 || body.length < kOperandBytes)
		return false;

	const auto value = static_cast<uint32_t>(address);

	for (size_t i = 0; i + kOperandBytes <= body.length; ++i)
	{
		if (ImageScanner::ReadDword(body.start + i) == value)
			return true;
	}

	return false;
}

uintptr_t DataAt(const Body& body, size_t at)
{
	if (at == kNotFound || at + kOperandBytes > body.length)
		return 0;

	const uintptr_t value = ImageScanner::ReadDword(body.start + at);
	return ImageScanner::InData(value) ? value : 0;
}

std::vector<uintptr_t> GlobalsAfter(const Body& body, const uint8_t* opcode, size_t count)
{
	std::vector<uintptr_t> values;

	for (size_t at = Find(body, opcode, count); at != kNotFound; at = Find(body, opcode, count, at + 1))
	{
		const uintptr_t value = DataAt(body, at + count);

		if (value != 0 && std::find(values.begin(), values.end(), value) == values.end())
			values.push_back(value);
	}

	return values;
}

std::vector<uintptr_t> ConstantStores(const Body& body, uint32_t constant)
{
	std::vector<uintptr_t> values;

	for (size_t at = Find(body, Music::kStoreConstant, sizeof(Music::kStoreConstant)); at != kNotFound;
		at = Find(body, Music::kStoreConstant, sizeof(Music::kStoreConstant), at + 1))
	{
		if (at + Music::kConstantAt + sizeof(constant) > body.length)
			continue;
		if (std::memcmp(body.start + at + Music::kConstantAt, &constant, sizeof(constant)) != 0)
			continue;

		const uintptr_t value = DataAt(body, at + Music::kOperandAt);

		if (value != 0 && std::find(values.begin(), values.end(), value) == values.end())
			values.push_back(value);
	}

	return values;
}

uintptr_t OnlyGlobal(const std::vector<uintptr_t>& values, const char* what)
{
	if (values.size() == 1)
		return values.front();

	LOG("Music: %u candidate(s) for the %s global, expected exactly one", static_cast<unsigned>(values.size()),
		what);
	return 0;
}

uint8_t* OnlyTailJump(const Body& body, const char* what)
{
	std::vector<uint8_t*> targets;

	for (size_t at = 0; at + Music::kJumpNearLength <= body.length; ++at)
	{
		if (body.start[at] != Music::kJumpNear)
			continue;

		const auto target = const_cast<uint8_t*>(body.start + at + Music::kJumpNearLength +
			static_cast<int32_t>(ImageScanner::ReadDword(body.start + at + 1)));

		if (!ImageScanner::InCode(target, 1) || target != ImageScanner::FunctionStart(target))
			continue;
		if (std::find(targets.begin(), targets.end(), target) == targets.end())
			targets.push_back(target);
	}

	if (targets.size() == 1)
		return targets.front();

	LOG("Music: %u tail jump(s) for %s, expected exactly one", static_cast<unsigned>(targets.size()), what);
	return nullptr;
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

	const size_t at = Find(loader, Music::kClearTable, sizeof(Music::kClearTable));

	if (at == kNotFound)
	{
		LOG("Music: the bgm.txt loader no longer clears a 200-slot table, so the table is not trusted");
		return;
	}

	g_addresses.table = DataAt(loader, at + Music::kClearTableValueAt);
}

void ResolveCommands()
{
	const uint8_t* const native = ImageScanner::NativeFunction(Music::kSetNative);

	if (native == nullptr)
	{
		LOG("Music: the script native BGM_Set was not found");
		return;
	}

	const std::vector<uint8_t*> calls = ImageScanner::CallSequence(native);

	if (calls.size() < Music::kLeastSetCalls)
	{
		LOG("Music: BGM_Set makes %u call(s), expected stop and play among them",
			static_cast<unsigned>(calls.size()));
		return;
	}

	uint8_t* const stop = calls[calls.size() - 2];
	uint8_t* const play = calls[calls.size() - 1];
	uint8_t* const start = OnlyTailJump(BodyOf(native), "StartBgm");

	const std::vector<uint8_t*> extensions =
		ImageScanner::FunctionsReferencing(ImageScanner::FindString(Music::kExtensionAnchor));
	const std::vector<uint8_t*> played = ImageScanner::CallSequence(play);

	size_t extensionAt = kNotFound;

	for (size_t i = 0; i < played.size(); ++i)
	{
		if (std::find(extensions.begin(), extensions.end(), played[i]) != extensions.end())
			extensionAt = i;
	}

	if (start == nullptr || played.empty() || played.front() != stop || extensionAt == kNotFound || extensionAt == 0)
	{
		LOG("Music: PlayBgm does not stop the player and build a path as expected, so it is not hooked");
		return;
	}

	g_addresses.stop = stop;
	g_addresses.play = play;
	g_addresses.start = start;
	g_addresses.extension = played[extensionAt];
	g_addresses.pathBuilder = played[extensionAt - 1];

	const std::vector<uint8_t*> stopped = ImageScanner::CallSequence(stop);
	uint8_t* const volume = stopped.empty() ? nullptr : stopped.back();

	if (volume != nullptr && volume == OnlyTailJump(BodyOf(start), "SetBgmVolume"))
		g_addresses.setVolume = volume;
	else
		LOG("Music: StopBgm and StartBgm do not end in the same volume call, so SetBgmVolume is not hooked");
}

void ResolvePlayerGlobals()
{
	const Body play = BodyOf(g_addresses.play);
	const Body stop = BodyOf(g_addresses.stop);
	const Body start = BodyOf(g_addresses.start);

	if (play.length == 0 || stop.length == 0 || start.length == 0)
		return;

	g_addresses.muted = OnlyGlobal(GlobalsAfter(play, Music::kCompareGlobal, sizeof(Music::kCompareGlobal)),
		"BGM off");
	g_addresses.loaded = OnlyGlobal(ConstantStores(play, kStoredOne), "BGM loaded");
	g_addresses.currentId = OnlyGlobal(ConstantStores(play, Music::kNoTrack), "current BGM id");
	g_addresses.state = OnlyGlobal(ConstantStores(stop, kStoredZero), "BGM state");
	g_addresses.trackVolume = OnlyGlobal(GlobalsAfter(start, Music::kLoadEdx, sizeof(Music::kLoadEdx)),
		"BGM track volume");

	std::vector<uintptr_t> streams;

	for (uintptr_t value : GlobalsAfter(play, &Music::kStoreEax, sizeof(Music::kStoreEax)))
	{
		if (value != g_addresses.trackVolume)
			streams.push_back(value);
	}

	g_addresses.stream = OnlyGlobal(streams, "BGM stream");

	std::vector<uintptr_t> bases;

	for (uintptr_t value : GlobalsAfter(start, Music::kLoadEcx, sizeof(Music::kLoadEcx)))
	{
		if (value != g_addresses.stream)
			bases.push_back(value);
	}

	g_addresses.baseVolume = OnlyGlobal(bases, "BGM base volume");

	if (g_addresses.trackVolume != 0 && !Mentions(BodyOf(g_addresses.setVolume), g_addresses.trackVolume))
	{
		LOG("Music: SetBgmVolume does not read the track volume, so the per-track volume is not used");
		g_addresses.trackVolume = 0;
	}
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
