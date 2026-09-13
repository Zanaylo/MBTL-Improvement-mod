#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace UserTracks
{
	inline constexpr int kLowestId = 127;

	struct Track
	{
		int id;
		std::string file;
		bool loop;
		double loopPosition;
		bool fileFound;
	};

	void Load();

	bool Import(const std::string& source, char* status, size_t statusSize);
	void Remove(int id);
	void SetLoop(int id, bool loop, double loopPosition);

	int Count();
	const Track& At(int index);
	bool Owns(int id);
	bool IsLive(const Track& track);

	std::string Root();
	uint32_t Version();
	std::vector<Track> Snapshot();
}
