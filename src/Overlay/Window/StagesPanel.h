#pragma once

#include "Core/AsyncFileDialog.h"
#include "Stages/GameStages.h"
#include "Stages/StageLibrary.h"
#include "Stages/StageReplacements.h"
#include "Stages/StageThumbs.h"

#include <cstdint>
#include <vector>

class StagesPanel
{
public:
	void Draw();

private:
	struct Row
	{
		char name[96];
	};

	void Refresh();
	void TakeDialogs();
	void BuildRows();
	void PumpQueue();
	void Queue(int index);
	bool Queued(int index) const;
	bool Unlocked(int number) const;

	void DrawInstalled();
	void DrawStageTable();
	void DrawLighting();
	void DrawHidden();
	void DrawReplaced();
	void DrawLibrary();
	void DrawEntry(const StageLibrary::Entry& entry);
	void DrawCard(const StageLibrary::Entry& entry);
	void DrawMusic(const StageLibrary::Entry& entry);

	void DrawAdd();
	void DrawSource();
	void DrawOffers();
	void DrawOfferRow(int index);
	void DrawCustom();
	void DrawReplace();

	void DrawHelp();
	void DrawRestart();

	AsyncFileDialog m_sourceDialog;
	AsyncFileDialog m_folderDialog;
	AsyncFileDialog m_replaceDialog;
	std::vector<Row> m_rows;
	std::vector<int> m_queue;
	std::vector<StageLibrary::Entry> m_entries;
	std::vector<GameStages::Own> m_own;
	std::vector<GameStages::Own> m_hidden;
	std::vector<GameStages::Track> m_tracks;
	std::vector<StageReplacements::Replacement> m_replaced;
	std::vector<StageThumbs::Card> m_thumbs;
	std::vector<int> m_unlocked;
	uint32_t m_revision = 0;
	bool m_learned = false;
	int m_freeNumbers = 0;
	int m_pickerUsed = 0;
	int m_pickerCapacity = 0;
	int m_templateCard = 0;
	int m_lastCard = 0;
	int m_replaceNumber = 0;
};
