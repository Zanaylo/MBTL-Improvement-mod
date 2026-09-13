#include "Overlay/Window/MainWindow.h"

#include "Core/Hotkeys.h"
#include "Core/Settings.h"
#include "Core/interfaces.h"
#include "Game/ModFiles.h"
#include "Game/ModPacks.h"
#include "Music/BgmControl.h"
#include "Network/PaletteShare.h"
#include "Overlay/ComboNav.h"
#include "Overlay/FrameMeterHud.h"
#include "Overlay/UiScale.h"
#include "Overlay/UiText.h"
#include "Overlay/Window/HitboxOverlay.h"
#include "Overlay/WindowContainer/WindowContainer.h"
#include "Palette/Characters.h"
#include "Palette/PaletteChoice.h"
#include "Palette/PaletteControl.h"
#include "Palette/PaletteLibrary.h"
#include "Palette/PaletteOwner.h"
#include "Stages/StageImport.h"
#include "Stages/StageLibrary.h"
#include "Training/BattleHud.h"
#include "Training/CharacterDraw.h"
#include "Training/FrameStepper.h"
#include "Training/GameState.h"

#include <algorithm>
#include <cfloat>
#include <cstring>

namespace {

constexpr float kBaseWidthInFonts = 30.0f;
constexpr float kHeightShare = 0.85f;
constexpr float kOptionWidth = 160.0f;
constexpr const char* kFrameMeter = "FrameMeter";
constexpr const char* kPalette = "Palette";
constexpr const char* kDefaultPalette = "Default";

void SavedCheckbox(const char* label, bool* value, const char* key, const char* tooltip)
{
	if (ImGui::Checkbox(label, value))
		Settings::SaveBool(kFrameMeter, key, *value);

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("%s", tooltip);
}

}

MainWindow::MainWindow(const std::string& title, bool closable, const WindowContainer& windows)
	: IWindow(title, closable)
	, m_windows(windows)
{
}

void MainWindow::BeforeDraw()
{
	const float base = ImGui::GetFontSize() * kBaseWidthInFonts;
	const float ceiling = ImGui::GetIO().DisplaySize.y * kHeightShare;

	ImGui::SetNextWindowSizeConstraints(ImVec2(base, 0.0f), ImVec2(FLT_MAX, ceiling > 0.0f ? ceiling : FLT_MAX));
	ImGui::SetNextWindowSize(ImVec2(base, 0.0f), ImGuiCond_FirstUseEver);
}

void MainWindow::Draw()
{
	DrawTrainingSection();
	DrawStagesSection();
	DrawMusicSection();
	DrawPaletteSection();
	DrawModsSection();
	DrawPerformanceSection();
	DrawConfigSection();
}

bool MainWindow::DrawWindowButton(WindowType type, const char* open, const char* close) const
{
	IWindow* const window = m_windows.GetWindow(type);

	if (window == nullptr)
		return false;

	if (!ImGui::Button(window->IsOpen() ? close : open))
		return false;

	window->Toggle();
	return true;
}

void MainWindow::DrawTrainingSection()
{
	if (!ImGui::CollapsingHeader("Training", ImGuiTreeNodeFlags_DefaultOpen))
		return;

	if (!GameState::AllowsTrainingTools())
	{
		UiText::Muted("%s", GameState::StatusText());
		DrawHitboxControls();
		DrawFrameMeterControls();
		DrawHudControls();
		DrawCharacterControls();
		return;
	}

	DrawHitboxControls();
	DrawFrameMeterControls();
	DrawHudControls();
	DrawCharacterControls();
	ImGui::Spacing();
	DrawFrameStepControls();
	DrawHitboxTypes();
	DrawFrameMeterOptions();
}

void MainWindow::DrawFrameMeterControls()
{
	bool visible = FrameMeterHud::IsVisible();

	if (ImGui::Checkbox("Frame meter", &visible))
		FrameMeterHud::SetVisible(visible);

	ImGui::SameLine();
	ImGui::TextDisabled("(%s)", Hotkeys::Describe(Hotkeys::Action_ToggleFrameMeter));

	if (visible && FrameMeterHud::FontFailed())
		UiText::Warn("The game's font did not load, so the meter draws without its numbers.");
}

void MainWindow::DrawFrameMeterOptions()
{
	if (!ImGui::TreeNode("Frame meter options"))
		return;

	Ui::SetItemWidth(kOptionWidth);
	ImGui::SliderFloat("Size", &g_settings.frameMeterScale, 0.5f, 4.0f, "%.2fx");

	if (ImGui::IsItemDeactivatedAfterEdit())
		Settings::SaveFloat(kFrameMeter, "Scale", g_settings.frameMeterScale);

	Ui::SetItemWidth(kOptionWidth);
	ImGui::SliderInt("Opacity", &g_settings.frameMeterOpacity, 10, 100, "%d%%");

	if (ImGui::IsItemDeactivatedAfterEdit())
		Settings::SaveInt(kFrameMeter, "Opacity", g_settings.frameMeterOpacity);

	SavedCheckbox("Count band", &g_settings.frameMeterCounts, "BandCounts",
		"The length of each run of cells, written inside the run.");
	SavedCheckbox("Hitstun, gap and flash", &g_settings.frameMeterTotals, "LineTotals",
		"A line above and below the bars totalling blockstun, hitstun, gaps and super flash.");
	SavedCheckbox("Status row", &g_settings.frameMeterAttributes, "AttributeRow",
		"A thin row under each bar showing which kinds of invincibility are in force.");
	SavedCheckbox("Place automatically", &g_settings.frameMeterAuto, "PlaceAutomatically",
		"Centred near the bottom of the screen, whatever the window size.");

	if (!g_settings.frameMeterAuto)
	{
		SavedCheckbox("Move the meter with the mouse", &g_settings.frameMeterDrag, "MouseDrag",
			"While this window is open, drag the meter where you want it.");

		Ui::SetItemWidth(kOptionWidth);
		ImGui::DragInt("X", &g_settings.frameMeterX, 2.0f, 0, 4096);

		if (ImGui::IsItemDeactivatedAfterEdit())
			Settings::SaveInt(kFrameMeter, "PositionX", g_settings.frameMeterX);

		Ui::SetItemWidth(kOptionWidth);
		ImGui::DragInt("Y", &g_settings.frameMeterY, 2.0f, 0, 4096);

		if (ImGui::IsItemDeactivatedAfterEdit())
			Settings::SaveInt(kFrameMeter, "PositionY", g_settings.frameMeterY);
	}

	SavedCheckbox("Write the meter's readings to the log", &g_settings.frameMeterTrace, "Trace",
		"One line per player per recorded frame with every field the meter reads. Send the log when a colour looks "
		"wrong; leave it off otherwise.");

	DrawWindowButton(WindowType_FrameMeterLegend, "Frame meter information", "Close frame meter information");
	ImGui::TreePop();
}

void MainWindow::DrawHudControls()
{
	if (!BattleHud::IsAvailable())
	{
		UiText::Warn("Hide the HUD: %s", BattleHud::StatusText());
		return;
	}

	bool hidden = BattleHud::IsHidden();

	if (ImGui::Checkbox("Hide the HUD", &hidden))
		BattleHud::SetHidden(hidden);

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Takes the gauges, the timer and the training info off the screen the way the game itself does "
			"for a cinematic. Offline only.");

	ImGui::SameLine();
	ImGui::TextDisabled("(%s)", Hotkeys::Describe(Hotkeys::Action_HideHud));
}

void MainWindow::DrawCharacterControls()
{
	if (!CharacterDraw::IsAvailable())
	{
		UiText::Warn("Hide the characters: %s", CharacterDraw::StatusText());
		return;
	}

	bool hidden = CharacterDraw::IsHidden();

	if (ImGui::Checkbox("Hide the characters", &hidden))
		CharacterDraw::SetHidden(hidden);

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Stops the characters being drawn while the match runs exactly as before. The stage and the HUD "
			"stay. Offline only.");

	ImGui::SameLine();
	ImGui::TextDisabled("(%s)", Hotkeys::Describe(Hotkeys::Action_HideCharacters));

	if (!hidden)
		return;

	ImGui::Indent();

	bool effects = CharacterDraw::EffectsHidden();

	if (ImGui::Checkbox("Their effects too", &effects))
		CharacterDraw::SetEffectsHidden(effects);

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Projectiles, hit sparks and super effects.");

	ImGui::Unindent();
}

void MainWindow::DrawHitboxControls()
{
	IWindow* const overlay = m_windows.GetWindow(WindowType_HitboxOverlay);

	if (overlay == nullptr)
		return;

	bool open = overlay->IsOpen();

	if (ImGui::Checkbox("Hitbox viewer", &open))
		open ? overlay->Open() : overlay->Close();

	ImGui::SameLine();
	ImGui::TextDisabled("(%s)", Hotkeys::Describe(Hotkeys::Action_ToggleHitbox));

	if (!HitboxOverlay::IsAvailable())
		UiText::Warn("%s", HitboxOverlay::StatusText());
}

void MainWindow::DrawHitboxTypes()
{
	HitboxOverlay* const overlay = m_windows.GetWindow<HitboxOverlay>(WindowType_HitboxOverlay);

	if (overlay == nullptr || !ImGui::TreeNode("Hitbox types"))
		return;

	if (ImGui::Checkbox("Show origin", &g_settings.hitboxShowOrigin))
		Settings::SaveBool("Hitbox", "ShowOrigin", g_settings.hitboxShowOrigin);

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("A cross at each object's own position - the point its boxes are measured from.");

	if (ImGui::BeginTable("##boxtypes", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
		ImGuiTableFlags_SizingStretchProp))
	{
		const float swatch = ImGui::GetFontSize();

		ImGui::TableSetupColumn("Type");
		ImGui::TableSetupColumn("On", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFrameHeight());
		ImGui::TableSetupColumn("Fill");
		ImGui::TableSetupColumn("Outline");
		ImGui::TableHeadersRow();

		for (int i = 0; i < HitboxOverlay::CategoryCount(); ++i)
		{
			HitboxOverlay::CategorySettings& settings = overlay->Category(i);

			ImGui::PushID(i);
			ImGui::TableNextRow();

			ImGui::TableNextColumn();
			ImGui::ColorButton("##swatch", ImGui::ColorConvertU32ToFloat4(HitboxOverlay::CategoryColor(i)),
				ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker, ImVec2(swatch, swatch));
			ImGui::SameLine();
			ImGui::TextUnformatted(HitboxOverlay::CategoryName(i));

			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("%s", HitboxOverlay::CategorySummary(i));

			ImGui::TableNextColumn();
			ImGui::Checkbox("##on", &settings.enabled);

			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::SliderFloat("##fill", &settings.fillAlpha, 0.0f, 1.0f, "%.2f");

			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::SliderFloat("##outline", &settings.outlineAlpha, 0.0f, 1.0f, "%.2f");

			ImGui::PopID();
		}

		ImGui::EndTable();
	}

	ImGui::TreePop();
}

void MainWindow::DrawFrameStepControls()
{
	if (!FrameStepper::IsImplemented())
	{
		UiText::Warn("Pause is not available: %s", FrameStepper::StatusText());
		return;
	}

	bool paused = FrameStepper::IsPaused();

	if (ImGui::Checkbox("Pause", &paused))
		FrameStepper::SetPaused(paused);

	ImGui::SameLine();
	ImGui::TextDisabled("(%s)", Hotkeys::Describe(Hotkeys::Action_FreezeFrame));

	ImGui::BeginDisabled(!paused);

	if (ImGui::Button("Next frame"))
		FrameStepper::RequestStep(1);

	ImGui::SameLine();
	ImGui::TextDisabled("(%s)", Hotkeys::Describe(Hotkeys::Action_StepForward));

	ImGui::EndDisabled();
}

void MainWindow::DrawStagesSection()
{
	if (!ImGui::CollapsingHeader("Stages"))
		return;

	DrawWindowButton(WindowType_Stages, "Open stages", "Close stages");

	ImGui::TextWrapped("Stages taken out of another French-Bread game you own and installed as stages of their "
		"own, the stages the game hides, and the stage table.");

	UiText::Good("%d stage(s) installed.", StageLibrary::Count());
	UiText::Muted("%s", StageImport::StatusText());
}

void MainWindow::DrawMusicSection()
{
	if (!ImGui::CollapsingHeader("Music"))
		return;

	DrawWindowButton(WindowType_Music, "Open music", "Close music");

	ImGui::TextWrapped("The whole track list, your own music, and the rules that decide what plays where.");

	if (!BgmControl::IsHooked())
	{
		UiText::Warn("Music control is not active: %s", BgmControl::StatusText());
		return;
	}

	UiText::Muted("%s", BgmControl::StatusText());
}

void MainWindow::DrawModsSection()
{
	if (!ImGui::CollapsingHeader("Mods"))
		return;

	DrawWindowButton(WindowType_Mods, "Open mods", "Close mods");

	ImGui::TextWrapped("Folders in MBTL-IM\\Packs that stand in for the game's own files. Switch one on or off "
		"without restarting.");

	UiText::Good("%d of %d mod(s) on. %s", ModPacks::EnabledCount(), ModPacks::Count(), ModFiles::StatusText());
}

void MainWindow::DrawPaletteSection()
{
	if (!ImGui::CollapsingHeader("Palettes"))
		return;

	DrawWindowButton(WindowType_Palette, "Open palette editor", "Close palette editor");

	ImGui::SeparatorText("Character Palette");

	for (int seat = 0; seat < PaletteOwner::kSeats; ++seat)
		DrawPaletteChooser(seat);

	ImGui::TextDisabled("Previous palette %s,", Hotkeys::Describe(Hotkeys::Action_PreviousPalette));
	ImGui::SameLine();
	ImGui::TextDisabled("next palette %s", Hotkeys::Describe(Hotkeys::Action_NextPalette));

	DrawPaletteOptions();
}

void MainWindow::DrawPaletteChooser(int player)
{
	const int chara = PaletteOwner::CharaNumber(player);

	if (chara < 0 && PaletteOwner::MemberOf(player) > 0)
		return;

	if (chara < 0)
	{
		ImGui::TextDisabled("P%d: nobody there yet", PaletteOwner::SideOf(player) + 1);
		return;
	}

	ImGui::PushID(player);

	ImGui::Text("P%d", PaletteOwner::SideOf(player) + 1);
	ImGui::SameLine();
	ImGui::TextDisabled("%s", Characters::Name(chara));

	const char* const worn = PaletteChoice::WornFile(player);
	const bool bare = worn[0] == '\0';

	ImGui::BeginDisabled(!PaletteControl::CanEdit(player));
	Ui::SetItemWidth(kOptionWidth);

	if (ImGui::BeginCombo("##worn", bare ? kDefaultPalette : worn))
	{
		if (ImGui::Selectable(kDefaultPalette, bare))
			PaletteChoice::Bare(player);

		ComboNav::KeepSelectedInView(bare);

		for (int i = 0; i < PaletteLibrary::GetCount(chara); ++i)
		{
			const char* const file = PaletteLibrary::GetName(chara, i);
			const bool selected = !bare && strcmp(file, worn) == 0;

			ImGui::PushID(i);

			if (ImGui::Selectable(file, selected))
				PaletteChoice::Wear(player, file);

			ComboNav::KeepSelectedInView(selected);
			ImGui::PopID();
		}

		ImGui::EndCombo();
	}

	const int steps = ComboNav::WheelSteps();

	if (steps != 0)
		PaletteChoice::Step(player, steps);

	ImGui::SameLine();

	if (ImGui::Button("Rescan"))
		PaletteLibrary::Rescan(chara);

	ImGui::EndDisabled();
	ImGui::PopID();
}

void MainWindow::DrawPaletteOptions()
{
	if (ImGui::Checkbox("Group by part", &g_settings.paletteGroupByPart))
		Settings::SaveBool(kPalette, "GroupByPart", g_settings.paletteGroupByPart);

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Groups the entries the way the game's own colour screen does - hair, skin, shoes - out of its "
			"colour-edit table rather than by guessing.");

	if (ImGui::Checkbox("Flash the entry on the character", &g_settings.paletteFlashEntry))
		Settings::SaveBool(kPalette, "FlashEntry", g_settings.paletteFlashEntry);

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Picking an entry darkens everything else and blinks that entry on the character, so what it "
			"owns is unmistakable before you change it.");

	if (ImGui::Checkbox("Filter junk colours", &g_settings.paletteFilterJunk))
		Settings::SaveBool(kPalette, "FilterJunk", g_settings.paletteFilterJunk);

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Hides the entries no pixel of this character ever uses, read off its own sprite sheet, and the "
			"green the unused slots are filled with. Anything the game's own colour screen offers stays.");

	if (ImGui::Checkbox("See the other player's colours", &g_settings.showOnlinePalettes))
		Settings::SaveBool(kPalette, "ShowOnlinePalettes", g_settings.showOnlinePalettes);

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("On, you see the palette they picked. Off, their side is left the way the game gives it. Yours is "
			"sent either way.");

	if (ImGui::Checkbox("Send my palette to the other player", &g_settings.sharePalettes))
		Settings::SaveBool("Netplay", "SharePalettes", g_settings.sharePalettes);

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Online, the palette you wear is sent to your opponent on the mod's own Steam channel, so they see "
			"it too if they run the mod.");

	if (PaletteControl::IsSpectating())
		ImGui::TextDisabled("watching - the colours are the players' own");
	else if (PaletteControl::LocalPlayer() >= 0)
		ImGui::TextDisabled(PaletteControl::LocalPlayer() == 0 ? "you are playing P1" : "you are playing P2");
	else
		ImGui::TextDisabled("both characters are yours to dress");

	if (PaletteControl::IsOnline())
		UiText::Muted("%s", PaletteShare::GetStatusText());
}

void MainWindow::DrawPerformanceSection()
{
	if (!ImGui::CollapsingHeader("Performance"))
		return;

	DrawWindowButton(WindowType_Performance, "Open performance", "Close performance");

	ImGui::TextWrapped("Frame pacing, the display parameters and where the time in each frame goes.");
}

void MainWindow::DrawConfigSection()
{
	if (!ImGui::CollapsingHeader("Config"))
	{
		m_config.CancelCapture();
		return;
	}

	m_config.Draw();
}
