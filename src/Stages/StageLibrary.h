#pragma once

#include <string>
#include <vector>

namespace StageLibrary
{
	constexpr int kRandomStage = 0;
	constexpr int kTrainingStage = 90;
	constexpr int kDebugStage = 99;
	constexpr int kTemplateCard = -1;
	constexpr int kDefaultMusic = 1;
	constexpr const char* kSection = "StageLibrary";

	struct Entry
	{
		int number;
		bool shown;
		bool removed;
		int card;
		int music;
		std::string game;
		std::string folder;
		std::string name;
	};

	void Load();

	void Snapshot(std::vector<Entry>& out);
	bool Of(int number, Entry& out);
	bool Installed(int number);

	int Count();
	int ShownCount();

	bool Reserved(int number);
	int FreeNumber(const std::vector<int>& taken);
	void FreeNumbers(std::vector<int>& out);

	void Put(const Entry& entry);
	void Erase(int number);

	bool Show(int number, bool shown);
	bool SetCard(int number, int card);
	bool SetMusic(int number, int music);

	void DeleteFiles(int number);

	std::string Root();
	std::string FolderOf(int number);
	std::string NoteOf(int number);
}
