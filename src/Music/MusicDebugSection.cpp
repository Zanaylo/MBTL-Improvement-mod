#include "Music/MusicDebugSection.h"

#include "Core/utils.h"
#include "Music/BgmControl.h"
#include "Music/BgmTable.h"
#include "Music/BgmVolume.h"
#include "Music/MusicAnchors.h"
#include "Overlay/UiText.h"

#include <imgui.h>

namespace {

constexpr ImGuiTableFlags kTableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
	ImGuiTableFlags_SizingStretchProp;

void DrawAnchors()
{
	if (!ImGui::BeginTable("##musicanchors", 3, kTableFlags))
		return;

	ImGui::TableSetupColumn("Name");
	ImGui::TableSetupColumn("Where");
	ImGui::TableSetupColumn("Note");
	ImGui::TableHeadersRow();

	for (int i = 0; i < MusicAnchors::Count(); ++i)
	{
		const MusicAnchor& anchor = MusicAnchors::At(i);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(anchor.name);
		ImGui::TableNextColumn();

		if (anchor.address == 0)
			UiText::Warn("not found");
		else
			ImGui::TextUnformatted(DescribeAddress(anchor.address).c_str());

		ImGui::TableNextColumn();
		ImGui::TextUnformatted(anchor.note);
	}

	ImGui::EndTable();
}

}

void MusicDebugSection::Draw()
{
	ImGui::TextWrapped("%s", BgmControl::StatusText());

	ImGui::SeparatorText("Player");
	ImGui::Text("Current id %d, loaded %d, stream %d, started %d, BGM off %d", BgmTable::CurrentId(),
		BgmTable::IsLoaded() ? 1 : 0, BgmTable::HasStream() ? 1 : 0, BgmTable::IsStarted() ? 1 : 0,
		BgmTable::IsMuted() ? 1 : 0);
	ImGui::Text("Track volume %d, base volume %d, SetBgmVolume hook %s", BgmTable::TrackVolume(),
		BgmTable::BaseVolume(), BgmVolume::IsHooked() ? "on" : "off");

	ImGui::SeparatorText("Hook");
	ImGui::Text("PlayBgm calls %ld, last asked %d, last played %d, held %d", BgmControl::CallCount(),
		BgmControl::LastAsked(), BgmControl::LastPlayed(), BgmControl::HeldId());
	ImGui::Text("Game thread %lu, Present thread %lu, UI requests run on %s", BgmControl::GameThread(),
		BgmControl::PresentThread(), BgmControl::RunsOnPresent() ? "Present" : "the next PlayBgm");

	char pending[96] = {};
	BgmControl::PendingText(pending, sizeof(pending));
	ImGui::Text("Queue: %s", BgmControl::HasPending() ? pending : "empty");

	char reason[192] = {};
	BgmControl::ReasonText(reason, sizeof(reason));
	ImGui::TextWrapped("Reason: %s", reason);

	ImGui::SeparatorText("Resolved addresses");
	DrawAnchors();
}
