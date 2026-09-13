#include "Overlay/Window/FrameMeterLegendWindow.h"

#include "Overlay/UiScale.h"
#include "Training/FrameMeter.h"

namespace {

struct Sample
{
	FrameMeter::State state;
	int frames;
};

constexpr Sample kSample[] = {
	{ FrameMeter::State::Startup, 6 },
	{ FrameMeter::State::Active, 3 },
	{ FrameMeter::State::Recovery, 9 },
	{ FrameMeter::State::Idle, 6 },
};

constexpr int kSampleInvulnFrom = 1;
constexpr int kSampleInvulnTo = 7;
constexpr int kPerRow = 3;
constexpr float kColumnWidth = 190.0f;
constexpr float kTopShade = 1.25f;
constexpr float kBottomShade = 0.72f;

constexpr FrameMeter::State kStates[] = {
	FrameMeter::State::Startup,
	FrameMeter::State::Active,
	FrameMeter::State::Recovery,
	FrameMeter::State::Cancellable,
	FrameMeter::State::Blockstun,
	FrameMeter::State::Hitstun,
	FrameMeter::State::Shield,
	FrameMeter::State::Armor,
	FrameMeter::State::Movement,
	FrameMeter::State::Jump,
	FrameMeter::State::AirMovement,
	FrameMeter::State::Idle,
};

ImU32 Rgba(uint32_t argb)
{
	return IM_COL32((argb >> 16) & 0xFF, (argb >> 8) & 0xFF, argb & 0xFF, (argb >> 24) & 0xFF);
}

int Channel(uint32_t color, int shift, float factor)
{
	const float value = static_cast<float>((color >> shift) & 0xFF) * factor;
	return static_cast<int>(value > 255.0f ? 255.0f : value);
}

ImU32 Shaded(uint32_t color, float factor)
{
	return IM_COL32(Channel(color, 16, factor), Channel(color, 8, factor), Channel(color, 0, factor), 255);
}

void Cell(ImDrawList* draw, ImVec2 at, float width, float height, uint32_t color)
{
	const ImU32 top = Shaded(color, kTopShade);
	const ImU32 bottom = Shaded(color, kBottomShade);

	draw->AddRectFilledMultiColor(at, ImVec2(at.x + width, at.y + height), top, top, bottom, bottom);
}

void DrawSampleMeter()
{
	const float cellWidth = Ui::Scaled(11.0f);
	const float gap = Ui::Scaled(1.0f);
	const float rowHeight = Ui::Scaled(20.0f);
	const float attributeHeight = Ui::Scaled(9.0f);
	const float attributeGap = Ui::Scaled(3.0f);

	ImDrawList* const draw = ImGui::GetWindowDrawList();
	const ImVec2 origin = ImGui::GetCursorScreenPos();
	int cell = 0;

	for (const Sample& sample : kSample)
	{
		for (int frame = 0; frame < sample.frames; ++frame, ++cell)
		{
			const ImVec2 at(origin.x + cell * cellWidth, origin.y);
			Cell(draw, at, cellWidth - gap, rowHeight, FrameMeter::GetStateColor(sample.state));

			if (cell < kSampleInvulnFrom || cell > kSampleInvulnTo)
				continue;

			const ImVec2 attributeAt(at.x, origin.y + rowHeight + attributeGap);
			draw->AddRectFilled(attributeAt, ImVec2(attributeAt.x + cellWidth - gap, attributeAt.y + attributeHeight),
				Rgba(FrameMeter::GetMarkerColor(FrameMeter::Marker_StrikeInvuln)));
		}
	}

	const float width = cell * cellWidth;

	draw->AddRect(ImVec2(origin.x - 1.0f, origin.y - 1.0f), ImVec2(origin.x + width, origin.y + rowHeight + 1.0f),
		IM_COL32(255, 255, 255, 48));

	ImGui::Dummy(ImVec2(width, rowHeight + attributeGap + attributeHeight));
}

void Entry(uint32_t color, const char* name, int placed)
{
	if (placed % kPerRow != 0)
		ImGui::SameLine(0.0f, Ui::Scaled(8.0f));

	const ImVec2 at = ImGui::GetCursorScreenPos();
	const float size = ImGui::GetTextLineHeight();

	ImGui::GetWindowDrawList()->AddRectFilled(at, ImVec2(at.x + size, at.y + size), Rgba(color));
	ImGui::GetWindowDrawList()->AddRect(at, ImVec2(at.x + size, at.y + size), IM_COL32(0, 0, 0, 160));

	ImGui::Dummy(ImVec2(size, size));
	ImGui::SameLine(0.0f, Ui::Scaled(6.0f));
	ImGui::TextUnformatted(name);
	ImGui::SameLine(0.0f, 0.0f);
	ImGui::Dummy(ImVec2(Ui::Scaled(kColumnWidth) - (ImGui::GetItemRectMax().x - at.x), 0.0f));
}

void MarkerEntries(int from, int to)
{
	int placed = 0;

	for (int i = from; i < to; ++i, ++placed)
	{
		const FrameMeter::Marker marker = static_cast<FrameMeter::Marker>(i);
		Entry(FrameMeter::GetMarkerColor(marker), FrameMeter::GetMarkerName(marker), placed);
	}
}

}

FrameMeterLegendWindow::FrameMeterLegendWindow(const std::string& title, bool closable, ImGuiWindowFlags windowFlags)
	: IWindow(title, closable, windowFlags)
{
}

void FrameMeterLegendWindow::BeforeDraw()
{
	const ImGuiViewport* const viewport = ImGui::GetMainViewport();
	const ImVec2 work = viewport->WorkSize;

	ImGui::SetNextWindowSizeConstraints(Ui::Scaled(420.0f, 200.0f), work);
}

void FrameMeterLegendWindow::Draw()
{
	ImGui::TextUnformatted("Startup 7F  /  Total 18F  /  Advantage +4F");
	DrawSampleMeter();

	ImGui::Spacing();
	ImGui::SeparatorText("The bar");

	int placed = 0;

	for (FrameMeter::State state : kStates)
		Entry(FrameMeter::GetStateColor(state), FrameMeter::GetStateName(state), placed++);

	ImGui::NewLine();
	MarkerEntries(0, FrameMeter::kFirstInvulnMarker);

	ImGui::Spacing();
	ImGui::SeparatorText("Status row");
	ImGui::TextWrapped("Everything in force on that frame at once, the cell split evenly between them. White means "
		"nothing can connect and is drawn on its own.");
	MarkerEntries(FrameMeter::kFirstInvulnMarker, FrameMeter::Marker_COUNT);

	ImGui::Spacing();
	ImGui::SeparatorText("The numbers");
	ImGui::BulletText("Startup - until the move can connect, first active frame included.");
	ImGui::BulletText("Total - from the move starting to it ending.");
	ImGui::BulletText("Advantage - who acts first, positive meaning you. The number in brackets is the advantage "
		"before the opponent teched.");
	ImGui::BulletText("Blockstun and hitstun - how long the opponent was held.");
	ImGui::BulletText("Gap - free frames between two held runs.");
	ImGui::BulletText("Flash - how long a super flash ran inside the move. The bar never gains a cell for it.");
}
