#include "Stages/StagePicker.h"

#include "Stages/GameStages.h"
#include "Stages/HiddenStages.h"
#include "Stages/StageLibrary.h"
#include "Stages/StageTable.h"

#include <vector>

int StagePicker::Capacity()
{
	return StageTable::ListEntries() - kRandomEntries;
}

int StagePicker::GameEntries()
{
	std::vector<GameStages::Own> hidden;
	GameStages::HiddenSnapshot(hidden);

	int entries = GameStages::ListedCount();

	for (const GameStages::Own& own : hidden)
		entries += !own.listed && HiddenStages::Unlocked(own.number) ? 1 : 0;

	return entries;
}

int StagePicker::Used()
{
	return GameEntries() + StageLibrary::ShownCount();
}

int StagePicker::Room()
{
	const int room = Capacity() - Used();

	return room < 0 ? 0 : room;
}
