#include "Music/BgmTable.h"

#include "Core/utils.h"
#include "Music/MusicAnchors.h"

#include <windows.h>

#include <cstring>

namespace {

namespace Music = GameOffsets::Music;

constexpr int kStarted = 1;
constexpr int32_t kPresent = 1;
constexpr int32_t kAbsent = 0;

bool WriteMemory(uintptr_t address, const void* source, size_t size)
{
	if (address == 0)
		return false;

	__try
	{
		std::memcpy(reinterpret_cast<void*>(address), source, size);
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
}

uintptr_t SlotAddress(int id)
{
	const uintptr_t table = MusicAnchors::Get().table;

	if (table == 0 || !BgmTable::IsValidId(id))
		return 0;

	return table + static_cast<uintptr_t>(id) * Music::kSlotBytes;
}

template <typename T>
T Field(const uint8_t* raw, uintptr_t offset)
{
	T value{};
	std::memcpy(&value, raw + offset, sizeof(T));
	return value;
}

template <typename T>
void SetField(uint8_t* raw, uintptr_t offset, const T& value)
{
	std::memcpy(raw + offset, &value, sizeof(T));
}

template <typename T>
bool WriteField(int id, uintptr_t offset, const T& value)
{
	const uintptr_t slot = SlotAddress(id);
	return slot != 0 && WriteMemory(slot + offset, &value, sizeof(T));
}

int ReadGlobal(uintptr_t address, int fallback)
{
	int value = fallback;

	if (address == 0 || !TryRead(address, value))
		return fallback;

	return value;
}

}

bool BgmTable::IsReady()
{
	return MusicAnchors::Get().table != 0;
}

bool BgmTable::IsValidId(int id)
{
	return id >= 0 && id < Music::kSlotCount;
}

bool BgmTable::Read(int id, Slot& out)
{
	const uintptr_t slot = SlotAddress(id);
	uint8_t raw[Music::kSlotBytes] = {};

	if (slot == 0 || !TryReadMemory(raw, reinterpret_cast<const void*>(slot), sizeof(raw)))
		return false;

	out.present = Field<int32_t>(raw, Music::kSlotPresent) != kAbsent;
	out.loop = Field<uint8_t>(raw, Music::kSlotLoop) != 0;
	out.loopPosition = Field<double>(raw, Music::kSlotLoopPosition);
	out.volume = Field<int32_t>(raw, Music::kSlotVolume);

	std::memcpy(out.file, raw + Music::kSlotFile, Music::kSlotFileBytes);
	out.file[Music::kSlotFileBytes] = '\0';
	return true;
}

bool BgmTable::IsPresent(int id)
{
	const uintptr_t slot = SlotAddress(id);
	int32_t present = kAbsent;

	return slot != 0 && TryRead(slot + Music::kSlotPresent, present) && present != kAbsent;
}

bool BgmTable::ReadVolume(int id, int& out)
{
	const uintptr_t slot = SlotAddress(id);
	return slot != 0 && TryRead(slot + Music::kSlotVolume, out);
}

bool BgmTable::WriteVolume(int id, int value)
{
	return WriteField<int32_t>(id, Music::kSlotVolume, value);
}

bool BgmTable::WriteTrack(int id, const char* file, bool loop, double loopPosition)
{
	const uintptr_t slot = SlotAddress(id);

	if (slot == 0 || file == nullptr)
		return false;

	uint8_t raw[Music::kSlotBytes] = {};

	SetField<int32_t>(raw, Music::kSlotPresent, kPresent);
	SetField<uint8_t>(raw, Music::kSlotLoop, loop ? 1 : 0);
	SetField<double>(raw, Music::kSlotLoopPosition, loopPosition);
	SetField<int32_t>(raw, Music::kSlotVolume, Music::kFullVolume);
	strncpy_s(reinterpret_cast<char*>(raw + Music::kSlotFile), Music::kSlotFileBytes, file, _TRUNCATE);

	return WriteMemory(slot, raw, sizeof(raw));
}

bool BgmTable::WriteLoop(int id, bool loop, double loopPosition)
{
	return WriteField<double>(id, Music::kSlotLoopPosition, loopPosition) &&
		WriteField<uint8_t>(id, Music::kSlotLoop, loop ? 1 : 0);
}

bool BgmTable::Clear(int id)
{
	return WriteField<int32_t>(id, Music::kSlotPresent, kAbsent);
}

int BgmTable::CurrentId()
{
	return ReadGlobal(MusicAnchors::Get().currentId, kNoTrack);
}

int BgmTable::Playing()
{
	if (!HasStream() || !IsStarted())
		return kNoTrack;

	return CurrentId();
}

bool BgmTable::HasStream()
{
	return ReadGlobal(MusicAnchors::Get().stream, 0) != 0;
}

bool BgmTable::IsLoaded()
{
	return ReadGlobal(MusicAnchors::Get().loaded, 0) != 0;
}

bool BgmTable::IsStarted()
{
	return ReadGlobal(MusicAnchors::Get().state, kStarted) == kStarted;
}

bool BgmTable::IsMuted()
{
	return ReadGlobal(MusicAnchors::Get().muted, 0) != 0;
}

int BgmTable::TrackVolume()
{
	return ReadGlobal(MusicAnchors::Get().trackVolume, Music::kFullVolume);
}

int BgmTable::BaseVolume()
{
	return ReadGlobal(MusicAnchors::Get().baseVolume, 0);
}

bool BgmTable::SetTrackVolume(int value)
{
	return WriteMemory(MusicAnchors::Get().trackVolume, &value, sizeof(value));
}
