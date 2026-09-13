#pragma once

#include "Core/AsyncFileDialog.h"
#include "Music/BgmCatalog.h"
#include "Music/BgmTable.h"
#include "Music/UserTracks.h"

class MusicPanel
{
public:
	void Draw();

private:
	void DrawStatus();

	void DrawBrowse();
	void DrawRandomizer();
	void DrawVolumeTools();
	void DrawTrackTable();
	void DrawTrackRow(const BgmCatalog::Track& track, int playing);
	void DrawVolume(int id);

	void TakeImport();
	void DrawAddMusic();
	void DrawUserTable();
	bool DrawUserRow(const UserTracks::Track& track);

	void DrawRules();
	bool DrawRuleRow(int index);
	void DrawRuleEditor();
	void DrawTrackCombo(const char* label, int& id);
	void DrawTrackChoices(int& id);

	AsyncFileDialog m_import;
	char m_search[64] = {};
	char m_pick[64] = {};
	char m_importStatus[256] = {};
	int m_ruleFrom = BgmTable::kNoTrack;
	int m_ruleTo = BgmTable::kNoTrack;
};
