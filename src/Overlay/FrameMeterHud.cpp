#include "Overlay/FrameMeterHud.h"

#include "Core/Settings.h"
#include "Core/interfaces.h"
#include "D3D9/DeviceHooks.h"
#include "D3D9/GameFont.h"
#include "D3D9/QuadRenderer.h"
#include "Game/GameOffsets.h"
#include "Overlay/WindowManager.h"
#include "Training/FrameMeter.h"
#include "Training/GameState.h"

#include <imgui.h>

#include <cstdio>

namespace {

constexpr const char* kSection = "FrameMeter";

constexpr int kVisibleFrames = 90;

constexpr float kCellWidth = 9.0f;
constexpr float kCellGap = 1.0f;
constexpr float kRowHeight = 18.0f;
constexpr float kRowGap = 4.0f;
constexpr float kTextGap = 3.0f;
constexpr float kAttrRowHeight = 12.0f;
constexpr float kAttrRowGap = 2.0f;
constexpr int kMaxAttrSlices = 3;
constexpr float kAttrSliceEdge = 1.0f;
constexpr float kTrackWidth = kVisibleFrames * kCellWidth;
constexpr float kOwnerStripHeight = 4.0f;
constexpr float kCounterPadding = 8.0f;

constexpr float kTextPixels = 16.0f;
constexpr float kCounterPixels = 13.0f;
constexpr float kShadowPixels = 20.0f;

constexpr float kAutoHeightShare = 0.87f;
constexpr float kKeepVisibleX = 60.0f;
constexpr float kKeepVisibleY = 40.0f;

constexpr float kPreviousLapDim = 0.45f;
constexpr float kTopShade = 1.25f;
constexpr float kBottomShade = 0.72f;

constexpr uint32_t kEmptyCell = 0xD8121316;
constexpr uint32_t kCellEdge = 0x30FFFFFF;
constexpr uint32_t kAttrSliceEdgeColor = 0xC0000000;
constexpr uint32_t kText = 0xFFFFFFFF;
constexpr uint32_t kPreviousLapText = 0x80FFFFFF;
constexpr uint32_t kTextShadow = 0xE0000000;
constexpr uint32_t kActionName = 0xFFB0B8C8;
constexpr uint32_t kAdvPlus = 0xFF8FD8B0;
constexpr uint32_t kAdvMinus = 0xFFE09090;
constexpr uint32_t kCounterBg = 0xFF101216;
constexpr uint32_t kCounterEdge = 0xFFFFFFFF;

struct Placement
{
	float x;
	float y;
	float width;
	float height;
	float scale;
	float lineHeight;
};

bool g_fontTried = false;
bool g_dragging = false;
float g_dragOffsetX = 0.0f;
float g_dragOffsetY = 0.0f;

float TextScale(float s)
{
	return kTextPixels * s / GameFont::GetLineHeight();
}

float CounterScale(float s)
{
	return kCounterPixels * s / GameFont::GetLineHeight();
}

uint32_t Fade(uint32_t color)
{
	const int opacity = g_settings.frameMeterOpacity;

	if (opacity >= 100)
		return color;

	const uint32_t alpha = ((color >> 24) & 0xFF) * static_cast<uint32_t>(opacity < 0 ? 0 : opacity) / 100u;
	return (alpha << 24) | (color & 0x00FFFFFF);
}

uint32_t Shade(uint32_t color, float factor)
{
	const uint32_t alpha = (color >> 24) & 0xFF;
	uint32_t red = static_cast<uint32_t>(((color >> 16) & 0xFF) * factor);
	uint32_t green = static_cast<uint32_t>(((color >> 8) & 0xFF) * factor);
	uint32_t blue = static_cast<uint32_t>((color & 0xFF) * factor);

	red = red > 255 ? 255 : red;
	green = green > 255 ? 255 : green;
	blue = blue > 255 ? 255 : blue;

	return Fade((alpha << 24) | (red << 16) | (green << 8) | blue);
}

float Clamp(float value, float highest)
{
	const float capped = value > highest ? highest : value;
	return capped < 0.0f ? 0.0f : capped;
}

void DrawShadowedText(const char* text, float x, float y, float scale, uint32_t color)
{
	const float offset = GameFont::GetLineHeight() * scale > kShadowPixels ? 2.0f : 1.0f;

	GameFont::Draw(text, x + offset, y + offset, scale, Fade(kTextShadow));
	GameFont::Draw(text, x, y, scale, Fade(color));
}

void DrawCell(float x, float y, float width, float height, uint32_t color)
{
	QuadRenderer::FillRectVertical(x, y, width, height, Shade(color, kTopShade), Shade(color, kBottomShade));
}

bool TrackSlot(int length, int slot, int& outFrame, bool& outPrevious)
{
	const int lap = length / kVisibleFrames;
	const int position = length % kVisibleFrames;

	if (slot < position)
	{
		outFrame = lap * kVisibleFrames + slot;
		outPrevious = false;
		return true;
	}

	if (lap == 0)
		return false;

	outFrame = (lap - 1) * kVisibleFrames + slot;
	outPrevious = true;
	return true;
}

bool SlotFrame(int length, int player, int slot, FrameMeter::Frame& out, bool& outPrevious)
{
	int index = 0;
	return TrackSlot(length, slot, index, outPrevious) && FrameMeter::GetFrame(player, index, out);
}

void DrawTrack(float x, float y, float s, int player, int length)
{
	const float cellWidth = (kCellWidth - kCellGap) * s;
	const float rowHeight = kRowHeight * s;

	for (int slot = 0; slot < kVisibleFrames; ++slot)
	{
		const float cellX = x + slot * kCellWidth * s;
		FrameMeter::Frame frame = {};
		bool previousLap = false;

		DrawCell(cellX, y, cellWidth, rowHeight, kEmptyCell);

		if (!SlotFrame(length, player, slot, frame, previousLap) || frame.state == FrameMeter::State::None)
			continue;

		const float dim = previousLap ? kPreviousLapDim : 1.0f;
		const uint32_t stateColor = Shade(FrameMeter::GetStateColor(frame.state), dim);

		if (frame.projectileActive)
		{
			DrawCell(cellX, y, cellWidth, rowHeight, Shade(FrameMeter::GetMarkerColor(FrameMeter::Marker_Projectile), dim));
			QuadRenderer::FillRect(cellX, y, cellWidth, kOwnerStripHeight * s, stateColor);
		}
		else
		{
			DrawCell(cellX, y, cellWidth, rowHeight, stateColor);
		}

		if (frame.teching)
			DrawCell(cellX, y, cellWidth, rowHeight, Shade(FrameMeter::GetMarkerColor(FrameMeter::Marker_Tech), dim));
	}

	QuadRenderer::StrokeRect(x - s, y - s, kTrackWidth * s + s * 2.0f, rowHeight + s * 2.0f, s, Fade(kCellEdge));
}

int SliceCount(uint16_t invuln)
{
	int slices = 0;

	for (int m = FrameMeter::kFirstInvulnMarker; m < FrameMeter::Marker_COUNT; ++m)
		slices += (invuln & FrameMeter::GetMarkerInvulnBit(static_cast<FrameMeter::Marker>(m))) != 0 ? 1 : 0;

	return slices > kMaxAttrSlices ? kMaxAttrSlices : slices;
}

void DrawAttributeCell(float cellX, float y, float cellWidth, float rowHeight, float s, uint16_t invuln, float dim)
{
	const int slices = SliceCount(invuln);

	if (slices == 0)
		return;

	const float sliceHeight = rowHeight / static_cast<float>(slices);
	int drawn = 0;

	for (int m = FrameMeter::kFirstInvulnMarker; m < FrameMeter::Marker_COUNT && drawn < slices; ++m)
	{
		const FrameMeter::Marker marker = static_cast<FrameMeter::Marker>(m);

		if ((invuln & FrameMeter::GetMarkerInvulnBit(marker)) == 0)
			continue;

		const float sliceY = y + drawn * sliceHeight;
		QuadRenderer::FillRect(cellX, sliceY, cellWidth, sliceHeight, Shade(FrameMeter::GetMarkerColor(marker), dim));

		if (drawn + 1 < slices)
		{
			QuadRenderer::FillRect(cellX, sliceY + sliceHeight - kAttrSliceEdge * s, cellWidth, kAttrSliceEdge * s,
				Fade(kAttrSliceEdgeColor));
		}

		++drawn;
	}
}

void DrawAttributeTrack(float x, float y, float s, int player, int length)
{
	const float cellWidth = (kCellWidth - kCellGap) * s;
	const float rowHeight = kAttrRowHeight * s;

	for (int slot = 0; slot < kVisibleFrames; ++slot)
	{
		const float cellX = x + slot * kCellWidth * s;
		FrameMeter::Frame frame = {};
		bool previousLap = false;

		QuadRenderer::FillRect(cellX, y, cellWidth, rowHeight, Fade(kEmptyCell));

		if (!SlotFrame(length, player, slot, frame, previousLap) || frame.invuln == 0)
			continue;

		DrawAttributeCell(cellX, y, cellWidth, rowHeight, s, frame.invuln, previousLap ? kPreviousLapDim : 1.0f);
	}

	QuadRenderer::StrokeRect(x - s, y - s, kTrackWidth * s + s * 2.0f, rowHeight + s * 2.0f, s, Fade(kCellEdge));
}

bool RunAt(int length, int player, int slot, int& outEnd, FrameMeter::State& outState, bool& outPreviousLap)
{
	FrameMeter::Frame frame = {};

	if (!SlotFrame(length, player, slot, frame, outPreviousLap))
		return false;

	outState = frame.state;
	outEnd = slot;

	while (outEnd + 1 < kVisibleFrames)
	{
		FrameMeter::Frame next = {};
		bool nextPreviousLap = false;

		if (!SlotFrame(length, player, outEnd + 1, next, nextPreviousLap) || nextPreviousLap != outPreviousLap ||
			next.state != outState)
		{
			break;
		}

		++outEnd;
	}

	return true;
}

void DrawRunCount(float x, float y, float s, int start, int end, bool previousLap)
{
	char text[8] = {};
	sprintf_s(text, "%d", end - start + 1);

	const float scale = CounterScale(s);
	const float cellWidth = (kCellWidth - kCellGap) * s;
	const float textWidth = GameFont::MeasureWidth(text, scale);
	const float runWidth = (end - start) * kCellWidth * s + cellWidth;

	if (textWidth + s > runWidth)
		return;

	const float right = x + end * kCellWidth * s + cellWidth;
	const float textY = y + (kRowHeight * s - GameFont::GetLineHeight() * scale) * 0.5f;

	DrawShadowedText(text, right - textWidth - s, textY, scale, previousLap ? kPreviousLapText : kText);
}

void DrawRunCounts(float x, float y, float s, int player, int length)
{
	const int position = length % kVisibleFrames;
	const int newest = position > 0 ? position - 1 : kVisibleFrames - 1;
	const bool trailing = FrameMeter::GetTrailingRunLength(player) > 0;

	for (int slot = 0; slot < kVisibleFrames;)
	{
		int end = 0;
		FrameMeter::State state = FrameMeter::State::None;
		bool previousLap = false;

		if (!RunAt(length, player, slot, end, state, previousLap))
		{
			++slot;
			continue;
		}

		const int start = slot;
		slot = end + 1;

		if (state == FrameMeter::State::None || state == FrameMeter::State::Idle)
			continue;

		if (trailing && start <= newest && newest <= end)
			continue;

		DrawRunCount(x, y, s, start, end, previousLap);
	}
}

bool DrawLineTotals(float x, float y, float s, int player, int length)
{
	int blockstun = 0;
	int hitstun = 0;
	int gap = 0;
	int pendingGap = 0;
	bool held = false;

	for (int i = 0; i < length; ++i)
	{
		FrameMeter::Frame frame = {};

		if (!FrameMeter::GetFrame(player, i, frame))
			continue;

		const bool blocked = frame.state == FrameMeter::State::Blockstun;
		const bool hit = frame.state == FrameMeter::State::Hitstun;

		if (!blocked && !hit)
		{
			pendingGap += held ? 1 : 0;
			continue;
		}

		blockstun += blocked ? 1 : 0;
		hitstun += hit ? 1 : 0;
		gap += pendingGap;
		pendingGap = 0;
		held = true;
	}

	const int flash = FrameMeter::GetFlashFrames(player);

	if (blockstun == 0 && hitstun == 0 && flash == 0)
		return false;

	char text[128] = {};
	int at = 0;

	if (blockstun > 0)
		at += sprintf_s(text + at, sizeof(text) - at, "blockstun %d", blockstun);

	if (hitstun > 0)
		at += sprintf_s(text + at, sizeof(text) - at, "%shitstun %d", at > 0 ? "  " : "", hitstun);

	if (gap > 0)
		at += sprintf_s(text + at, sizeof(text) - at, "%sgap %d", at > 0 ? "  " : "", gap);

	if (flash > 0)
		sprintf_s(text + at, sizeof(text) - at, "%sflash %d", at > 0 ? "  " : "", flash);

	DrawShadowedText(text, x, y, TextScale(s), kActionName);
	return true;
}

void DrawTrailingCounter(float x, float y, float s, int player, int length)
{
	const int run = FrameMeter::GetTrailingRunLength(player);
	const int drawn = length % kVisibleFrames;

	if (run <= 0 || drawn <= 0)
		return;

	char text[8] = {};
	sprintf_s(text, "%d", run);

	const float scale = CounterScale(s);
	const float textWidth = GameFont::MeasureWidth(text, scale);
	const float boxWidth = textWidth + kCounterPadding * s;
	const float boxHeight = kRowHeight * s;
	const float rightEdge = x + kTrackWidth * s - boxWidth;
	const float boxX = x + drawn * kCellWidth * s > rightEdge ? rightEdge : x + drawn * kCellWidth * s;

	QuadRenderer::FillRect(boxX, y, boxWidth, boxHeight, Fade(kCounterBg));
	QuadRenderer::StrokeRect(boxX, y, boxWidth, boxHeight, s, Fade(kCounterEdge));

	GameFont::Draw(text, boxX + (boxWidth - textWidth) * 0.5f, y + (boxHeight - GameFont::GetLineHeight() * scale) * 0.5f,
		scale, Fade(kText));
}

float DrawAdvantage(float x, float y, float scale, int player)
{
	int value = 0;

	if (!FrameMeter::GetAdvantageFor(player, value))
	{
		DrawShadowedText("--", x, y, scale, kText);
		return x + GameFont::MeasureWidth("--", scale);
	}

	char advantage[16] = {};
	sprintf_s(advantage, "%+dF", value);
	DrawShadowedText(advantage, x, y, scale, value >= 0 ? kAdvPlus : kAdvMinus);

	float at = x + GameFont::MeasureWidth(advantage, scale);
	int afterRecovery = 0;

	if (!FrameMeter::GetAdvantageAfterRecoveryFor(player, afterRecovery))
		return at;

	char bracket[16] = {};
	sprintf_s(bracket, " (%+dF)", afterRecovery);
	DrawShadowedText(bracket, at, y, scale, afterRecovery >= 0 ? kAdvPlus : kAdvMinus);
	return at + GameFont::MeasureWidth(bracket, scale);
}

void DrawReadout(float x, float y, float s, int player)
{
	char text[96] = {};
	int startup = 0;
	int active = 0;
	int recovery = 0;
	int total = 0;

	if (FrameMeter::GetLastMove(player, startup, active, recovery, total))
		sprintf_s(text, "Startup %dF / Total %dF / Advantage ", startup, total);
	else
		sprintf_s(text, "Startup -- / Total -- / Advantage ");

	const float scale = TextScale(s);
	DrawShadowedText(text, x, y, scale, kText);

	const float at = DrawAdvantage(x + GameFont::MeasureWidth(text, scale), y, scale, player);
	const char* const action = FrameMeter::GetActionName(player);

	if (action[0] == '\0')
		return;

	char named[64] = {};
	sprintf_s(named, "   %s", action);
	DrawShadowedText(named, at, y, scale, kActionName);
}

void SavePosition()
{
	Settings::SaveInt(kSection, "PositionX", g_settings.frameMeterX);
	Settings::SaveInt(kSection, "PositionY", g_settings.frameMeterY);
}

bool DragMeter(const D3DVIEWPORT9& viewport, const Placement& placement)
{
	if (ImGui::GetCurrentContext() == nullptr)
		return false;

	const ImGuiIO& io = ImGui::GetIO();

	if (!g_settings.frameMeterDrag || g_settings.frameMeterAuto || !WindowManager::GetInstance().IsInteractive() ||
		(io.WantCaptureMouse && !g_dragging) || io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f)
	{
		g_dragging = false;
		return false;
	}

	const float mouseX = io.MousePos.x * (viewport.Width / io.DisplaySize.x);
	const float mouseY = io.MousePos.y * (viewport.Height / io.DisplaySize.y);

	if (!io.MouseDown[0])
	{
		if (g_dragging)
			SavePosition();

		g_dragging = false;
		return false;
	}

	if (!g_dragging)
	{
		const bool over = mouseX >= placement.x && mouseX < placement.x + placement.width && mouseY >= placement.y &&
			mouseY < placement.y + placement.height;

		if (!over || !ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			return false;

		g_dragging = true;
		g_dragOffsetX = mouseX - placement.x;
		g_dragOffsetY = mouseY - placement.y;
	}

	g_settings.frameMeterX = static_cast<int>(Clamp(mouseX - g_dragOffsetX, viewport.Width - kKeepVisibleX));
	g_settings.frameMeterY = static_cast<int>(Clamp(mouseY - g_dragOffsetY, viewport.Height - kKeepVisibleY));
	return true;
}

Placement Place(const D3DVIEWPORT9& viewport)
{
	Placement placement = {};
	placement.scale = g_settings.frameMeterScale * DeviceHooks::OverlayScale();
	placement.lineHeight = kTextPixels * placement.scale;
	placement.width = kTrackWidth * placement.scale;

	const float totals = g_settings.frameMeterTotals ? (placement.lineHeight + kTextGap * placement.scale) * 2.0f : 0.0f;
	const float attributes = g_settings.frameMeterAttributes ? (kAttrRowHeight + kAttrRowGap) * placement.scale : 0.0f;

	placement.height = placement.lineHeight * 2.0f + kTextGap * placement.scale * 2.0f +
		kRowHeight * placement.scale * 2.0f + kRowGap * placement.scale + attributes * 2.0f + totals;

	const float autoX = (viewport.Width - placement.width) * 0.5f;
	const float autoY = viewport.Height * kAutoHeightShare - placement.height;
	const bool unplaced = g_settings.frameMeterX < 0 || g_settings.frameMeterY < 0;

	if (!g_settings.frameMeterAuto && unplaced)
	{
		g_settings.frameMeterX = static_cast<int>(Clamp(autoX, viewport.Width - kKeepVisibleX));
		g_settings.frameMeterY = static_cast<int>(Clamp(autoY, viewport.Height - kKeepVisibleY));
		SavePosition();
	}

	const float x = g_settings.frameMeterAuto ? autoX : static_cast<float>(g_settings.frameMeterX);
	const float y = g_settings.frameMeterAuto ? autoY : static_cast<float>(g_settings.frameMeterY);

	placement.x = Clamp(x, viewport.Width - kKeepVisibleX);
	placement.y = Clamp(y, viewport.Height - kKeepVisibleY);
	return placement;
}

void DrawTotalsLine(const Placement& placement, float& cursor, int player, int length)
{
	if (!g_settings.frameMeterTotals || !DrawLineTotals(placement.x, cursor, placement.scale, player, length))
		return;

	cursor += placement.lineHeight + kTextGap * placement.scale;
}

void DrawReadoutLine(const Placement& placement, float& cursor, int player)
{
	DrawReadout(placement.x, cursor, placement.scale, player);
	cursor += placement.lineHeight + kTextGap * placement.scale;
}

void DrawTrackBlock(const Placement& placement, float& cursor, int player, int length)
{
	DrawTrack(placement.x, cursor, placement.scale, player, length);

	if (g_settings.frameMeterCounts)
		DrawRunCounts(placement.x, cursor, placement.scale, player, length);

	DrawTrailingCounter(placement.x, cursor, placement.scale, player, length);
	cursor += kRowHeight * placement.scale;

	if (!g_settings.frameMeterAttributes)
		return;

	cursor += kAttrRowGap * placement.scale;
	DrawAttributeTrack(placement.x, cursor, placement.scale, player, length);
	cursor += kAttrRowHeight * placement.scale;
}

void EnsureFont(IDirect3DDevice9* device)
{
	if (g_fontTried)
		return;

	g_fontTried = true;
	GameFont::Load(device, GameOffsets::Meter::kFontPath);
}

void Render(IDirect3DDevice9* device)
{
	if (!g_settings.frameMeterVisible || device == nullptr || !GameState::AllowsTrainingTools())
		return;

	EnsureFont(device);

	const D3DPRESENT_PARAMETERS& present = DeviceHooks::GetPresentParameters();
	D3DVIEWPORT9 viewport = {};
	viewport.Width = present.BackBufferWidth;
	viewport.Height = present.BackBufferHeight;

	if (viewport.Width == 0 || viewport.Height == 0)
		return;

	Placement placement = Place(viewport);

	if (DragMeter(viewport, placement))
	{
		placement.x = static_cast<float>(g_settings.frameMeterX);
		placement.y = static_cast<float>(g_settings.frameMeterY);
	}

	if (!QuadRenderer::Begin(device))
		return;

	const int length = FrameMeter::GetLength();
	float cursor = placement.y;

	DrawTotalsLine(placement, cursor, 0, length);
	DrawReadoutLine(placement, cursor, 0);
	DrawTrackBlock(placement, cursor, 0, length);

	cursor += kRowGap * placement.scale;

	DrawTrackBlock(placement, cursor, 1, length);
	cursor += kTextGap * placement.scale;
	DrawReadoutLine(placement, cursor, 1);
	DrawTotalsLine(placement, cursor, 1, length);

	QuadRenderer::End();
}

class HudListener final : public IDeviceListener
{
public:
	void OnDeviceLost() override
	{
		QuadRenderer::OnDeviceLost();
	}

	void OnPresent(IDirect3DDevice9* device) override
	{
		Render(device);
	}
};

HudListener g_listener;

}

IDeviceListener* FrameMeterHud::Listener()
{
	return &g_listener;
}

bool FrameMeterHud::IsVisible()
{
	return g_settings.frameMeterVisible;
}

void FrameMeterHud::SetVisible(bool visible)
{
	g_settings.frameMeterVisible = visible;
	Settings::SaveBool(kSection, "Visible", visible);

	if (!visible)
		FrameMeter::Reset();
}

void FrameMeterHud::Toggle()
{
	SetVisible(!g_settings.frameMeterVisible);
}

bool FrameMeterHud::FontFailed()
{
	return g_fontTried && !GameFont::IsLoaded();
}
