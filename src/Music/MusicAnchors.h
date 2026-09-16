#pragma once

#include <cstdint>

struct MusicAddresses
{
	uint8_t* loader = nullptr;
	uint8_t* pathBuilder = nullptr;
	uint8_t* extension = nullptr;
	uint8_t* play = nullptr;
	uint8_t* stop = nullptr;
	uint8_t* start = nullptr;
	uint8_t* setVolume = nullptr;

	uintptr_t table = 0;
	uintptr_t stream = 0;
	uintptr_t trackVolume = 0;
	uintptr_t baseVolume = 0;
	uintptr_t loaded = 0;
	uintptr_t currentId = 0;
	uintptr_t state = 0;
	uintptr_t muted = 0;
};

struct MusicAnchor
{
	const char* name;
	uintptr_t address;
	const char* note;
};

namespace MusicAnchors
{
	void Resolve();
	const MusicAddresses& Get();

	int Count();
	const MusicAnchor& At(int index);
}
