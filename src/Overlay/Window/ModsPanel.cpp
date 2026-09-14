#include "Overlay/Window/ModsPanel.h"

#include "Game/ModFiles.h"
#include "Game/ModPacks.h"
#include "Overlay/UiScale.h"
#include "Overlay/UiText.h"
#include "Stages/StageImport.h"

#include <imgui.h>

#include <windows.h>

#include <cstdio>
#include <string>

namespace {

constexpr float kListHeight = 300.0f;
constexpr float kSwitchColumn = 40.0f;
constexpr float kFilesColumn = 70.0f;
constexpr float kOrderColumn = 100.0f;
constexpr const char* kZipFilter = "A mod (*.zip)\0*.zip\0All files\0*.*\0";

const ImVec4 kOwnText(0.45f, 0.90f, 0.50f, 1.0f);

void Open(const std::string& folder)
{
	ShellExecuteA(nullptr, "open", folder.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

}

void ModsPanel::Draw()
{
	TakeInstall();

	if (!ImGui::BeginTabBar("##modstabs"))
		return;

	if (ImGui::BeginTabItem("Mods"))
	{
		DrawTools();
		DrawList();
		DrawFooter();
		ImGui::EndTabItem();
	}

	if (ImGui::BeginTabItem("Help"))
	{
		DrawHelp();
		ImGui::EndTabItem();
	}

	ImGui::EndTabBar();
}

void ModsPanel::TakeInstall()
{
	std::string picked;

	if (!m_install.TakeResult(picked) || picked.empty())
		return;

	if (ModPacks::Install(picked, m_status, sizeof(m_status)))
		ModFiles::Rescan();
}

void ModsPanel::DrawTools()
{
	if (ImGui::Button("Install a zip...") && !m_install.IsRunning())
		m_install.BeginOpen("Choose a mod", kZipFilter);

	ImGui::SameLine();

	if (ImGui::Button("Open the folder"))
		Open(ModPacks::Root());

	ImGui::SameLine();

	if (ImGui::Button("Look again"))
	{
		ModPacks::Scan();
		ModFiles::Rescan();
	}

	if (m_status[0] != '\0')
		UiText::Muted("%s", m_status);
}

void ModsPanel::DrawList()
{
	m_moved = -1;
	m_delta = 0;

	if (!ImGui::BeginTable("##mods", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
		ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY, ImVec2(0.0f, Ui::Scaled(kListHeight))))
	{
		return;
	}

	ImGui::TableSetupColumn("On", ImGuiTableColumnFlags_WidthFixed, Ui::Scaled(kSwitchColumn));
	ImGui::TableSetupColumn("Mod (top wins)");
	ImGui::TableSetupColumn("Files", ImGuiTableColumnFlags_WidthFixed, Ui::Scaled(kFilesColumn));
	ImGui::TableSetupColumn("Move", ImGuiTableColumnFlags_WidthFixed, Ui::Scaled(kOrderColumn));
	ImGui::TableSetupScrollFreeze(0, 1);
	ImGui::TableHeadersRow();

	DrawOwnRow();

	for (int i = 0; i < ModPacks::Count(); ++i)
		DrawRow(i);

	ImGui::EndTable();

	if (m_moved >= 0)
		m_dirty = ModPacks::Move(m_moved, m_delta) || m_dirty;

	if (!m_dirty)
		return;

	m_dirty = false;
	ModFiles::Rescan();
}

void ModsPanel::DrawOwnRow()
{
	ImGui::TableNextRow();

	ImGui::TableNextColumn();
	ImGui::TextDisabled("--");

	ImGui::TableNextColumn();
	ImGui::TextColored(kOwnText, "Your own files");

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("%s", ModFiles::Root());

	ImGui::SameLine();
	UiText::Muted("always on, always wins");

	ImGui::TableNextColumn();
	ImGui::Text("%d", ModFiles::OwnCount());

	ImGui::TableNextColumn();

	if (ImGui::SmallButton("Open"))
		Open(ModFiles::Root());
}

void ModsPanel::DrawRow(int index)
{
	const ModPacks::Pack* const pack = ModPacks::At(index);

	if (pack == nullptr)
		return;

	ImGui::PushID(index);
	ImGui::TableNextRow();

	ImGui::TableNextColumn();

	bool enabled = pack->enabled;

	if (ImGui::Checkbox("##on", &enabled))
	{
		ModPacks::SetEnabled(index, enabled);
		m_dirty = true;
	}

	ImGui::TableNextColumn();
	ImGui::TextUnformatted(pack->name.c_str());

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("%s", pack->path.c_str());

	if (!pack->author.empty() || !pack->version.empty())
	{
		ImGui::SameLine();
		UiText::Muted("%s%s%s", pack->author.c_str(), pack->author.empty() ? "" : " ", pack->version.c_str());
	}

	if (!pack->note.empty())
		UiText::Muted("%s", pack->note.c_str());

	if (pack->beaten > 0)
		UiText::Warn("%d file(s) not used: %s is higher in the list and has them too.", pack->beaten,
			pack->beatenBy.c_str());

	DrawStageRow(index);

	ImGui::TableNextColumn();
	ImGui::Text("%d", pack->files);

	ImGui::TableNextColumn();
	ImGui::BeginDisabled(index == 0);

	if (ImGui::SmallButton("Up"))
	{
		m_moved = index;
		m_delta = -1;
	}

	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::BeginDisabled(index + 1 >= ModPacks::Count());

	if (ImGui::SmallButton("Down"))
	{
		m_moved = index;
		m_delta = 1;
	}

	ImGui::EndDisabled();
	ImGui::PopID();
}

void ModsPanel::DrawStageRow(int index)
{
	const ModPacks::Pack* const pack = ModPacks::At(index);

	if (pack == nullptr || pack->stage <= 0)
		return;

	const int owner = ModPacks::StageOwner(pack->stage, index);
	const ModPacks::Pack* const other = owner < 0 ? nullptr : ModPacks::At(owner);

	if (other != nullptr)
		UiText::Warn("Stage %d, same number as %s. Install as a stage gives it a free number.",
			pack->stage, other->name.c_str());
	else
		UiText::Muted("Has stage %d.", pack->stage);

	ImGui::BeginDisabled(StageImport::IsBusy());

	if (ImGui::SmallButton("Install as a stage"))
	{
		char folder[MAX_PATH] = {};
		sprintf_s(folder, "%s\\bg\\bg%03d", pack->path.c_str(), pack->stage);

		const bool started = StageImport::InstallFolder(folder, pack->name.c_str());
		sprintf_s(m_status, "%s", started ? "Installing. Restart the game when it is done"
			: StageImport::StatusText());
	}

	ImGui::EndDisabled();

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Adds the stage to the game's stage list with the next free number. The game reads that "
			"list at startup, so restart the game after.");
}

void ModsPanel::DrawFooter()
{
	if (ModPacks::Count() == 0)
	{
		UiText::Muted("No mods installed. Install a zip, or drop a mod's folder into %s.", ModPacks::Root().c_str());
		return;
	}

	UiText::Good("%d file(s) from %d mod(s) in use.", ModPacks::FileCount(),
		ModPacks::EnabledCount());
}

void ModsPanel::DrawHelp()
{
	ImGui::TextWrapped("A mod is a folder that uses the game's own file paths. For example, a mod that changes the "
		"textures of stage 1 only holds bg\\bg001. Install a zip or copy the folder in yourself. It shows up in "
		"the list right away, no restart needed.");

	ImGui::SeparatorText("Turning a mod on or off");
	ImGui::TextWrapped("Turn a mod off and the game uses its own file the next time it loads it. Mods never "
		"overwrite game files or dataNNN.bin archives, so removing all mods leaves the game exactly as Steam "
		"installed it.");

	ImGui::SeparatorText("When two mods change the same file");
	ImGui::TextWrapped("The mod higher in the list wins, for that file only. The other mod's row shows a warning "
		"with the name of the winner.");

	ImGui::TextWrapped("Which file the game uses, from highest priority to lowest:");
	ImGui::BulletText("Your own files in MBTL-IM\\Mods (the top row)");
	ImGui::BulletText("The mod list, top to bottom");
	ImGui::BulletText("The game's own files, for everything else");

	ImGui::SeparatorText("Stages");
	ImGui::TextWrapped("A new stage must be added to the game's stage list, which the game reads at startup. A mod "
		"with a stage gets an Install as a stage button. The On switch alone does not add it.");

	ImGui::SeparatorText("Making a mod");
	ImGui::TextWrapped("Put your files at the same paths the game uses. Add a mod.ini to give it a name:");
	ImGui::TextUnformatted("[Mod]\nName = Sunset Bridge\nAuthor = you\nVersion = 1.0\nNote = A brighter sky.");

	ImGui::SeparatorText("Online");
	ImGui::TextWrapped("Art and sound mods are fine online. Only you see them. Mods with data or script files "
		"change how the game plays and cause desyncs.");
}
