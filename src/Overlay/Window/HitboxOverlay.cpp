#include "Overlay/Window/HitboxOverlay.h"

#include "Core/interfaces.h"
#include "D3D9/DeviceHooks.h"
#include "Game/GameOffsets.h"
#include "Training/FrameStepper.h"
#include "Training/GameState.h"
#include "Training/Invulnerability.h"

#include <algorithm>

namespace {

constexpr float kOriginArm = 8.0f;
constexpr float kLineWidth = 1.5f;
constexpr float kDefaultFill = 0.22f;
constexpr float kDefaultOutline = 0.9f;
constexpr float kSubpixels = GameOffsets::Camera::kSubpixels;
constexpr unsigned int kOriginColor = IM_COL32(255, 255, 0, 200);

struct CategoryInfo
{
	const char* name;
	unsigned int color;
	const char* summary;
};

const CategoryInfo kCategories[HitboxOverlay::Category_COUNT] = {
	{ "Pushbox", IM_COL32(255, 255, 255, 255), "Body collision. Two characters cannot walk through each other." },
	{ "Hurtbox", IM_COL32(60, 220, 60, 255),
		"Where the character can be hit. Throw invincibility is shown on the frame meter's status row, not here." },
	{ "Strike invulnerable", IM_COL32(90, 120, 255, 255),
		"A hurtbox on a frame strikes and projectiles pass through. The character can still be thrown." },
	{ "Fully invulnerable", IM_COL32(190, 110, 255, 255),
		"A hurtbox on a frame nothing connects: full invincibility, no hurtbox, knocked out, or linked to another "
		"object such as the partner maid." },
	{ "Hitbox", IM_COL32(255, 60, 60, 255), "The attack itself." },
	{ "Clash", IM_COL32(60, 220, 230, 255), "Box 11, drawn as the clash box by the community viewer. Not confirmed." },
	{ "Other", IM_COL32(230, 230, 60, 255), "Boxes with no shared meaning, mostly anchors." },
};

unsigned int WithAlpha(unsigned int color, float alpha)
{
	const unsigned int a = static_cast<unsigned int>(alpha * 255.0f) & 0xFF;
	return (color & 0x00FFFFFF) | (a << IM_COL32_A_SHIFT);
}

bool Interactive(const HitboxData::Box* boxes, int count)
{
	for (int i = 0; i < count; ++i)
	{
		if (boxes[i].kind == HitboxData::BoxKind_Attack || boxes[i].kind == HitboxData::BoxKind_Hurt)
			return true;
	}

	return false;
}

}

HitboxOverlay::HitboxOverlay(const std::string& title, bool closable, ImGuiWindowFlags windowFlags)
	: IWindow(title, closable, windowFlags)
{
	for (CategorySettings& settings : m_categories)
		settings = { true, kDefaultFill, kDefaultOutline };

	m_categories[Category_Push].fillAlpha = 0.0f;
	m_categories[Category_Other].enabled = false;
}

int HitboxOverlay::CategoryCount()
{
	return Category_COUNT;
}

const char* HitboxOverlay::CategoryName(int category)
{
	return kCategories[category].name;
}

const char* HitboxOverlay::CategorySummary(int category)
{
	return kCategories[category].summary;
}

unsigned int HitboxOverlay::CategoryColor(int category)
{
	return kCategories[category].color;
}

bool HitboxOverlay::IsAvailable()
{
	return HitboxData::IsAvailable() && Camera::IsAvailable();
}

const char* HitboxOverlay::StatusText()
{
	if (!HitboxData::IsAvailable())
		return "The hitbox viewer is not available: the character array was not found in this build.";

	if (!Camera::IsAvailable())
		return "The hitbox viewer is not available: the camera was not found in this build.";

	return "ready";
}

int HitboxOverlay::Classify(const HitboxData::Box& box, uint8_t invulnerability)
{
	const bool strike = (invulnerability & Invulnerability::Flag_Strike) != 0;
	const bool thrown = (invulnerability & Invulnerability::Flag_Throw) != 0;

	switch (box.kind)
	{
	case HitboxData::BoxKind_Push:
		return Category_Push;
	case HitboxData::BoxKind_Hurt:
		if (strike && thrown)
			return Category_FullyInvulnerable;
		return strike ? Category_StrikeInvulnerable : Category_Hurt;
	case HitboxData::BoxKind_Attack:
		return Category_Hit;
	case HitboxData::BoxKind_Clash:
		return Category_Clash;
	default:
		return Category_Other;
	}
}

bool HitboxOverlay::ShouldDraw()
{
	if (!IsAvailable() || !GameState::IsOnlineKnown() || GameState::IsOnline())
		return false;

	if (GameState::IsGamePaused() && !g_settings.drawWhilePaused)
		return false;

	return GameState::IsBattleRunning() || !FrameStepper::IsImplemented();
}

void HitboxOverlay::BeforeDraw()
{
	const ImGuiViewport* const viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
}

void HitboxOverlay::AfterDraw()
{
	ImGui::PopStyleVar();
}

void HitboxOverlay::Draw()
{
	if (!ShouldDraw())
		return;

	Camera::View view = {};

	if (!Camera::Read(view))
		return;

	ImDrawList* const drawList = ImGui::GetForegroundDrawList();
	const int count = HitboxData::Objects(m_objects, HitboxData::kMaxObjects);

	for (int i = 0; i < count; ++i)
		DrawObject(drawList, view, m_objects[i]);
}

void HitboxOverlay::DrawObject(ImDrawList* drawList, const Camera::View& view, const HitboxData::Object& object)
{
	const int count = HitboxData::Boxes(object, m_boxes, HitboxData::kMaxBoxes);

	if (count == 0)
		return;

	if (object.effect && !Interactive(m_boxes, count))
		return;

	if (g_settings.hitboxShowOrigin)
		DrawOrigin(drawList, view, object);

	const uint8_t invulnerability = Invulnerability::Read(object.address, object.effect);

	for (int i = 0; i < count; ++i)
		DrawBox(drawList, view, object, m_boxes[i], invulnerability);
}

void HitboxOverlay::DrawOrigin(ImDrawList* drawList, const Camera::View& view, const HitboxData::Object& object) const
{
	const ImVec2 display = ImGui::GetIO().DisplaySize;
	const float arm = kOriginArm * DeviceHooks::OverlayScale();

	float x = 0.0f;
	float y = 0.0f;
	Camera::ToScreen(view, static_cast<float>(object.x), static_cast<float>(object.y), display.x, display.y, x, y);

	drawList->AddLine(ImVec2(x - arm, y), ImVec2(x + arm, y), kOriginColor, kLineWidth);
	drawList->AddLine(ImVec2(x, y - arm), ImVec2(x, y + arm), kOriginColor, kLineWidth);
}

void HitboxOverlay::DrawBox(ImDrawList* drawList, const Camera::View& view, const HitboxData::Object& object,
	const HitboxData::Box& box, uint8_t invulnerability) const
{
	const int category = Classify(box, invulnerability);
	const CategorySettings& settings = m_categories[category];

	if (!settings.enabled)
		return;

	const float facing = object.facingLeft ? -1.0f : 1.0f;
	const ImVec2 display = ImGui::GetIO().DisplaySize;

	float left = 0.0f;
	float top = 0.0f;
	float right = 0.0f;
	float bottom = 0.0f;

	Camera::ToScreen(view, object.x + box.x1 * facing * kSubpixels, object.y + box.y1 * kSubpixels, display.x, display.y,
		left, top);
	Camera::ToScreen(view, object.x + box.x2 * facing * kSubpixels, object.y + box.y2 * kSubpixels, display.x, display.y,
		right, bottom);

	const ImVec2 topLeft((std::min)(left, right), (std::min)(top, bottom));
	const ImVec2 bottomRight((std::max)(left, right), (std::max)(top, bottom));
	const unsigned int color = CategoryColor(category);

	if (settings.fillAlpha > 0.0f)
		drawList->AddRectFilled(topLeft, bottomRight, WithAlpha(color, settings.fillAlpha));

	if (settings.outlineAlpha > 0.0f)
		drawList->AddRect(topLeft, bottomRight, WithAlpha(color, settings.outlineAlpha), 0.0f,
			kLineWidth * DeviceHooks::OverlayScale());
}
