#pragma once

#include "Game/GameOffsets.h"

namespace BgmTable
{
	inline constexpr int kNoTrack = -1;

	struct Slot
	{
		bool present;
		bool loop;
		double loopPosition;
		int volume;
		char file[GameOffsets::Music::kSlotFileBytes + 1];
	};

	bool IsReady();
	bool IsValidId(int id);

	bool Read(int id, Slot& out);
	bool IsPresent(int id);
	bool ReadVolume(int id, int& out);

	bool WriteVolume(int id, int value);
	bool WriteTrack(int id, const char* file, bool loop, double loopPosition);
	bool WriteLoop(int id, bool loop, double loopPosition);
	bool Clear(int id);

	int CurrentId();
	int Playing();
	bool HasStream();
	bool IsLoaded();
	bool IsStarted();
	bool IsMuted();

	int TrackVolume();
	int BaseVolume();
	bool SetTrackVolume(int value);
}
