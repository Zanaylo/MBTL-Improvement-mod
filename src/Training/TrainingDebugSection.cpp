#include "Training/TrainingDebugSection.h"

#include "Core/Settings.h"
#include "Core/interfaces.h"
#include "Overlay/UiScale.h"
#include "Overlay/UiText.h"
#include "Training/Camera.h"
#include "Training/FrameStepper.h"
#include "Training/GameState.h"

#include <imgui.h>

namespace {

constexpr int kShownObjects = 32;
constexpr ImGuiTableFlags kTableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
	ImGuiTableFlags_SizingStretchProp;

const char* YesNo(bool value)
{
	return value ? "yes" : "no";
}

}

void TrainingDebugSection::Draw()
{
	DrawState();
	DrawCamera();
	DrawObjects();
}

void TrainingDebugSection::DrawState()
{
	ImGui::SeparatorText("State");
	ImGui::Text("Battle running %s, online %s (known %s), game paused %s", YesNo(GameState::IsBattleRunning()),
		YesNo(GameState::IsOnline()), YesNo(GameState::IsOnlineKnown()), YesNo(GameState::IsGamePaused()));
	ImGui::Text("Mode %d, sub-mode %d, training %s, frame counter %u", GameState::Mode(), GameState::SubMode(),
		YesNo(GameState::IsTraining()), GameState::FrameCounter());
	ImGui::Text("Mod pause %s, frozen %s", YesNo(FrameStepper::IsPaused()), YesNo(FrameStepper::IsFrozen()));

	if (ImGui::Checkbox("Replay the last frame while paused", &g_settings.replayFrozenFrame))
		Settings::SaveBool("Training", "ReplayFrozenFrame", g_settings.replayFrozenFrame);

	UiText::Help("When off, the paused battle keeps drawing with its simulation stopped. Turn it on only if the "
		"picture looks wrong while paused. It then shows the last captured frame.");
}

void TrainingDebugSection::DrawCamera()
{
	ImGui::SeparatorText("Camera");

	for (int element = 0; element < Camera::ElementCount(); ++element)
	{
		Camera::View view = {};
		const bool read = Camera::ReadElement(element, view);

		ImGui::PushID(element);

		if (ImGui::RadioButton(Camera::ElementName(element), g_settings.hitboxCameraElement == element))
		{
			g_settings.hitboxCameraElement = element;
			Settings::SaveInt("Hitbox", "CameraElement", element);
		}

		ImGui::SameLine();

		if (read)
			ImGui::Text("x %.0f  y %.0f  zoom %.3f", view.x, view.y, view.zoom);
		else
			ImGui::TextDisabled("unreadable");

		ImGui::PopID();
	}

	UiText::Muted("The hitboxes follow the selected element. Pick the one whose boxes stay on the characters when "
		"the camera moves and zooms.");
}

void TrainingDebugSection::DrawObjects()
{
	ImGui::SeparatorText("Objects");

	const int count = HitboxData::Objects(m_objects, HitboxData::kMaxObjects);

	if (!ImGui::BeginTable("##objects", 7, kTableFlags))
		return;

	ImGui::TableSetupColumn("Slot");
	ImGui::TableSetupColumn("Address");
	ImGui::TableSetupColumn("X");
	ImGui::TableSetupColumn("Y");
	ImGui::TableSetupColumn("Facing");
	ImGui::TableSetupColumn("Flags");
	ImGui::TableSetupColumn("Boxes");
	ImGui::TableHeadersRow();

	for (int i = 0; i < count && i < kShownObjects; ++i)
	{
		const HitboxData::Object& object = m_objects[i];
		const int boxes = HitboxData::Boxes(object, m_boxes, HitboxData::kMaxBoxes);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		if (object.effect)
			ImGui::TextUnformatted("effect");
		else
			ImGui::Text("%d", object.slot);

		ImGui::TableNextColumn();
		ImGui::Text("%08X", static_cast<unsigned>(object.address));
		ImGui::TableNextColumn();
		ImGui::Text("%d", object.x);
		ImGui::TableNextColumn();
		ImGui::Text("%d", object.y);
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(object.facingLeft ? "left" : "right");
		ImGui::TableNextColumn();
		ImGui::Text("%08X", object.flags);
		ImGui::TableNextColumn();
		ImGui::Text("%d", boxes);
	}

	ImGui::EndTable();
}
