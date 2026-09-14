#include "Overlay/Window/MusicPanel.h"

#include "Core/utils.h"
#include "Game/ModFiles.h"
#include "Music/BgmControl.h"
#include "Music/BgmRules.h"
#include "Music/BgmShuffle.h"
#include "Music/BgmVolume.h"
#include "Overlay/ComboNav.h"
#include "Overlay/UiScale.h"
#include "Overlay/UiText.h"

#include <imgui.h>

#include <windows.h>

#include <cctype>
#include <cfloat>
#include <cstdio>
#include <string>

namespace {

constexpr int kNoTrack = BgmTable::kNoTrack;
constexpr float kComboWidth = 340.0f;
constexpr float kSearchWidth = 240.0f;
constexpr float kBrowseHeight = 320.0f;
constexpr float kUserHeight = 240.0f;
constexpr float kRulesHeight = 200.0f;
constexpr float kDrawColumn = 40.0f;
constexpr float kIdColumn = 40.0f;
constexpr float kLoopColumn = 60.0f;
constexpr float kVolumeColumn = 120.0f;
constexpr float kPlayColumn = 44.0f;
constexpr float kLoopFromColumn = 90.0f;
constexpr float kStateColumn = 170.0f;
constexpr float kActionsColumn = 110.0f;
constexpr const char* kOggFilter = "OGG Vorbis (*.ogg)\0*.ogg\0All files\0*.*\0";
constexpr ImGuiTableFlags kListFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
	ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY;

const ImVec4 kPlayingText(0.45f, 0.90f, 0.50f, 1.0f);

bool SameLetter(char a, char b)
{
	return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
}

bool ContainsText(const char* haystack, const char* needle)
{
	if (needle[0] == '\0')
		return true;

	for (const char* start = haystack; *start != '\0'; ++start)
	{
		const char* a = start;
		const char* b = needle;

		while (*a != '\0' && *b != '\0' && SameLetter(*a, *b))
		{
			++a;
			++b;
		}

		if (*b == '\0')
			return true;
	}

	return false;
}

bool Matches(const BgmCatalog::Track& track, const char* needle)
{
	return ContainsText(track.name, needle) || ContainsText(track.file, needle);
}

void Label(const BgmCatalog::Track& track, char* out, size_t size)
{
	sprintf_s(out, size, "%03d  %s", track.id, track.name);
}

void LabelOf(int id, char* out, size_t size)
{
	const BgmCatalog::Track* const track = BgmCatalog::Find(id);

	if (track == nullptr)
	{
		sprintf_s(out, size, "%03d  (no track)", id);
		return;
	}

	Label(*track, out, size);
}

int CountMatching(const char* needle)
{
	int shown = 0;

	for (int i = 0; i < BgmCatalog::Count(); ++i)
		shown += Matches(BgmCatalog::At(i), needle) ? 1 : 0;

	return shown;
}

int CountInDraw()
{
	int inDraw = 0;

	for (int i = 0; i < BgmCatalog::Count(); ++i)
	{
		const BgmCatalog::Track& track = BgmCatalog::At(i);
		inDraw += BgmShuffle::IsInDraw(track.id, track.loop) ? 1 : 0;
	}

	return inDraw;
}

int IndexOf(int id)
{
	for (int i = 0; i < BgmCatalog::Count(); ++i)
	{
		if (BgmCatalog::At(i).id == id)
			return i;
	}

	return -1;
}

int StepTrack(int id, int steps)
{
	if (BgmCatalog::Count() == 0)
		return id;

	const int index = IndexOf(id);

	if (index < 0)
		return BgmCatalog::At(0).id;

	const int target = index + steps;

	if (target < 0 || target >= BgmCatalog::Count())
		return id;

	return BgmCatalog::At(target).id;
}

void OpenFolder(const std::string& folder)
{
	CreateDirectoryTree(folder);
	ShellExecuteA(nullptr, "open", folder.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void DrawUserState(const UserTracks::Track& track)
{
	if (!track.fileFound)
	{
		UiText::Warn("file missing");
		return;
	}

	if (!UserTracks::IsLive(track))
	{
		UiText::Warn("%03d is used by another track. Remove it and import again", track.id);
		return;
	}

	UiText::Good("ready");
}

void RemoveUserTrack(int id)
{
	if (id == BgmTable::CurrentId() || id == BgmControl::HeldId())
		BgmControl::Stop();

	BgmRules::ForgetTrack(id);
	UserTracks::Remove(id);
}

}

void MusicPanel::Draw()
{
	if (!BgmControl::IsHooked())
	{
		UiText::Warn("Music control is off: %s", BgmControl::StatusText());
		return;
	}

	BgmCatalog::Refresh();
	TakeImport();

	DrawStatus();
	ImGui::Separator();

	if (!ImGui::BeginTabBar("##music"))
		return;

	if (ImGui::BeginTabItem("Browse"))
	{
		DrawBrowse();
		ImGui::EndTabItem();
	}

	if (ImGui::BeginTabItem("Add music"))
	{
		DrawAddMusic();
		ImGui::EndTabItem();
	}

	if (ImGui::BeginTabItem("Rules"))
	{
		DrawRules();
		DrawRuleEditor();
		ImGui::EndTabItem();
	}

	ImGui::EndTabBar();
}

void MusicPanel::DrawStatus()
{
	if (BgmTable::IsMuted())
		UiText::Warn("Music is off in the game options, so nothing plays.");

	const int playing = BgmTable::Playing();
	const int held = BgmControl::HeldId();
	char name[96] = "nothing";

	if (playing != kNoTrack)
		LabelOf(playing, name, sizeof(name));

	ImGui::Text("Playing: %s%s", name, held != kNoTrack ? "  (your pick)" : "");

	ImGui::SameLine();
	ImGui::BeginDisabled(playing == kNoTrack && held == kNoTrack);

	if (ImGui::SmallButton("Stop"))
		BgmControl::Stop();

	ImGui::EndDisabled();

	if (held != kNoTrack)
	{
		ImGui::SameLine();

		if (ImGui::SmallButton("Let the game choose"))
			BgmControl::Release();
	}

	char reason[192] = {};
	BgmControl::ReasonText(reason, sizeof(reason));
	UiText::Muted("Why: %s", reason);

	if (!BgmControl::RunsOnPresent())
		UiText::Muted("%s", BgmControl::StatusText());

	if (!BgmControl::HasPending())
		return;

	char pending[96] = {};
	BgmControl::PendingText(pending, sizeof(pending));
	UiText::Muted("%s", pending);
}

void MusicPanel::DrawBrowse()
{
	DrawRandomizer();

	Ui::SetItemWidth(kSearchWidth);
	ImGui::InputTextWithHint("##search", "Search by name or file", m_search, IM_ARRAYSIZE(m_search));

	ImGui::SameLine();
	ImGui::TextDisabled("%d of %d tracks", CountMatching(m_search), BgmCatalog::Count());

	UiText::Muted("Play keeps a track on until you press Stop or Let the game choose.");

	DrawVolumeTools();
	DrawTrackTable();
}

void MusicPanel::DrawRandomizer()
{
	bool enabled = BgmShuffle::IsEnabled();

	if (ImGui::Checkbox("Randomizer", &enabled))
		BgmShuffle::SetEnabled(enabled);

	UiText::Help("Plays a random track ticked in the Draw column on screens with no rule. It stays until the game "
		"asks for new music. Tracks that do not loop start unticked.");

	ImGui::SameLine();
	ImGui::BeginDisabled(!enabled);

	if (ImGui::SmallButton("Draw again"))
		BgmControl::Redraw();

	ImGui::EndDisabled();
	ImGui::SameLine();

	if (ImGui::SmallButton("Tick all"))
		BgmShuffle::SetAllInDraw(true);

	ImGui::SameLine();

	if (ImGui::SmallButton("Untick all"))
		BgmShuffle::SetAllInDraw(false);

	ImGui::SameLine();
	ImGui::TextDisabled("%d ticked", CountInDraw());
}

void MusicPanel::DrawVolumeTools()
{
	ImGui::TextUnformatted("Volume per track");
	UiText::Help("Works on top of the game's music volume and can only make a track quieter. Saved in "
		"MBTL-IM\\Music\\music.ini.");

	if (!BgmVolume::IsHooked())
		UiText::Warn("On this game version, changing the music volume in the game options puts a quieter track "
			"back to full until the next track starts.");

	const int custom = BgmVolume::CustomCount();

	if (custom == 0)
		return;

	ImGui::SameLine();

	if (ImGui::SmallButton("Reset volumes"))
	{
		BgmVolume::ResetAll();
		BgmControl::RefreshVolume();
	}

	ImGui::SameLine();
	ImGui::TextDisabled("%d track(s) made quieter", custom);
}

void MusicPanel::DrawTrackTable()
{
	if (!ImGui::BeginTable("##tracks", 7, kListFlags, ImVec2(0.0f, Ui::Scaled(kBrowseHeight))))
		return;

	ImGui::TableSetupColumn("Draw", ImGuiTableColumnFlags_WidthFixed, Ui::Scaled(kDrawColumn));
	ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, Ui::Scaled(kIdColumn));
	ImGui::TableSetupColumn("Name");
	ImGui::TableSetupColumn("File");
	ImGui::TableSetupColumn("Loop", ImGuiTableColumnFlags_WidthFixed, Ui::Scaled(kLoopColumn));
	ImGui::TableSetupColumn("Volume", ImGuiTableColumnFlags_WidthFixed, Ui::Scaled(kVolumeColumn));
	ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, Ui::Scaled(kPlayColumn));
	ImGui::TableSetupScrollFreeze(0, 1);
	ImGui::TableHeadersRow();

	const int playing = BgmTable::Playing();

	for (int i = 0; i < BgmCatalog::Count(); ++i)
	{
		const BgmCatalog::Track& track = BgmCatalog::At(i);

		if (!Matches(track, m_search))
			continue;

		ImGui::PushID(track.id);
		DrawTrackRow(track, playing);
		ImGui::PopID();
	}

	ImGui::EndTable();
}

void MusicPanel::DrawTrackRow(const BgmCatalog::Track& track, int playing)
{
	ImGui::TableNextRow();
	ImGui::TableNextColumn();

	bool inDraw = BgmShuffle::IsInDraw(track.id, track.loop);

	if (ImGui::Checkbox("##draw", &inDraw))
		BgmShuffle::SetInDraw(track.id, inDraw);

	ImGui::TableNextColumn();
	ImGui::Text("%03d", track.id);

	ImGui::TableNextColumn();

	if (track.id == playing)
		ImGui::TextColored(kPlayingText, "%s", track.name);
	else
		ImGui::TextUnformatted(track.name);

	ImGui::TableNextColumn();
	ImGui::TextUnformatted(track.file);

	ImGui::TableNextColumn();

	if (track.loop)
		ImGui::Text("%.2fs", track.loopPosition);
	else
		ImGui::TextDisabled("no");

	ImGui::TableNextColumn();
	DrawVolume(track.id);

	ImGui::TableNextColumn();

	if (ImGui::SmallButton("Play"))
		BgmControl::Play(track.id);
}

void MusicPanel::DrawVolume(int id)
{
	int percent = BgmVolume::Percent(id);

	ImGui::SetNextItemWidth(-FLT_MIN);

	if (ImGui::SliderInt("##volume", &percent, 0, BgmVolume::kFullPercent, "%d%%"))
	{
		BgmVolume::SetPercent(id, percent);

		if (id == BgmTable::CurrentId())
			BgmControl::RefreshVolume();
	}

	if (ImGui::IsItemDeactivatedAfterEdit())
		BgmVolume::Save();
}

void MusicPanel::TakeImport()
{
	std::string picked;

	if (!m_import.TakeResult(picked) || picked.empty())
		return;

	UserTracks::Import(picked, m_importStatus, sizeof(m_importStatus));
}

void MusicPanel::DrawAddMusic()
{
	ImGui::BeginDisabled(m_import.IsRunning() || !BgmTable::IsReady());

	if (ImGui::Button("Import an OGG..."))
		m_import.BeginOpen("Choose music (OGG Vorbis)", kOggFilter);

	ImGui::EndDisabled();

	UiText::Help("Copies the file to MBTL-IM\\Mods\\Bgm with a free number (199 and down). You can play it right "
		"away and it stays after a restart. Only OGG Vorbis works: convert MP3, WAV or FLAC first, for example "
		"with Audacity.");

	ImGui::SameLine();

	if (ImGui::Button("Open the folder"))
		OpenFolder(UserTracks::Root());

	if (!BgmTable::IsReady())
		UiText::Warn("Adding music does not work on this game version.");

	if (m_importStatus[0] != '\0')
		UiText::Muted("%s", m_importStatus);

	UiText::Muted("Mods folder: %s", ModFiles::StatusText());

	if (UserTracks::Count() == 0)
	{
		UiText::Muted("You have not added any music yet.");
		return;
	}

	DrawUserTable();
}

void MusicPanel::DrawUserTable()
{
	ImGui::TextUnformatted("Your tracks");
	UiText::Help("Loop from: the second a looping track jumps back to when it ends. 0 repeats the whole track. Set "
		"it after the intro to skip the intro on repeat. Changes apply the next time the track starts.");

	if (!ImGui::BeginTable("##usertracks", 6, kListFlags, ImVec2(0.0f, Ui::Scaled(kUserHeight))))
		return;

	ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, Ui::Scaled(kIdColumn));
	ImGui::TableSetupColumn("File");
	ImGui::TableSetupColumn("Loop", ImGuiTableColumnFlags_WidthFixed, Ui::Scaled(kLoopColumn));
	ImGui::TableSetupColumn("Loop from", ImGuiTableColumnFlags_WidthFixed, Ui::Scaled(kLoopFromColumn));
	ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, Ui::Scaled(kStateColumn));
	ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, Ui::Scaled(kActionsColumn));
	ImGui::TableSetupScrollFreeze(0, 1);
	ImGui::TableHeadersRow();

	int removed = kNoTrack;

	for (int i = 0; i < UserTracks::Count(); ++i)
	{
		const UserTracks::Track& track = UserTracks::At(i);

		ImGui::PushID(track.id);

		if (DrawUserRow(track))
			removed = track.id;

		ImGui::PopID();
	}

	ImGui::EndTable();

	if (removed == kNoTrack)
		return;

	RemoveUserTrack(removed);
}

bool MusicPanel::DrawUserRow(const UserTracks::Track& track)
{
	ImGui::TableNextRow();

	ImGui::TableNextColumn();
	ImGui::Text("%03d", track.id);

	ImGui::TableNextColumn();
	ImGui::TextUnformatted(track.file.c_str());

	ImGui::TableNextColumn();

	bool loop = track.loop;

	if (ImGui::Checkbox("##loop", &loop))
		UserTracks::SetLoop(track.id, loop, track.loopPosition);

	ImGui::TableNextColumn();

	double position = track.loopPosition;

	ImGui::BeginDisabled(!track.loop);
	ImGui::SetNextItemWidth(-FLT_MIN);
	ImGui::InputDouble("##from", &position, 0.0, 0.0, "%.3f");

	if (ImGui::IsItemDeactivatedAfterEdit())
		UserTracks::SetLoop(track.id, track.loop, position);

	ImGui::EndDisabled();

	ImGui::TableNextColumn();
	DrawUserState(track);

	ImGui::TableNextColumn();
	ImGui::BeginDisabled(!track.fileFound || !UserTracks::IsLive(track));

	if (ImGui::SmallButton("Play"))
		BgmControl::Play(track.id);

	ImGui::EndDisabled();
	ImGui::SameLine();
	return ImGui::SmallButton("Remove");
}

void MusicPanel::DrawRules()
{
	UiText::Muted("A rule plays another track whenever the game plays the one you pick.");
	UiText::Help("If two rules replace the same track, the top one wins. Rules do not chain. A track you play from "
		"Browse beats every rule. The randomizer only works on screens with no rule.");

	if (BgmRules::Count() == 0)
	{
		UiText::Muted("No rules yet.");
		return;
	}

	if (!ImGui::BeginTable("##rules", 3, kListFlags, ImVec2(0.0f, Ui::Scaled(kRulesHeight))))
		return;

	ImGui::TableSetupColumn("On", ImGuiTableColumnFlags_WidthFixed, Ui::Scaled(kDrawColumn));
	ImGui::TableSetupColumn("Rule");
	ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, Ui::Scaled(kActionsColumn));
	ImGui::TableSetupScrollFreeze(0, 1);
	ImGui::TableHeadersRow();

	int removed = -1;

	for (int i = 0; i < BgmRules::Count(); ++i)
	{
		ImGui::PushID(i);

		if (DrawRuleRow(i))
			removed = i;

		ImGui::PopID();
	}

	ImGui::EndTable();

	if (removed < 0)
		return;

	BgmRules::Remove(removed);
}

bool MusicPanel::DrawRuleRow(int index)
{
	const BgmRules::Rule rule = BgmRules::At(index);
	const bool playable = BgmCatalog::Find(rule.to) != nullptr;

	ImGui::TableNextRow();
	ImGui::TableNextColumn();

	bool enabled = rule.enabled;

	if (ImGui::Checkbox("##on", &enabled))
		BgmRules::SetEnabled(index, enabled);

	ImGui::TableNextColumn();

	char from[96] = {};
	char to[96] = {};
	LabelOf(rule.from, from, sizeof(from));
	LabelOf(rule.to, to, sizeof(to));

	ImGui::Text("Replace %s with %s", from, to);

	if (!playable)
		UiText::Warn("Track %03d does not exist, so this rule does nothing.", rule.to);

	ImGui::TableNextColumn();
	ImGui::BeginDisabled(!playable);

	if (ImGui::SmallButton("Preview"))
		BgmControl::Play(rule.to);

	ImGui::EndDisabled();
	ImGui::SameLine();
	return ImGui::SmallButton("Remove");
}

void MusicPanel::DrawRuleEditor()
{
	ImGui::SeparatorText("Add a rule");

	DrawTrackCombo("Replace", m_ruleFrom);
	DrawTrackCombo("with", m_ruleTo);

	const bool picked = BgmCatalog::Find(m_ruleFrom) != nullptr && BgmCatalog::Find(m_ruleTo) != nullptr;
	const bool valid = picked && m_ruleFrom != m_ruleTo;

	ImGui::BeginDisabled(!valid);

	if (ImGui::Button("Add rule"))
		BgmRules::Add(m_ruleFrom, m_ruleTo);

	ImGui::EndDisabled();

	if (picked && !valid)
		UiText::Muted("Pick two different tracks.");
}

void MusicPanel::DrawTrackCombo(const char* label, int& id)
{
	char preview[96] = "Choose a track";

	if (BgmCatalog::Find(id) != nullptr)
		LabelOf(id, preview, sizeof(preview));

	Ui::SetItemWidth(kComboWidth);

	if (ImGui::BeginCombo(label, preview, ImGuiComboFlags_HeightLarge))
	{
		DrawTrackChoices(id);
		ImGui::EndCombo();
	}

	const int steps = ComboNav::WheelSteps();

	if (steps == 0)
		return;

	id = StepTrack(id, steps);
}

void MusicPanel::DrawTrackChoices(int& id)
{
	if (ImGui::IsWindowAppearing())
	{
		m_pick[0] = '\0';
		ImGui::SetKeyboardFocusHere();
	}

	ImGui::SetNextItemWidth(-FLT_MIN);
	ImGui::InputTextWithHint("##pick", "Search", m_pick, IM_ARRAYSIZE(m_pick));
	ImGui::Separator();

	for (int i = 0; i < BgmCatalog::Count(); ++i)
	{
		const BgmCatalog::Track& track = BgmCatalog::At(i);

		if (!Matches(track, m_pick))
			continue;

		char text[96] = {};
		Label(track, text, sizeof(text));

		const bool selected = track.id == id;

		ImGui::PushID(track.id);

		if (ImGui::Selectable(text, selected))
			id = track.id;

		ComboNav::KeepSelectedInView(selected);
		ImGui::PopID();
	}
}
