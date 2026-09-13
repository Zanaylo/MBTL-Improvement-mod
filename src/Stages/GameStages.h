#pragma once

#include <string>
#include <vector>

namespace GameStages
{
	struct Own
	{
		int number;
		bool listed;
		bool selectDisabled;
		std::string name;
	};

	struct Track
	{
		int id;
		std::string file;
	};

	void Learn(const std::vector<Own>& own, int listed, int templateCard);
	void LearnTracks(const std::vector<Track>& tracks);

	bool Learned();
	bool TracksLearned();

	bool Owns(int number);
	bool Hidden(const Own& own);

	void Snapshot(std::vector<Own>& out);
	void HiddenSnapshot(std::vector<Own>& out);
	void TrackSnapshot(std::vector<Track>& out);

	int ListedCount();
	int TemplateCard();
}
