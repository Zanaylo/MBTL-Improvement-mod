#include "Overlay/Window/ConfigPanel.h"

#include "Core/KeyboardCapture.h"
#include "Core/Settings.h"
#include "Core/info.h"
#include "Core/interfaces.h"
#include "Core/keycodes.h"
#include "Game/HiddenCharacters.h"
#include "Overlay/UiScale.h"
#include "Overlay/UiText.h"

#include <imgui.h>

#include <windows.h>

namespace {

constexpr int kFunctionRow = Hotkeys::Action_Count;
constexpr const char* kOverlay = "Overlay";
constexpr const char* kTraining = "Training";
constexpr float kSliderWidth = 160.0f;

const char* const kCursorModes[] = { "Automatic", "Drawn by the overlay", "Windows cursor" };

bool SaveBoolOnChange(const char* label, bool& value, const char* section, const char* key)
{
	if (!ImGui::Checkbox(label, &value))
		return false;

	Settings::SaveBool(section, key, value);
	return true;
}

}

void ConfigPanel::Draw()
{
	if (!ImGui::BeginTabBar("##config"))
		return;

	if (ImGui::BeginTabItem("General"))
	{
		DrawGeneralTab();
		ImGui::EndTabItem();
	}

	if (ImGui::BeginTabItem("Keybinds"))
	{
		DrawKeybindsTab();
		ImGui::EndTabItem();
	}
	else
	{
		CancelCapture();
	}

	ImGui::EndTabBar();
}

void ConfigPanel::CancelCapture()
{
	m_capture = -1;
	KeyboardCapture::SetKeyCaptureActive(false);
}

void ConfigPanel::DrawGeneralTab()
{
	ImGui::Text("%s %s", MBTL_IM_NAME, MBTL_IM_VERSION);
	ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);

	DrawOverlayOptions();
	DrawStepOptions();
	DrawRosterOptions();

	ImGui::SeparatorText("Logging");

	SaveBoolOnChange("Log every file the mod serves", g_settings.logServedFiles, "ModFiles", "LogServedFiles");
	SaveBoolOnChange("Log files the game asked for next to yours", g_settings.logMissingFiles, "ModFiles",
		"LogMissingFiles");

	UiText::Muted("Saved to %s as soon as something changes.", Settings::IniPath().c_str());
}

void ConfigPanel::DrawOverlayOptions()
{
	ImGui::SeparatorText("Overlay");

	SaveBoolOnChange("Keep the hitboxes up in the game's own pause", g_settings.drawWhilePaused, kOverlay,
		"DrawWhileGamePaused");
	SaveBoolOnChange("Show notifications", g_settings.notifications, kOverlay, "Notifications");

	Ui::SetItemWidth(kSliderWidth);
	ImGui::SliderFloat("Interface scale", &g_settings.uiScale, 0.5f, 3.0f, "%.2fx");

	if (ImGui::IsItemDeactivatedAfterEdit())
		Settings::SaveFloat(kOverlay, "UiScale", g_settings.uiScale);

	Ui::SetItemWidth(kSliderWidth);
	ImGui::SliderFloat("Text size", &g_settings.fontSize, 10.0f, 32.0f, "%.0f px");

	if (ImGui::IsItemDeactivatedAfterEdit())
		Settings::SaveFloat(kOverlay, "FontSize", g_settings.fontSize);

	Ui::SetItemWidth(kSliderWidth);

	if (ImGui::Combo("Mouse cursor", &g_settings.overlayCursor, kCursorModes, IM_ARRAYSIZE(kCursorModes)))
		Settings::SaveInt(kOverlay, "Cursor", g_settings.overlayCursor);

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Automatic draws the cursor itself in exclusive fullscreen, where Windows hides it.");
}

void ConfigPanel::DrawStepOptions()
{
	ImGui::SeparatorText("Holding the next-frame key");

	Ui::SetItemWidth(kSliderWidth);
	ImGui::SliderInt("Wait before repeating", &g_settings.stepRepeatDelayMs, 0, 1000, "%d ms");

	if (ImGui::IsItemDeactivatedAfterEdit())
		Settings::SaveInt(kTraining, "StepRepeatDelayMs", g_settings.stepRepeatDelayMs);

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("How long the key has to be held before it steps on its own. A tap is always one frame.");

	Ui::SetItemWidth(kSliderWidth);
	ImGui::SliderInt("Between steps", &g_settings.stepRepeatIntervalMs, 16, 500, "%d ms");

	if (ImGui::IsItemDeactivatedAfterEdit())
		Settings::SaveInt(kTraining, "StepRepeatIntervalMs", g_settings.stepRepeatIntervalMs);
}

void ConfigPanel::DrawRosterOptions()
{
	ImGui::SeparatorText("Characters");

	SaveBoolOnChange("Unplayable characters", g_settings.unlockHiddenCharacters, "Roster", "UnlockHiddenCharacters");

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Lets you pick the characters the game lists but locks, such as Mario's Sister, on character "
			"select and in training's quick character select. Offline only: it switches itself off in any online mode. "
			"They were never meant to be played alone, so expect rough edges.");

	if (!HiddenCharacters::IsAvailable())
	{
		UiText::Warn("%s", HiddenCharacters::StatusText());
		return;
	}

	if (g_settings.unlockHiddenCharacters)
		UiText::Muted("%s", HiddenCharacters::StatusText());
}

void ConfigPanel::DrawKeybindsTab()
{
	if (m_capture >= 0)
		Capture();

	DrawFunctionRow();

	if (ImGui::BeginTable("##keybinds", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
		ImGuiTableFlags_SizingStretchProp))
	{
		ImGui::TableSetupColumn("What");
		ImGui::TableSetupColumn("Key");
		ImGui::TableSetupColumn("");
		ImGui::TableHeadersRow();

		for (int i = 0; i < Hotkeys::Action_Count; ++i)
			DrawBindRow(static_cast<Hotkeys::Action>(i));

		ImGui::EndTable();
	}

	ImGui::TextDisabled("%s", m_capture >= 0 ? "Press the key you want, or Escape to cancel."
		: "Saved to MBTL_IM.ini as soon as something is bound.");

	DrawConflicts();
}

void ConfigPanel::BeginCapture(int row)
{
	m_capture = m_capture == row ? -1 : row;
	KeyboardCapture::SetKeyCaptureActive(m_capture >= 0);
}

void ConfigPanel::Capture()
{
	KeyboardCapture::SetKeyCaptureActive(true);

	const int pressed = PollPressedKey();

	if (pressed == 0)
		return;

	if (pressed == VK_ESCAPE)
	{
		CancelCapture();
		return;
	}

	if (m_capture == kFunctionRow)
	{
		Hotkeys::SetFunctionKey(pressed);
		CancelCapture();
		return;
	}

	const Hotkeys::Action action = static_cast<Hotkeys::Action>(m_capture);

	KeyBind bind = Hotkeys::Bind(action);
	bind.key = pressed;
	Hotkeys::SetBind(action, bind);

	CancelCapture();
}

void ConfigPanel::DrawFunctionRow()
{
	const bool capturing = m_capture == kFunctionRow;
	const int key = Hotkeys::FunctionKey().key;

	ImGui::TextUnformatted("Function key");
	ImGui::SameLine();
	ImGui::TextDisabled("held with a bind marked Fn");

	ImGui::PushID("function");

	if (ImGui::SmallButton(capturing ? "Cancel" : "Change"))
		BeginCapture(kFunctionRow);

	ImGui::SameLine();
	ImGui::TextUnformatted(capturing ? "press a key" : (key != 0 ? GetNameFromVirtualKey(key) : "none"));

	ImGui::PopID();
}

void ConfigPanel::DrawBindRow(Hotkeys::Action action)
{
	const bool capturing = m_capture == action;
	const KeyBind& bind = Hotkeys::Bind(action);

	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::TextUnformatted(Hotkeys::Label(action));

	ImGui::TableNextColumn();
	ImGui::PushID(action);

	ImGui::TextUnformatted(capturing ? "press a key" : (bind.key != 0 ? GetNameFromVirtualKey(bind.key) : "none"));
	ImGui::SameLine();

	bool function = bind.function;

	if (ImGui::Checkbox("Fn", &function))
	{
		KeyBind changed = bind;
		changed.function = function;
		Hotkeys::SetBind(action, changed);
	}

	ImGui::TableNextColumn();

	if (ImGui::SmallButton(capturing ? "Cancel" : "Change"))
		BeginCapture(action);

	ImGui::SameLine();

	if (ImGui::SmallButton("Clear"))
		Hotkeys::SetBind(action, KeyBind());

	ImGui::PopID();
}

void ConfigPanel::DrawConflicts()
{
	for (int i = 0; i < Hotkeys::Action_Count; ++i)
	{
		const Hotkeys::Action mine = static_cast<Hotkeys::Action>(i);

		for (int k = i + 1; k < Hotkeys::Action_Count; ++k)
		{
			const Hotkeys::Action theirs = static_cast<Hotkeys::Action>(k);

			if (!KeyBinds::SameKey(Hotkeys::Bind(mine), Hotkeys::Bind(theirs)))
				continue;

			UiText::Warn("%s and %s are both %s.", Hotkeys::Label(mine), Hotkeys::Label(theirs),
				Hotkeys::Describe(mine));
		}
	}
}
