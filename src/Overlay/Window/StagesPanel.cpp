#include "Overlay/Window/StagesPanel.h"

#include "Core/Settings.h"
#include "Core/interfaces.h"
#include "Game/GameRestart.h"
#include "Overlay/ComboNav.h"
#include "Overlay/UiScale.h"
#include "Overlay/UiText.h"
#include "Stages/CharacterLight.h"
#include "Stages/HiddenStages.h"
#include "Stages/StageBloom.h"
#include "Stages/StageCards.h"
#include "Stages/StageImport.h"
#include "Stages/StagePicker.h"
#include "Stages/StageRevision.h"
#include "Stages/StageTable.h"
#include "Stages/StageThumbs.h"

#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>

namespace {

using Entry = StageLibrary::Entry;

constexpr float kStrengthWidth = 200.0f;
constexpr int kMostLight = 100;
constexpr int kMostBloom = 200;

void StrengthSlider(const char* label, int* value, int most, const char* key, bool available, const char* status,
	const char* help)
{
	if (!available)
	{
		UiText::Warn("%s: %s", label, status);
		return;
	}

	Ui::SetItemWidth(kStrengthWidth);
	ImGui::SliderInt(label, value, 0, most, "%d%%");

	if (ImGui::IsItemDeactivatedAfterEdit())
		Settings::SaveInt("Stages", key, *value);

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("%s", help);
}

constexpr float kListHeight = 260.0f;
constexpr float kNumberColumn = 44.0f;
constexpr float kFolderColumn = 90.0f;
constexpr float kSizeColumn = 80.0f;
constexpr float kPickerColumn = 64.0f;
constexpr float kCardColumn = 130.0f;
constexpr float kMusicColumn = 190.0f;
constexpr float kActionColumn = 70.0f;
constexpr float kCheckboxGap = 24.0f;
constexpr float kProgressWidth = 260.0f;
constexpr float kPercent = 100.0f;
constexpr float kReplaceWidth = 260.0f;
constexpr int kHiddenPerLine = 3;
constexpr int kFirstSheetLast = 20;

constexpr ImGuiTableFlags kTableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
	ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY;

void CardLabel(int card, int templateCard, char* out, size_t size)
{
	if (card < 0)
	{
		sprintf_s(out, size, "Default (%d)", templateCard);
		return;
	}

	sprintf_s(out, size, "%d, sheet %d", card, card <= kFirstSheetLast ? 1 : 2);
}

void OwnLabel(const GameStages::Own& own, char* out, size_t size)
{
	sprintf_s(out, size, "%03d  %s", own.number, own.name.c_str());
}

void TrackLabel(int id, const std::vector<GameStages::Track>& tracks, char* out, size_t size)
{
	const auto found = std::find_if(tracks.begin(), tracks.end(), [id](const GameStages::Track& track) { return track.id == id; });

	sprintf_s(out, size, "%03d  %s", id, found == tracks.end() ? "not in bgm.txt" : found->file.c_str());
}

void Megabytes(uint32_t bytes, char* out, size_t size)
{
	sprintf_s(out, size, "%.1f MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
}

}

void StagesPanel::Draw()
{
	TakeDialogs();
	PumpQueue();
	Refresh();

	if (ImGui::BeginTabBar("##stagetabs"))
	{
		if (ImGui::BeginTabItem("Installed stages"))
		{
			DrawInstalled();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Add stages"))
		{
			DrawAdd();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Help"))
		{
			DrawHelp();
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	DrawRestart();
}

void StagesPanel::Refresh()
{
	const uint32_t revision = StageRevision::Current();
	const bool learned = GameStages::Learned() && GameStages::TracksLearned();

	if (revision == m_revision && learned == m_learned)
		return;

	m_revision = revision;
	m_learned = learned;

	StageLibrary::Snapshot(m_entries);
	GameStages::Snapshot(m_own);
	GameStages::HiddenSnapshot(m_hidden);
	GameStages::TrackSnapshot(m_tracks);
	HiddenStages::Snapshot(m_unlocked);
	StageReplacements::Snapshot(m_replaced);
	StageThumbs::Assign(m_thumbs);
	m_lastCard = StageThumbs::LastCard();

	m_own.erase(std::remove_if(m_own.begin(), m_own.end(),
		[](const GameStages::Own& own) { return own.number == StageLibrary::kRandomStage; }), m_own.end());

	if (m_replaceNumber == 0 && !m_own.empty())
		m_replaceNumber = m_own.front().number;

	std::vector<int> free;
	StageLibrary::FreeNumbers(free);

	m_freeNumbers = static_cast<int>(free.size());
	m_pickerUsed = StagePicker::Used();
	m_pickerCapacity = StagePicker::Capacity();
	m_templateCard = GameStages::TemplateCard();
}

void StagesPanel::TakeDialogs()
{
	std::string picked;

	if (m_sourceDialog.TakeResult(picked) && !picked.empty())
	{
		m_queue.clear();
		StageImport::Scan(picked.c_str());
		BuildRows();
	}

	picked.clear();

	if (m_folderDialog.TakeResult(picked) && !picked.empty())
		StageImport::InstallFolder(picked.c_str(), nullptr);

	picked.clear();

	if (m_replaceDialog.TakeResult(picked) && !picked.empty())
		StageImport::ReplaceFolder(picked.c_str(), m_replaceNumber);
}

void StagesPanel::BuildRows()
{
	m_rows.assign(static_cast<size_t>(StageImport::OfferCount()), Row());

	for (int i = 0; i < StageImport::OfferCount(); ++i)
		strncpy_s(m_rows[i].name, StageImport::OfferAt(i)->name.c_str(), _TRUNCATE);
}

void StagesPanel::PumpQueue()
{
	if (m_queue.empty() || StageImport::IsBusy())
		return;

	std::vector<int> indices;
	std::vector<const char*> names;

	for (int index : m_queue)
	{
		if (index < 0 || index >= static_cast<int>(m_rows.size()))
			continue;

		indices.push_back(index);
		names.push_back(m_rows[index].name);
	}

	m_queue.clear();

	if (!indices.empty())
		StageImport::InstallMany(indices.data(), names.data(), static_cast<int>(indices.size()));
}

void StagesPanel::Queue(int index)
{
	if (!Queued(index))
		m_queue.push_back(index);
}

bool StagesPanel::Queued(int index) const
{
	return std::find(m_queue.begin(), m_queue.end(), index) != m_queue.end();
}

bool StagesPanel::Unlocked(int number) const
{
	return std::find(m_unlocked.begin(), m_unlocked.end(), number) != m_unlocked.end();
}

void StagesPanel::DrawInstalled()
{
	DrawStageTable();
	DrawLighting();
	DrawHidden();
	DrawReplaced();
	DrawLibrary();
}

void StagesPanel::DrawReplaced()
{
	if (m_replaced.empty())
		return;

	ImGui::SeparatorText("Replaced game stages");

	for (const StageReplacements::Replacement& replacement : m_replaced)
	{
		ImGui::PushID(replacement.number);
		ImGui::BeginDisabled(StageImport::IsBusy());

		if (ImGui::SmallButton("Restore"))
			StageImport::Restore(replacement.number);

		ImGui::EndDisabled();

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Deletes every file in %s, so the game's own stage comes back.",
				StageLibrary::FolderOf(replacement.number).c_str());

		ImGui::SameLine();
		ImGui::Text("%03d  %s", replacement.number, replacement.name.c_str());
		ImGui::PopID();
	}
}

void StagesPanel::DrawLighting()
{
	ImGui::SeparatorText("Lighting");

	if (ImGui::Checkbox("Stage lighting on characters", &g_settings.stageLighting))
		Settings::SaveBool("Stages", "CharacterLighting", g_settings.stageLighting);

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("The game lights the characters from bg\\bg65535 on every stage it loads, imported ones included, "
			"and has no option to turn that off. Untick this and the mod hands the game the same plain files the "
			"no-lighting mod ships: white colour, no glow. Nothing on disk changes, and it applies from the next stage "
			"the game loads.");

	StrengthSlider("Character light", &g_settings.lightStrength, kMostLight, "LightStrength", CharacterLight::IsAvailable(),
		CharacterLight::StatusText(), "How much of the stage's light reaches the characters, mid-match. 100% is the game as "
		"it ships; 0% draws them in plain white light with no glow.");

	StrengthSlider("Stage bloom", &g_settings.bloomStrength, kMostBloom, "BloomStrength", StageBloom::IsAvailable(),
		StageBloom::StatusText(), "Scales the stage's own bloom, mid-match. 100% is the stage as it ships, 0% turns it "
		"off, and above 100% brightens it up to the game's own ceiling.");
}

void StagesPanel::DrawStageTable()
{
	ImGui::SeparatorText("Stage table");
	UiText::Muted("%s", StageTable::StatusText());

	UiText::Help("MBTL has room for stage numbers 0 to 99, and most numbers below 36 are its own stages. The mod "
		"moves the stage table to a bigger one at startup, so higher numbers work too.");

	UiText::Muted("Stage picker: %d of %d entries used. %d free stage number(s).", m_pickerUsed, m_pickerCapacity,
		m_freeNumbers);
}

void StagesPanel::DrawHidden()
{
	ImGui::SeparatorText("Hidden stages");

	if (m_hidden.empty())
	{
		UiText::Muted("The game hides no stage.");
		return;
	}

	for (size_t i = 0; i < m_hidden.size(); ++i)
	{
		const GameStages::Own& own = m_hidden[i];

		if (i % kHiddenPerLine != 0)
			ImGui::SameLine(0.0f, Ui::Scaled(kCheckboxGap));

		ImGui::PushID(own.number);

		bool unlocked = Unlocked(own.number);

		if (ImGui::Checkbox(own.name.c_str(), &unlocked))
			StageImport::Unlock(own.number, unlocked);

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Stage %d%s%s", own.number, own.listed ? "" : ", left out of the picker",
				own.selectDisabled ? ", marked not selectable" : "");

		ImGui::PopID();
	}
}

void StagesPanel::DrawLibrary()
{
	ImGui::SeparatorText("Installed stages");

	if (m_entries.empty())
	{
		UiText::Muted("None yet. Take one from another game on the Add stages tab.");
		return;
	}

	if (!ImGui::BeginTable("##stages", 7, kTableFlags, ImVec2(0.0f, Ui::Scaled(kListHeight))))
		return;

	ImGui::TableSetupColumn("No.", ImGuiTableColumnFlags_WidthFixed, Ui::Scaled(kNumberColumn));
	ImGui::TableSetupColumn("Name");
	ImGui::TableSetupColumn("Source");
	ImGui::TableSetupColumn("In picker", ImGuiTableColumnFlags_WidthFixed, Ui::Scaled(kPickerColumn));
	ImGui::TableSetupColumn("Card", ImGuiTableColumnFlags_WidthFixed, Ui::Scaled(kCardColumn));
	ImGui::TableSetupColumn("Music", ImGuiTableColumnFlags_WidthFixed, Ui::Scaled(kMusicColumn));
	ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, Ui::Scaled(kActionColumn));
	ImGui::TableSetupScrollFreeze(0, 1);
	ImGui::TableHeadersRow();

	for (const Entry& entry : m_entries)
		DrawEntry(entry);

	ImGui::EndTable();
}

void StagesPanel::DrawEntry(const Entry& entry)
{
	const bool locked = StageImport::IsBusy();

	ImGui::PushID(entry.number);
	ImGui::TableNextRow();

	ImGui::TableNextColumn();
	ImGui::Text("%d", entry.number);

	ImGui::TableNextColumn();
	ImGui::TextUnformatted(entry.name.c_str());

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("%s", StageLibrary::FolderOf(entry.number).c_str());

	if (entry.number >= StageTable::Numbers())
		UiText::Warn("needs the extension table");

	ImGui::TableNextColumn();
	ImGui::TextWrapped("%s, %s", entry.game.c_str(), entry.folder.c_str());

	ImGui::TableNextColumn();
	ImGui::BeginDisabled(locked);

	bool shown = entry.shown;

	if (ImGui::Checkbox("##picker", &shown))
		StageImport::SetInGame(entry.number, shown);

	ImGui::EndDisabled();

	ImGui::TableNextColumn();
	DrawCard(entry);

	ImGui::TableNextColumn();
	DrawMusic(entry);

	ImGui::TableNextColumn();
	ImGui::BeginDisabled(locked);

	if (ImGui::SmallButton("Remove"))
		StageImport::Remove(entry.number);

	ImGui::EndDisabled();
	ImGui::PopID();
}

void StagesPanel::DrawCard(const Entry& entry)
{
	const int thumb = StageThumbs::CardIn(m_thumbs, entry.number);

	if (thumb >= 0)
	{
		ImGui::Text("thumbnail (%d)", thumb);

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("The thumbnail.png or thumbnail.dds in this stage's folder is its card. Delete it to pick "
				"a card here.");

		return;
	}

	char preview[40] = {};
	CardLabel(entry.card, m_templateCard, preview, sizeof(preview));

	ImGui::SetNextItemWidth(-1.0f);
	ImGui::BeginDisabled(StageImport::IsBusy());

	if (ImGui::BeginCombo("##card", preview))
	{
		for (int card = StageLibrary::kTemplateCard; card <= m_lastCard; ++card)
		{
			char label[40] = {};
			CardLabel(card, m_templateCard, label, sizeof(label));

			const bool selected = card == entry.card;

			if (ImGui::Selectable(label, selected))
				StageLibrary::SetCard(entry.number, card);

			ComboNav::KeepSelectedInView(selected);
		}

		ImGui::EndCombo();
	}

	ImGui::EndDisabled();

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("The picture the stage picker shows: 0 to 20 on stage_thumb00, 21 and up on stage_thumb01, seven "
			"to a row of 144x336. A taller stage_thumb01.dds in MBTL-IM\\Mods\\grpdat\\CSel adds rows, 1024x2048 reaching "
			"card 62.\n%s", StageCards::StatusText());
}

void StagesPanel::DrawMusic(const Entry& entry)
{
	char preview[64] = {};
	TrackLabel(entry.music, m_tracks, preview, sizeof(preview));

	ImGui::SetNextItemWidth(-1.0f);
	ImGui::BeginDisabled(StageImport::IsBusy());

	if (ImGui::BeginCombo("##music", preview))
	{
		for (const GameStages::Track& track : m_tracks)
		{
			char label[64] = {};
			TrackLabel(track.id, m_tracks, label, sizeof(label));

			const bool selected = track.id == entry.music;

			if (ImGui::Selectable(label, selected))
				StageLibrary::SetMusic(entry.number, track.id);

			ComboNav::KeepSelectedInView(selected);
		}

		ImGui::EndCombo();
	}

	ImGui::EndDisabled();

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("The battle music. The stage gets a copy of this bgm.txt entry under its own number.");
}

void StagesPanel::DrawAdd()
{
	DrawSource();
	ImGui::Spacing();
	DrawCustom();
	ImGui::Spacing();
	DrawReplace();
}

void StagesPanel::DrawSource()
{
	ImGui::SeparatorText("From another game");

	ImGui::BeginDisabled(StageImport::IsBusy() || m_sourceDialog.IsRunning());

	if (ImGui::Button("Pick a game folder..."))
		m_sourceDialog.BeginFolder("Pick the folder a game is installed in");

	ImGui::EndDisabled();

	UiText::Help("Supported games: UNDER NIGHT IN-BIRTH II Sys:Celes, UNDER NIGHT IN-BIRTH Exe:Late[st] and "
		"[cl-r], UNDER NIGHT IN-BIRTH Exe:Late and DENGEKI BUNKO FIGHTING CLIMAX IGNITION. The mod reads that "
		"game's files and replaces nothing in MBTL.");

	if (!StageImport::IsBusy())
	{
		UiText::Muted("%s", StageImport::StatusText());
		DrawOffers();
		return;
	}

	ImGui::ProgressBar(StageImport::Progress() / kPercent, ImVec2(Ui::Scaled(kProgressWidth), 0.0f));

	if (m_queue.empty())
		UiText::Warn("%s", StageImport::StatusText());
	else
		UiText::Warn("%s (%d more queued)", StageImport::StatusText(), static_cast<int>(m_queue.size()));
}

void StagesPanel::DrawOffers()
{
	if (StageImport::OfferCount() == 0)
		return;

	if (ImGui::Button("Add all"))
	{
		for (int i = 0; i < StageImport::OfferCount(); ++i)
			Queue(i);
	}

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Installs every stage listed under the names shown. Any the picker has no room for are "
			"installed out of it.");

	ImGui::SameLine();
	UiText::Muted("Edit a name before adding it if you like.");

	if (!ImGui::BeginTable("##stageoffers", 4, kTableFlags, ImVec2(0.0f, Ui::Scaled(kListHeight))))
		return;

	ImGui::TableSetupColumn("Folder", ImGuiTableColumnFlags_WidthFixed, Ui::Scaled(kFolderColumn));
	ImGui::TableSetupColumn("Name");
	ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, Ui::Scaled(kSizeColumn));
	ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, Ui::Scaled(kActionColumn));
	ImGui::TableSetupScrollFreeze(0, 1);
	ImGui::TableHeadersRow();

	for (int i = 0; i < StageImport::OfferCount(); ++i)
		DrawOfferRow(i);

	ImGui::EndTable();
}

void StagesPanel::DrawOfferRow(int index)
{
	const StageImport::Offer* const offer = StageImport::OfferAt(index);

	if (offer == nullptr || index >= static_cast<int>(m_rows.size()))
		return;

	ImGui::PushID(index);
	ImGui::TableNextRow();

	ImGui::TableNextColumn();
	ImGui::TextUnformatted(offer->folder.c_str());

	ImGui::TableNextColumn();
	ImGui::SetNextItemWidth(-1.0f);
	ImGui::InputText("##name", m_rows[index].name, sizeof(m_rows[index].name));

	ImGui::TableNextColumn();

	char size[32] = {};
	Megabytes(offer->bytes, size, sizeof(size));
	ImGui::TextUnformatted(size);

	ImGui::TableNextColumn();

	if (Queued(index))
		ImGui::TextDisabled("queued");
	else if (ImGui::SmallButton("Add"))
		Queue(index);

	ImGui::PopID();
}

void StagesPanel::DrawCustom()
{
	ImGui::SeparatorText("A stage folder of your own");

	ImGui::BeginDisabled(StageImport::IsBusy() || m_folderDialog.IsRunning());

	if (ImGui::Button("Import a stage folder..."))
		m_folderDialog.BeginFolder("Pick the folder holding bg.fbx.bin");

	ImGui::EndDisabled();

	UiText::Help("Pick a folder with bg.fbx.bin and its textures.\n\nstage.txt (optional) can hold a Name and any "
		"BgList value: camera, fog, bloom, shadows, StageSelTex and so on. Write them as a plain list or as a "
		"whole Bg_NNN = { } block. Every value is used except StageW, which stays at the game's value so walls "
		"match online.\n\nthumbnail.png or thumbnail.dds (optional) becomes the stage select card.");
}

void StagesPanel::DrawReplace()
{
	ImGui::SeparatorText("In place of one of the game's stages");

	const auto chosen = std::find_if(m_own.begin(), m_own.end(),
		[this](const GameStages::Own& own) { return own.number == m_replaceNumber; });

	char preview[96] = {};

	if (chosen != m_own.end())
		OwnLabel(*chosen, preview, sizeof(preview));

	Ui::SetItemWidth(kReplaceWidth);

	if (ImGui::BeginCombo("##replace", preview))
	{
		for (const GameStages::Own& own : m_own)
		{
			char label[96] = {};
			OwnLabel(own, label, sizeof(label));

			const bool selected = own.number == m_replaceNumber;

			if (ImGui::Selectable(label, selected))
				m_replaceNumber = own.number;

			ComboNav::KeepSelectedInView(selected);
		}

		ImGui::EndCombo();
	}

	ImGui::SameLine();
	ImGui::BeginDisabled(StageImport::IsBusy() || m_replaceDialog.IsRunning() || chosen == m_own.end());

	if (ImGui::Button("Replace it with a stage folder..."))
		m_replaceDialog.BeginFolder("Pick the folder holding bg.fbx.bin");

	ImGui::EndDisabled();

	UiText::Help("The stage keeps its number, card and music, so it needs no free number. A player without it gets "
		"the game's stage instead of crashing.\n\nFiles go to MBTL-IM\\Mods\\bg\\bgNNN and stage.txt works the "
		"same way. A stage.txt alone in that folder changes the game's stage values and keeps its model.");
}

void StagesPanel::DrawHelp()
{
	ImGui::SeparatorText("Adding a stage");
	ImGui::TextWrapped("Pick the folder another French-Bread game is installed in and choose its stages, or import a "
		"folder of your own. Each stage is copied into MBTL-IM\\Mods\\bg under a free stage number. Nothing is "
		"downloaded, and no file of the game is changed on disk.");

	ImGui::SeparatorText("How it reaches the game");
	ImGui::TextWrapped("When the game reads its stage list, the mod adds a block for each installed stage, built from "
		"stage 1's, and puts the stage in the picker. The stage also gets a name, a copy of a battle music entry and "
		"a pre-battle background lent from stage 1.");

	ImGui::SeparatorText("Hidden stages");
	ImGui::TextWrapped("Stages the game builds but leaves out of the picker. Ticking one puts it in.");

	ImGui::SeparatorText("The picker");
	ImGui::TextWrapped("The picker has a fixed number of entries, shared by the game's stages and yours. In picker "
		"is which installed stages take one. Card is the picture it shows, Music the battle track.");

	ImGui::SeparatorText("Restarting");
	ImGui::TextWrapped("The game reads its stage list once, as it starts, so every change here shows after a "
		"restart. Remove deletes the stage's folder and takes it off the list at once; do not pick that stage before "
		"restarting.");

	ImGui::SeparatorText("Online");
	ImGui::TextWrapped("Both players need the same stages installed under the same numbers: the game crashes for a "
		"player who does not have the stage picked. A stage put in place of one of the game's own has no such "
		"problem. The player without it gets the game's stage, and the walls stay the game's so the match stays in "
		"sync.");
}

void StagesPanel::DrawRestart()
{
	if (!StageImport::NeedsRestart())
		return;

	ImGui::Separator();
	UiText::Warn("The game reads its stage list once, as it starts. Restart the game to see what changed here.");

	if (!GameRestart::CanSoftReset())
	{
		UiText::Muted("%s", GameRestart::StatusText());
		return;
	}

	ImGui::BeginDisabled(GameRestart::IsPending());

	if (ImGui::Button("Restart the game"))
		GameRestart::SoftReset();

	ImGui::EndDisabled();
}
