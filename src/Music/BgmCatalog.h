#pragma once

#include "Game/GameOffsets.h"

namespace BgmCatalog
{
	struct Track
	{
		int id;
		bool loop;
		double loopPosition;
		char file[GameOffsets::Music::kSlotFileBytes + 1];
		char name[64];
	};

	void Refresh();
	void Invalidate();

	int Count();
	const Track& At(int index);
	const Track* Find(int id);
}
