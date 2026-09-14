#include "Overlay/Window/PaletteWindow.h"

#include "Core/Settings.h"
#include "Core/interfaces.h"
#include "Core/utils.h"
#include "Network/PaletteShare.h"
#include "Overlay/ComboNav.h"
#include "Overlay/UiScale.h"
#include "Palette/BasePals.h"
#include "Palette/Characters.h"
#include "Palette/ColorPartTable.h"
#include "Palette/EffectPaint.h"
#include "Palette/EffectTable.h"
#include "Palette/PaletteChoice.h"
#include "Palette/PaletteControl.h"
#include "Palette/PaletteLibrary.h"
#include "Palette/PaletteOwner.h"
#include "Palette/PalettePaint.h"
#include "Palette/PartColourTable.h"
#include "Palette/PngPalette.h"
#include "Palette/StockPalettes.h"
#include "Palette/UsedEntryTable.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <cfloat>
#include <cstdio>
#include <cstring>

namespace {

constexpr const char* kDefaultPalette = "Default";
constexpr const char* kSection = "Palette";

constexpr float kSwatch = 22.0f;
constexpr int kPerRow = 14;
constexpr int kFlashFrames = 90;
constexpr int kFlashBlink = 8;

constexpr ImVec4 kSelectedBorder(1.0f, 1.0f, 1.0f, 1.0f);
constexpr ImVec4 kChangedBorder(1.0f, 0.8f, 0.2f, 1.0f);

constexpr const char* kPartLabels[LivePalette::kParts] = {
	"Base", "Part 1", "Part 2", "Part 3", "Part 4", "Part 5",
};

int Luminance(const uint8_t* rgb)
{
	return (rgb[0] * 299 + rgb[1] * 587 + rgb[2] * 114) / 1000;
}

bool IsUsableName(const char* name)
{
	if (name == nullptr || name[0] == '\0' || name[0] == ' ' || name[0] == '.')
		return false;

	for (const char* at = name; *at != '\0'; ++at)
	{
		if (strchr("\\/:*?\"<>|", *at) != nullptr)
			return false;
	}

	return true;
}

void StripExtension(char* name)
{
	const size_t length = strlen(name);
	const size_t suffix = strlen(PaletteFile::kExtension);

	if (length > suffix && _stricmp(name + length - suffix, PaletteFile::kExtension) == 0)
		name[length - suffix] = '\0';
}

bool PickColour(const char* id, const uint8_t* rgb, uint8_t* out)
{
	float picker[3] = { rgb[0] / 255.0f, rgb[1] / 255.0f, rgb[2] / 255.0f };

	if (!ImGui::ColorEdit3(id, picker, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
		return false;

	for (int c = 0; c < 3; ++c)
		out[c] = static_cast<uint8_t>(picker[c] * 255.0f + 0.5f);

	return true;
}

ImVec4 ToColour(const uint8_t* rgb)
{
	return ImVec4(rgb[0] / 255.0f, rgb[1] / 255.0f, rgb[2] / 255.0f, 1.0f);
}

bool Swatch(const char* id, const uint8_t* rgb, bool selected, bool changed)
{
	const bool marked = selected || changed;

	if (marked)
	{
		ImGui::PushStyleColor(ImGuiCol_Border, selected ? kSelectedBorder : kChangedBorder);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
	}

	const bool pressed = ImGui::ColorButton(id, ToColour(rgb), ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop,
		Ui::Scaled(kSwatch, kSwatch));

	if (marked)
	{
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
	}

	return pressed;
}

void NextSwatch(int index, int count)
{
	if (((index + 1) % kPerRow) != 0 && index + 1 < count)
		ImGui::SameLine();
}

void SideLabel(int side, char* out, size_t size)
{
	const int main = PaletteOwner::CharaNumber(PaletteOwner::SeatOf(side, 0));
	const int partner = PaletteOwner::CharaNumber(PaletteOwner::SeatOf(side, 1));

	if (main < 0)
	{
		sprintf_s(out, size, "P%d (empty)##%d", side + 1, side);
		return;
	}

	char name[32] = {};
	strncpy_s(name, Characters::Name(main), _TRUNCATE);

	if (partner < 0)
		sprintf_s(out, size, "P%d %s##%d", side + 1, name, side);
	else
		sprintf_s(out, size, "P%d %s & %s##%d", side + 1, name, Characters::Name(partner), side);
}

const char* SubLabel(int sub)
{
	static char label[24] = {};

	if (sub == 0)
		return "Main";

	sprintf_s(label, "Extra %d", sub);
	return label;
}

}

PaletteWindow::PaletteWindow(const std::string& title, bool closable, ImGuiWindowFlags windowFlags)
	: IWindow(title, closable, windowFlags)
{
	for (std::unique_ptr<Side>& side : m_sides)
		side = std::make_unique<Side>();
}

void PaletteWindow::BeforeDraw()
{
	ImGui::SetNextWindowSizeConstraints(Ui::Scaled(440.0f, 460.0f), ImVec2(FLT_MAX, FLT_MAX));
	ImGui::SetNextWindowSize(Ui::Scaled(480.0f, 760.0f), ImGuiCond_FirstUseEver);
}

void PaletteWindow::Draw()
{
	if (ImGui::BeginTabBar("##players"))
	{
		for (int side = 0; side < kPlayers; ++side)
		{
			char label[96] = {};
			SideLabel(side, label, sizeof(label));

			if (!ImGui::BeginTabItem(label))
				continue;

			ImGui::PushID(side);
			DrawSide(side);
			ImGui::PopID();

			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	RunFlash();
}

void PaletteWindow::DrawSide(int side)
{
	if (!PaletteOwner::HasPartner(side))
	{
		DrawPlayer(side);
		return;
	}

	if (!ImGui::BeginTabBar("##members"))
		return;

	for (int member = 0; member < PaletteOwner::kMembers; ++member)
	{
		const int seat = PaletteOwner::SeatOf(side, member);

		char label[64] = {};
		sprintf_s(label, "%s##member%d", Characters::Name(PaletteOwner::CharaNumber(seat)), member);

		if (!ImGui::BeginTabItem(label))
			continue;

		ImGui::PushID(seat);
		DrawPlayer(seat);
		ImGui::PopID();

		ImGui::EndTabItem();
	}

	ImGui::EndTabBar();
}

void PaletteWindow::DrawPlayer(int player)
{
	const int chara = PaletteOwner::CharaNumber(player);

	if (chara < 0)
	{
		ImGui::TextDisabled("No character here yet. The editor works during a match.");
		return;
	}

	Side& side = SideOf(player);

	if (chara != side.chara || PaletteChoice::GetGeneration(player) != side.generation)
		Adopt(player, chara);

	if (!StockPalettes::Load(chara))
	{
		ImGui::TextDisabled("Could not read this character's colour files.");
		return;
	}

	PullBaseline(player, false);
	Refresh(player);

	const bool mine = PaletteControl::CanEdit(player);

	if (!mine)
		DrawRemote(player);

	ImGui::BeginDisabled(!mine);

	DrawSubPalettes(player);

	if (ImGui::BeginTabBar("##what"))
	{
		if (ImGui::BeginTabItem("Character"))
		{
			DrawParts(player);
			DrawSwatches(player);
			DrawPicker(player);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Effects"))
		{
			DrawEffects(player);
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::Separator();
	DrawFiles(player);

	ImGui::EndDisabled();
}

void PaletteWindow::DrawRemote(int player)
{
	Side& side = SideOf(player);
	const uint8_t* const theirs = PalettePaint::GetRemote(player, side.sub);

	if (theirs != nullptr)
		memcpy(side.composed[side.sub], theirs, LivePalette::kBytes);

	ImGui::TextWrapped("%s", PaletteControl::WhyNot(player));

	const char* const name = PaletteShare::GetRemoteName(player);

	if (!PalettePaint::HasRemote(player))
		ImGui::TextDisabled("%s", PaletteControl::CanWear(player) ? "Their colours have not arrived yet."
			: "Their colours are turned off in the options.");
	else if (name[0] != '\0')
		ImGui::TextDisabled("Wearing their palette '%s'.", name);
	else
		ImGui::TextDisabled("Wearing the colour they picked in the game.");

	ImGui::Separator();
}

void PaletteWindow::DrawSubPalettes(int player)
{
	Side& side = SideOf(player);
	int subs = 0;

	for (int sub = 0; sub < kSubs; ++sub)
		subs += StockPalettes::HasSub(side.chara, sub) ? 1 : 0;

	if (subs <= 1)
		return;

	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted("Palette");
	ImGui::SameLine();

	Ui::SetItemWidth(140.0f);

	if (ImGui::BeginCombo("##sub", SubLabel(side.sub)))
	{
		for (int sub = 0; sub < kSubs; ++sub)
		{
			if (!StockPalettes::HasSub(side.chara, sub))
				continue;

			const bool selected = sub == side.sub;

			ImGui::PushID(sub);

			if (ImGui::Selectable(SubLabel(sub), selected) && !selected)
			{
				side.sub = sub;
				side.selected = 1;
			}

			ComboNav::KeepSelectedInView(selected);
			ImGui::PopID();
		}

		ImGui::EndCombo();
	}

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("This character uses more than one palette. Pick the one to edit.");
}

void PaletteWindow::DrawSwatches(int player)
{
	const Side& side = SideOf(player);

	ImGui::BeginChild("swatches", Ui::Scaled(0.0f, 200.0f), ImGuiChildFlags_Borders);

	if (g_settings.paletteGroupByPart && ColorPartTable::Has(side.chara, side.sub))
		DrawGroupedSwatches(player);
	else
		DrawFlatSwatches(player);

	ImGui::EndChild();
}

void PaletteWindow::DrawGroupedSwatches(int player)
{
	const Side& side = SideOf(player);
	bool covered[LivePalette::kColours] = {};
	unsigned char entries[LivePalette::kColours] = {};

	for (int part = 1; part < LivePalette::kParts; ++part)
	{
		const int count = ColorPartTable::Entries(side.chara, side.sub, part, entries);

		if (count == 0)
			continue;

		ImGui::PushID(part);

		if (ImGui::CollapsingHeader(ColorPartTable::Name(side.chara, side.sub, part), ImGuiTreeNodeFlags_DefaultOpen))
			DrawGrid(player, entries, count);

		for (int i = 0; i < count; ++i)
			covered[entries[i]] = true;

		ImGui::PopID();
	}

	int restCount = 0;

	for (int i = 1; i < LivePalette::kColours; ++i)
	{
		if (!covered[i] && !IsJunk(player, i))
			entries[restCount++] = static_cast<unsigned char>(i);
	}

	if (restCount == 0)
		return;

	ImGui::PushID(LivePalette::kParts);

	if (ImGui::CollapsingHeader("Other colours"))
		DrawGrid(player, entries, restCount);

	ImGui::PopID();
}

bool PaletteWindow::IsJunk(int player, int entry) const
{
	if (!g_settings.paletteFilterJunk)
		return false;

	const Side& side = SideOf(player);
	const uint8_t* const colour = side.colours[side.sub].baseline + entry * 4;

	if (colour[0] == 0 && colour[1] == 255 && colour[2] == 0)
		return true;

	if (UsedEntryTable::IsUsed(side.chara, side.sub, entry))
		return false;

	return side.sub != 0 || !PartColourTable::IsPartEntry(side.chara, entry);
}

void PaletteWindow::DrawFlatSwatches(int player)
{
	const Side& side = SideOf(player);
	bool offered[LivePalette::kColours] = {};
	unsigned char entries[LivePalette::kColours] = {};

	for (int part = 1; part < LivePalette::kParts; ++part)
	{
		const int count = ColorPartTable::Entries(side.chara, side.sub, part, entries);

		for (int i = 0; i < count; ++i)
			offered[entries[i]] = true;
	}

	int count = 0;

	for (int i = 1; i < LivePalette::kColours; ++i)
	{
		if (offered[i] || !IsJunk(player, i))
			entries[count++] = static_cast<unsigned char>(i);
	}

	if (count == 0)
	{
		ImGui::TextDisabled("All colours here look unused. Untick Filter junk colours to show them.");
		return;
	}

	DrawGrid(player, entries, count);
}

void PaletteWindow::DrawParts(int player)
{
	if (!ImGui::CollapsingHeader("Whole parts"))
		return;

	ImGui::TextWrapped("Changes every shade of a part at once. The shading stays, only the colour changes.");

	for (int part = 0; part < LivePalette::kParts; ++part)
	{
		ImGui::PushID(part);

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(kPartLabels[part]);

		const Side& side = SideOf(player);

		if (ImGui::IsItemHovered() && part > 0)
			ImGui::SetTooltip("%s", ColorPartTable::Name(side.chara, side.sub, part));

		ImGui::SameLine(ImGui::GetFontSize() * 4.5f);

		DrawPartStock(player, part);
		DrawPartPick(player, part);

		ImGui::PopID();
	}
}

void PaletteWindow::DrawPartStock(int player, int part)
{
	Side& side = SideOf(player);
	LivePalette::Colours& colours = side.colours[side.sub];
	const int count = StockPalettes::GetCount(side.chara, side.sub);
	const bool asDrawn = colours.stock[part] == LivePalette::kAsDrawn;
	const char* const drawnLabel = part == 0 ? "Original" : "Same as base";

	char label[32] = {};

	if (asDrawn)
		strncpy_s(label, drawnLabel, _TRUNCATE);
	else
		sprintf_s(label, "Colour %02d", colours.stock[part] + 1);

	Ui::SetItemWidth(140.0f);

	if (ImGui::BeginCombo("##stock", label))
	{
		if (ImGui::Selectable(drawnLabel, asDrawn) && !asDrawn)
		{
			Record(player);
			colours.stock[part] = LivePalette::kAsDrawn;
			Apply(player);
		}

		ComboNav::KeepSelectedInView(asDrawn);

		for (int i = 0; i < count; ++i)
		{
			char option[32] = {};
			sprintf_s(option, "Colour %02d", i + 1);

			const bool selected = i == colours.stock[part];

			ImGui::PushID(i);

			if (ImGui::Selectable(option, selected) && !selected)
			{
				Record(player);
				colours.stock[part] = i;
				Apply(player);
			}

			ComboNav::KeepSelectedInView(selected);
			ImGui::PopID();
		}

		ImGui::EndCombo();
	}

	const int steps = ComboNav::WheelSteps();
	const int target = colours.stock[part] + steps;

	if (steps == 0 || target < LivePalette::kAsDrawn || target >= count)
		return;

	Record(player);
	colours.stock[part] = target;
	Apply(player);
}

void PaletteWindow::DrawPartPick(int player, int part)
{
	Side& side = SideOf(player);
	LivePalette::Colours& colours = side.colours[side.sub];

	ImGui::SameLine();

	uint8_t rgb[3] = { 255, 255, 255 };

	if (colours.picked[part])
		memcpy(rgb, colours.pick[part], 3);
	else
		LivePalette::Reference(side.composed[side.sub], side.chara, side.sub, part, rgb);

	uint8_t chosen[3] = {};

	if (PickColour("##partpick", rgb, chosen))
	{
		if (ImGui::IsItemActivated() || !ImGui::IsItemActive())
			Record(player);

		memcpy(colours.pick[part], chosen, 3);
		colours.picked[part] = true;
		Apply(player);
	}

	if (!colours.picked[part])
		return;

	ImGui::SameLine();

	if (ImGui::SmallButton("x"))
	{
		Record(player);
		colours.picked[part] = false;
		Apply(player);
	}

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Go back to the stock colour.");
}

void PaletteWindow::DrawGrid(int player, const unsigned char* entries, int count)
{
	Side& side = SideOf(player);
	const LivePalette::Colours& colours = side.colours[side.sub];

	for (int n = 0; n < count; ++n)
	{
		const int i = entries[n];
		const bool changed = colours.edited[i];

		ImGui::PushID(i);

		if (Swatch("##swatch", side.composed[side.sub] + i * 4, i == side.selected, changed))
		{
			side.selected = i;
			StartFlash(player);
		}

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Entry %d%s", i, changed ? " (changed)" : "");

		NextSwatch(n, count);
		ImGui::PopID();
	}
}

void PaletteWindow::DrawPicker(int player)
{
	Side& side = SideOf(player);
	LivePalette::Colours& colours = side.colours[side.sub];
	const int selected = side.selected;
	const uint8_t* const current = side.composed[side.sub] + selected * 4;

	ImGui::Text("Entry %d%s", selected, colours.edited[selected] ? " (changed)" : "");

	float picked[3] = { current[0] / 255.0f, current[1] / 255.0f, current[2] / 255.0f };

	Ui::SetItemWidth(170.0f);

	if (ImGui::ColorPicker3("##entry", picked, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview))
	{
		if (ImGui::IsItemActivated() || !ImGui::IsItemActive())
			Record(player);

		for (int c = 0; c < 3; ++c)
			colours.entry[selected][c] = static_cast<uint8_t>(picked[c] * 255.0f + 0.5f);

		colours.edited[selected] = true;
		Apply(player);
	}

	DrawPickerButtons(player);

	if (!side.applied)
		ImGui::TextDisabled("Not applied.");
	else if (PalettePaint::IsPainting(player))
		ImGui::TextDisabled("Applied and in use.");
	else
		ImGui::TextDisabled("Applied. Waiting for the character to show up.");
}

void PaletteWindow::DrawPickerButtons(int player)
{
	Side& side = SideOf(player);

	if (ImGui::Button(side.applied ? "Apply again" : "Apply"))
		Apply(player);

	ImGui::SameLine();
	ImGui::BeginDisabled(side.historyCount == 0);
	const bool undo = ImGui::Button("Undo");
	ImGui::EndDisabled();

	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		ImGui::SetTooltip("Undo the last change. %d left.", side.historyCount);

	if (undo)
		Undo(player);

	ImGui::SameLine();
	ImGui::BeginDisabled(!side.applied);
	const bool remove = ImGui::Button("Remove");
	ImGui::EndDisabled();

	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		ImGui::SetTooltip("Puts the game's colours back. Your edits are kept, but this palette is no longer applied "
			"automatically next match.");

	if (remove)
		Remove(player);

	ImGui::SameLine();

	if (ImGui::Button("Reset"))
	{
		Record(player);
		LivePalette::Reset(side.colours[side.sub], side.chara, side.sub);

		if (side.applied)
			Apply(player);
	}

	ImGui::SameLine();

	if (ImGui::Button("Reload"))
	{
		PalettePaint::Clear(player);
		side.applied = false;
		PullBaseline(player, true);
		Refresh(player);
	}

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Reads the character's colours from the game again. Use it if their colour changed after you "
			"opened this. Your edits are kept.");
}

void PaletteWindow::DrawEffects(int player)
{
	const Side& side = SideOf(player);
	unsigned char entries[LivePalette::kColours] = {};
	int count = 0;

	for (int entry = 1; entry < LivePalette::kColours; ++entry)
	{
		if (PartColourTable::IsPartEntry(side.chara, entry) || EffectTable::CountUsing(side.chara, entry) > 0)
			entries[count++] = static_cast<unsigned char>(entry);
	}

	if (count == 0)
	{
		ImGui::TextDisabled("This character's effects do not use its palette.");
		return;
	}

	ImGui::Text("%d entries, %d changed", count, EffectPaint::GetEditedCount(player));
	ImGui::SameLine();

	if (ImGui::Button("Reset effects"))
		EffectPaint::Clear(player);

	ImGui::TextDisabled("Effects use the main palette unless you change an entry here.");

	DrawEffectGrid(player, entries, count);
	DrawEffectPicker(player);
}

void PaletteWindow::DrawEffectGrid(int player, const unsigned char* entries, int count)
{
	Side& side = SideOf(player);

	ImGui::BeginChild("effects", Ui::Scaled(0.0f, 150.0f), ImGuiChildFlags_Borders);

	for (int n = 0; n < count; ++n)
	{
		const int entry = entries[n];
		uint8_t rgb[3] = {};
		const bool observed = EffectPaint::GetObserved(player, entry, rgb);

		if (!observed)
			memcpy(rgb, side.composed[0] + entry * 4, 3);

		uint8_t edit[3] = {};
		const bool edited = EffectPaint::GetRemoteEntry(player, entry, edit) || EffectPaint::GetEdit(player, entry, edit);

		if (edited)
			memcpy(rgb, edit, 3);

		ImGui::PushID(entry);

		if (Swatch("##effect", rgb, entry == side.effectEntry, edited))
			side.effectEntry = entry;

		if (ImGui::IsItemHovered())
		{
			const int parts = PartColourTable::GetPartCount(side.chara, entry);
			const int effects = EffectTable::CountUsing(side.chara, entry);

			ImGui::SetTooltip("Entry %d colours %d part%s in %d effect%s%s", entry, parts, parts == 1 ? "" : "s", effects,
				effects == 1 ? "" : "s", edited ? " (changed)" : "");
		}

		NextSwatch(n, count);
		ImGui::PopID();
	}

	ImGui::EndChild();
}

void PaletteWindow::DrawEffectPicker(int player)
{
	const Side& side = SideOf(player);
	const int entry = side.effectEntry;

	if (entry < 0)
	{
		ImGui::TextDisabled("Pick an entry to change it.");
		return;
	}

	uint8_t rgb[3] = {};

	if (!EffectPaint::GetRemoteEntry(player, entry, rgb) && !EffectPaint::GetEdit(player, entry, rgb) &&
		!EffectPaint::GetObserved(player, entry, rgb))
	{
		memcpy(rgb, side.composed[0] + entry * 4, 3);
	}

	ImGui::Text("Entry %d%s", entry, EffectPaint::IsEdited(player, entry) ? " (changed)" : "");

	float picked[3] = { rgb[0] / 255.0f, rgb[1] / 255.0f, rgb[2] / 255.0f };

	Ui::SetItemWidth(170.0f);

	if (ImGui::ColorPicker3("##effectpick", picked, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview))
	{
		const uint8_t chosen[3] = {
			static_cast<uint8_t>(picked[0] * 255.0f + 0.5f),
			static_cast<uint8_t>(picked[1] * 255.0f + 0.5f),
			static_cast<uint8_t>(picked[2] * 255.0f + 0.5f),
		};

		EffectPaint::SetEntry(player, entry, chosen);
	}

	if (EffectPaint::IsEdited(player, entry) && ImGui::Button("Use the game's colour"))
		EffectPaint::ClearEntry(player, entry);
}

void PaletteWindow::LoadCreator()
{
	if (m_creatorLoaded)
		return;

	m_creatorLoaded = true;

	for (int seat = 0; seat < kSeats; ++seat)
		strncpy_s(SideOf(seat).creator, g_settings.paletteCreator.c_str(), _TRUNCATE);
}

void PaletteWindow::DrawFiles(int player)
{
	LoadCreator();
	PollPngDialogs(player);

	Side& side = SideOf(player);
	const bool mine = PaletteControl::CanEdit(player);
	const bool editingName = ImGui::GetActiveID() == ImGui::GetID("##name");

	if (editingName)
		ImGui::EndDisabled();

	ImGui::TextUnformatted("Name");
	Ui::SetItemWidth(170.0f);
	ImGui::InputText("##name", side.name, sizeof(side.name));

	if (editingName)
		ImGui::BeginDisabled(!mine);

	ImGui::SameLine();

	const bool nameOk = IsUsableName(side.name);

	ImGui::BeginDisabled(!nameOk);
	const bool save = ImGui::Button("Save");
	ImGui::EndDisabled();

	if (save)
	{
		const bool written = Save(player);

		sprintf_s(side.status, written ? "Saved." : "Could not write the file.");
		RefreshFiles(player);

		if (written)
			SelectFile(player, (std::string(side.name) + PaletteFile::kExtension).c_str());
	}

	ImGui::TextUnformatted("Author");
	Ui::SetItemWidth(170.0f);
	ImGui::InputText("##author", side.creator, sizeof(side.creator));

	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		g_settings.paletteCreator = side.creator;
		Settings::SaveString(kSection, "Creator", side.creator);
	}

	ImGui::TextUnformatted("Description");
	Ui::SetItemWidth(340.0f);
	ImGui::InputText("##description", side.description, sizeof(side.description));

	DrawFileChooser(player);
	DrawPngButtons(player);

	if (!nameOk && side.name[0] != '\0')
		ImGui::TextDisabled("This name cannot be used as a file name.");
	else if (side.status[0] != '\0')
		ImGui::TextDisabled("%s", side.status);
}

void PaletteWindow::DrawFileChooser(int player)
{
	Side& side = SideOf(player);

	ImGui::TextUnformatted("Load palette");

	const char* const chosen = side.chosen >= 0 && side.chosen < side.fileCount ? side.files[side.chosen].c_str()
		: kDefaultPalette;

	Ui::SetItemWidth(170.0f);

	if (ImGui::BeginCombo("##load", chosen))
	{
		const bool bare = side.chosen < 0;

		if (ImGui::Selectable(kDefaultPalette, bare))
			Bare(player);

		ComboNav::KeepSelectedInView(bare);

		for (int i = 0; i < side.fileCount; ++i)
		{
			const bool selected = i == side.chosen;

			ImGui::PushID(i);

			if (ImGui::Selectable(side.files[i].c_str(), selected))
			{
				side.chosen = i;
				sprintf_s(side.status, Load(player, side.files[i].c_str()) ? "Loaded." : "Could not read the file.");
			}

			ComboNav::KeepSelectedInView(selected);
			ImGui::PopID();
		}

		ImGui::EndCombo();
	}

	const int steps = ComboNav::WheelSteps();

	if (steps != 0)
	{
		int target = side.chosen + steps;
		target = target < -1 ? -1 : target;
		target = target >= side.fileCount ? side.fileCount - 1 : target;

		if (target != side.chosen && target < 0)
			Bare(player);

		if (target != side.chosen && target >= 0)
		{
			side.chosen = target;
			sprintf_s(side.status, Load(player, side.files[target].c_str()) ? "Loaded." : "Could not read the file.");
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("Refresh list"))
		RefreshFiles(player);
}

void PaletteWindow::DrawPngButtons(int player)
{
	Side& side = SideOf(player);
	const bool importing = side.importDialog.IsRunning();

	ImGui::BeginDisabled(importing);

	if (ImGui::Button(importing ? "Importing..." : "Import PNG..."))
		side.importDialog.BeginOpen("Import palette PNG", "PNG images\0*.png\0");

	ImGui::EndDisabled();

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Uses the colour table of an indexed PNG as this palette. The PNG colours must already be in "
			"the game's order. Colours are not converted or reordered.");

	ImGui::SameLine();

	const bool exporting = side.exportDialog.IsRunning();
	const bool hasBase = BasePals::Has(side.chara, side.sub);

	ImGui::BeginDisabled(exporting || !hasBase);

	if (ImGui::Button(exporting ? "Exporting..." : "Export PNG..."))
	{
		char suggested[96] = {};
		sprintf_s(suggested, "%s.png", side.name[0] != '\0' ? side.name : Characters::Name(side.chara));
		side.exportDialog.BeginSave("Export palette as PNG", "PNG images\0*.png\0", suggested);
	}

	ImGui::EndDisabled();

	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		ImGui::SetTooltip(hasBase ? "Paints these colours on this character's reference image and saves it where you "
			"choose." : "This palette has no reference image.");
}

void PaletteWindow::Record(int player)
{
	Side& side = SideOf(player);

	if (side.historyCount >= kUndoDepth)
	{
		for (int i = 1; i < kUndoDepth; ++i)
			side.history[i - 1] = side.history[i];

		--side.historyCount;
	}

	side.history[side.historyCount++] = { side.sub, side.colours[side.sub] };
}

void PaletteWindow::Undo(int player)
{
	Side& side = SideOf(player);

	if (side.historyCount == 0)
		return;

	const Snapshot& snapshot = side.history[--side.historyCount];

	side.sub = snapshot.sub;
	side.colours[snapshot.sub] = snapshot.colours;

	if (side.applied)
		Apply(player);
	else
		Refresh(player);
}

void PaletteWindow::RefreshFiles(int player)
{
	Side& side = SideOf(player);
	std::string keep;

	if (side.chosen >= 0 && side.chosen < side.fileCount)
		keep = side.files[side.chosen];

	side.fileCount = 0;
	side.chosen = -1;

	if (side.chara < 0)
		return;

	PaletteLibrary::Rescan(side.chara);

	const int count = PaletteLibrary::GetCount(side.chara);

	for (int i = 0; i < count && side.fileCount < kMaxFiles; ++i)
		side.files[side.fileCount++] = PaletteLibrary::GetName(side.chara, i);

	if (!keep.empty())
		SelectFile(player, keep.c_str());
}

void PaletteWindow::SelectFile(int player, const char* file)
{
	Side& side = SideOf(player);
	side.chosen = -1;

	if (file == nullptr || file[0] == '\0')
		return;

	for (int i = 0; i < side.fileCount; ++i)
	{
		if (side.files[i] != file)
			continue;

		side.chosen = i;
		return;
	}
}

void PaletteWindow::Remove(int player)
{
	Side& side = SideOf(player);

	PalettePaint::Clear(player);
	EffectPaint::Clear(player);

	PaletteChoice::Forget(side.chara);
	PaletteChoice::NoteBare(player);

	side.applied = false;
	side.chosen = -1;
}

void PaletteWindow::Bare(int player)
{
	Remove(player);
	sprintf_s(SideOf(player).status, "Using the game's colours.");
}

bool PaletteWindow::Save(int player)
{
	Side& side = SideOf(player);
	const std::unique_ptr<PaletteFile::Content> content = std::make_unique<PaletteFile::Content>();

	g_settings.paletteCreator = side.creator;
	Settings::SaveString(kSection, "Creator", side.creator);

	strncpy_s(content->info.name, side.name, _TRUNCATE);
	strncpy_s(content->info.creator, side.creator, _TRUNCATE);
	strncpy_s(content->info.description, side.description, _TRUNCATE);

	for (int sub = 0; sub < kSubs; ++sub)
	{
		if (sub != 0 && sub != side.sub && !LivePalette::IsCustom(side.colours[sub]))
			continue;

		if (!LivePalette::Compose(side.colours[sub], content->pages[sub]))
			continue;

		content->subMask |= static_cast<uint8_t>(1u << sub);
	}

	content->hasEffects = EffectPaint::GetEditedCount(player) > 0;

	if (content->hasEffects)
		EffectPaint::GetBlock(player, content->effects);

	const std::string folder = PaletteLibrary::FolderFor(side.chara);
	const std::string file = std::string(side.name) + PaletteFile::kExtension;

	if ((content->subMask & 1u) == 0 || !CreateDirectoryTree(folder) || !PaletteFile::Save(folder + "\\" + file, *content))
		return false;

	PaletteChoice::Remember(side.chara, file.c_str());
	PaletteChoice::NoteWorn(player, file.c_str());

	Apply(player);
	return true;
}

void PaletteWindow::ApplyImportedColours(int player, const uint8_t* colours)
{
	Side& side = SideOf(player);
	LivePalette::Colours& target = side.colours[side.sub];

	Record(player);
	LivePalette::Reset(target, side.chara, side.sub);

	for (int i = 1; i < LivePalette::kColours; ++i)
	{
		memcpy(target.entry[i], colours + i * 4, 3);
		target.edited[i] = true;
	}

	Apply(player);
}

bool PaletteWindow::Load(int player, const char* name)
{
	Side& side = SideOf(player);
	const std::unique_ptr<PaletteFile::Content> content = std::make_unique<PaletteFile::Content>();

	if (!PaletteFile::Load(PaletteLibrary::FolderFor(side.chara) + "\\" + name, *content))
		return false;

	strncpy_s(side.creator, content->info.creator, _TRUNCATE);
	strncpy_s(side.description, content->info.description, _TRUNCATE);

	Record(player);

	for (int sub = 0; sub < kSubs; ++sub)
	{
		LivePalette::Colours& colours = side.colours[sub];
		LivePalette::Reset(colours, side.chara, sub);

		if ((content->subMask & (1u << sub)) == 0)
			continue;

		for (int i = 1; i < LivePalette::kColours; ++i)
		{
			memcpy(colours.entry[i], content->pages[sub] + i * 4, 3);
			colours.edited[i] = true;
		}
	}

	EffectPaint::SetBlock(player, content->hasEffects ? content->effects : nullptr);
	PalettePaint::Clear(player);
	Apply(player);

	PaletteChoice::Remember(side.chara, name);
	PaletteChoice::NoteWorn(player, name);

	strncpy_s(side.name, content->info.name[0] != '\0' ? content->info.name : name, _TRUNCATE);
	StripExtension(side.name);
	return true;
}

void PaletteWindow::PollPngDialogs(int player)
{
	Side& side = SideOf(player);
	std::string path;

	if (side.importDialog.TakeResult(path))
		CompleteImportPng(player, path);

	if (side.exportDialog.TakeResult(path))
		CompleteExportPng(player, path);
}

void PaletteWindow::CompleteImportPng(int player, const std::string& path)
{
	Side& side = SideOf(player);
	uint8_t colours[PaletteFile::kBytes] = {};
	std::string error;

	if (!PngPalette::Read(path, colours, error))
	{
		sprintf_s(side.status, "%.120s", error.c_str());
		return;
	}

	ApplyImportedColours(player, colours);

	const size_t slash = path.find_last_of("\\/");
	std::string stem = path.substr(slash == std::string::npos ? 0 : slash + 1);
	const size_t dot = stem.find_last_of('.');

	if (dot != std::string::npos)
		stem.resize(dot);

	strncpy_s(side.name, stem.c_str(), _TRUNCATE);
	side.chosen = -1;
	sprintf_s(side.status, "Imported '%s'. Save to keep it as a .pal.", side.name);
}

void PaletteWindow::CompleteExportPng(int player, const std::string& path)
{
	Side& side = SideOf(player);
	const uint8_t* base = nullptr;
	size_t size = 0;

	if (!BasePals::Get(side.chara, side.sub, base, size))
	{
		sprintf_s(side.status, "This palette has no reference image.");
		return;
	}

	std::string target = path;

	if (target.size() < 4 || _stricmp(target.c_str() + target.size() - 4, ".png") != 0)
		target += ".png";

	std::string error;

	if (!PngPalette::Recolour(target, base, size, side.composed[side.sub], error))
	{
		sprintf_s(side.status, "%.120s", error.c_str());
		return;
	}

	sprintf_s(side.status, "Exported.");
}

void PaletteWindow::Adopt(int player, int chara)
{
	Side& side = SideOf(player);

	for (int sub = 0; sub < kSubs; ++sub)
	{
		LivePalette::Reset(side.colours[sub], chara, sub);
		side.pulled[sub] = false;
	}

	side.chara = chara;
	side.generation = PaletteChoice::GetGeneration(player);
	side.applied = false;
	side.sub = 0;
	side.selected = 1;
	side.effectEntry = -1;
	side.historyCount = 0;
	side.status[0] = '\0';
	side.chosen = -1;

	RefreshFiles(player);

	char worn[PaletteFile::kNameLength + 8] = {};
	strncpy_s(worn, PaletteChoice::WornFile(player), _TRUNCATE);

	if (worn[0] == '\0')
		return;

	SelectFile(player, worn);

	if (Load(player, worn))
	{
		side.historyCount = 0;
		return;
	}

	sprintf_s(side.status, "Could not read '%s'.", worn);
}

void PaletteWindow::PullBaseline(int player, bool force)
{
	Side& side = SideOf(player);

	if (side.pulled[side.sub] && !force)
		return;

	uint8_t drawn[LivePalette::kBytes] = {};

	if (!PalettePaint::ReadGameColours(player, side.sub, drawn))
		return;

	LivePalette::SetBaseline(side.colours[side.sub], drawn);
	side.pulled[side.sub] = true;
}

void PaletteWindow::Refresh(int player)
{
	Side& side = SideOf(player);

	for (int sub = 0; sub < kSubs; ++sub)
		LivePalette::Compose(side.colours[sub], side.composed[sub]);
}

void PaletteWindow::Apply(int player)
{
	Side& side = SideOf(player);

	Refresh(player);

	for (int sub = 0; sub < kSubs; ++sub)
	{
		const bool wanted = LivePalette::IsCustom(side.colours[sub]) || (sub == side.sub && side.pulled[sub]);

		if (wanted && StockPalettes::HasSub(side.chara, sub))
			PalettePaint::Stage(player, sub, side.composed[sub]);
	}

	side.applied = true;
}

void PaletteWindow::StartFlash(int player)
{
	if (!g_settings.paletteFlashEntry || !PaletteControl::CanEdit(player))
		return;

	if (m_flashPlayer >= 0 && m_flashPlayer != player)
	{
		PalettePaint::EndPreview(m_flashPlayer);
		EffectPaint::EndPreview(m_flashPlayer);
	}

	m_flashPlayer = player;
	m_flashFrames = kFlashFrames;
}

void PaletteWindow::RunFlash()
{
	const int player = m_flashPlayer;

	if (player < 0)
		return;

	if (m_flashFrames <= 0 || !g_settings.paletteFlashEntry || !PaletteControl::CanEdit(player))
	{
		PalettePaint::EndPreview(player);
		EffectPaint::EndPreview(player);
		m_flashPlayer = -1;
		return;
	}

	--m_flashFrames;

	const Side& side = SideOf(player);
	const int keep = side.selected;
	const bool on = (m_flashFrames / kFlashBlink) % 2 == 0;
	const uint8_t lit[3] = { static_cast<uint8_t>(on ? 255 : 0), static_cast<uint8_t>(on ? 0 : 255), 255 };

	uint8_t dimmed[LivePalette::kBytes] = {};
	memcpy(dimmed, side.composed[side.sub], sizeof(dimmed));

	for (int i = 0; i < LivePalette::kColours; ++i)
	{
		const uint8_t grey = static_cast<uint8_t>(Luminance(dimmed + i * 4) / 2);

		dimmed[i * 4 + 0] = grey;
		dimmed[i * 4 + 1] = grey;
		dimmed[i * 4 + 2] = grey;
	}

	memcpy(dimmed + keep * 4, lit, 3);
	PalettePaint::Preview(player, side.sub, dimmed);

	if (side.sub != 0)
		return;

	const uint8_t dark[3] = { 40, 40, 40 };
	EffectPaint::PreviewObserved(player, dark, keep, lit);
}
