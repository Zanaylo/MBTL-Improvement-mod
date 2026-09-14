#pragma once

#include <cstdint>
#include <string>

namespace StageImport
{
	struct Offer
	{
		std::string folder;
		std::string name;
		uint32_t bytes;
	};

	bool Scan(const char* folder);

	const char* ScannedGame();
	int OfferCount();
	const Offer* OfferAt(int index);

	bool InstallMany(const int* indices, const char* const* names, int count);
	bool InstallFolder(const char* folder, const char* name);
	bool ReplaceFolder(const char* folder, int number);
	bool Remove(int number);
	bool Restore(int number);

	bool SetInGame(int number, bool inGame);
	bool Unlock(int number, bool unlocked);

	void Update();

	bool IsBusy();
	int Progress();
	bool NeedsRestart();

	const char* StatusText();
}
