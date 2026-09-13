#include "Stages/StagesDebugSection.h"

#include "Overlay/UiText.h"
#include "Stages/GameStages.h"
#include "Stages/HiddenStages.h"
#include "Stages/StageLibrary.h"
#include "Stages/StageOverlays.h"
#include "Stages/StagePicker.h"
#include "Stages/StageRevision.h"
#include "Stages/StageTable.h"

#include <imgui.h>

#include <cstdio>
#include <string>
#include <vector>

namespace {

constexpr ImGuiTableFlags kTableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;

std::string Joined(const std::vector<int>& numbers)
{
	std::string text;

	for (int number : numbers)
	{
		char item[16] = {};
		sprintf_s(item, "%s%d", text.empty() ? "" : ", ", number);
		text += item;
	}

	return text.empty() ? "none" : text;
}

}

void StagesDebugSection::Draw()
{
	DrawLibrary();
	DrawNumbers();
	DrawOwn();
	DrawOverlays();
}

void StagesDebugSection::DrawLibrary()
{
	std::vector<StageLibrary::Entry> entries;
	StageLibrary::Snapshot(entries);

	ImGui::SeparatorText("Library");

	if (entries.empty())
	{
		UiText::Muted("No stage installed.");
		return;
	}

	if (!ImGui::BeginTable("##stagelibrary", 7, kTableFlags))
		return;

	ImGui::TableSetupColumn("No.");
	ImGui::TableSetupColumn("Name");
	ImGui::TableSetupColumn("Picker");
	ImGui::TableSetupColumn("Removed");
	ImGui::TableSetupColumn("Card");
	ImGui::TableSetupColumn("Music");
	ImGui::TableSetupColumn("From");
	ImGui::TableHeadersRow();

	for (const StageLibrary::Entry& entry : entries)
	{
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("%d", entry.number);
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(entry.name.c_str());
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(entry.shown ? "yes" : "no");
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(entry.removed ? "yes" : "no");
		ImGui::TableNextColumn();
		ImGui::Text("%d", entry.card);
		ImGui::TableNextColumn();
		ImGui::Text("%d", entry.music);
		ImGui::TableNextColumn();
		ImGui::Text("%s, %s", entry.game.c_str(), entry.folder.c_str());
	}

	ImGui::EndTable();
}

void StagesDebugSection::DrawNumbers()
{
	std::vector<int> free;
	StageLibrary::FreeNumbers(free);

	std::vector<int> unlocked;
	HiddenStages::Snapshot(unlocked);

	ImGui::SeparatorText("Numbers");
	ImGui::TextWrapped("Free: %s", Joined(free).c_str());
	ImGui::Text("Unlocked hidden stages: %s", Joined(unlocked).c_str());
	ImGui::Text("Stage table: %d numbers, %d list entries, %s", StageTable::Numbers(), StageTable::ListEntries(),
		StageTable::Lifted() ? "lifted" : "stock");
	ImGui::Text("Picker: %d of %d entries, %d from the game", StagePicker::Used(), StagePicker::Capacity(),
		StagePicker::GameEntries());
}

void StagesDebugSection::DrawOwn()
{
	std::vector<GameStages::Own> own;
	GameStages::Snapshot(own);

	ImGui::SeparatorText("The game's own stages");

	if (!GameStages::Learned())
	{
		UiText::Warn("The game has not read its stage list yet, so the numbers MBTL ships are assumed.");
		return;
	}

	ImGui::Text("%d stage block(s), %d listed, template card %d", static_cast<int>(own.size()),
		GameStages::ListedCount(), GameStages::TemplateCard());

	for (const GameStages::Own& stage : own)
		ImGui::Text("%3d  %s%s%s", stage.number, stage.name.c_str(), stage.listed ? "" : ", unlisted",
			stage.selectDisabled ? ", SelectDisable" : "");
}

void StagesDebugSection::DrawOverlays()
{
	const StageOverlays::Stats stats = StageOverlays::Snapshot();

	ImGui::SeparatorText("Overlays");
	ImGui::Text("Version %u%s", StageRevision::Current(), StageRevision::Changed() ? ", changed this session" : "");
	ImGui::Text("BgList.txt composed %d time(s), last %u bytes", stats.listApplied, stats.listBytes);
	ImGui::Text("BgList_str.ini extended %d time(s)", stats.namesApplied);
	ImGui::Text("bgm.txt extended %d time(s), %s", stats.musicApplied,
		GameStages::TracksLearned() ? "tracks learned" : "tracks not read yet");
	ImGui::Text("Pre-battle backgrounds lent %d time(s)", stats.vsServed);
}
