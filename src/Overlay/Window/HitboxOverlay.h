#pragma once

#include "Overlay/Window/IWindow.h"
#include "Training/Camera.h"
#include "Training/HitboxData.h"

class HitboxOverlay : public IWindow
{
public:
	enum BoxCategory
	{
		Category_Push,
		Category_Hurt,
		Category_StrikeInvulnerable,
		Category_FullyInvulnerable,
		Category_Hit,
		Category_Clash,
		Category_Other,
		Category_COUNT
	};

	struct CategorySettings
	{
		bool enabled;
		float fillAlpha;
		float outlineAlpha;
	};

	HitboxOverlay(const std::string& title, bool closable, ImGuiWindowFlags windowFlags = 0);

	bool IsInteractive() const override { return false; }

	CategorySettings& Category(int category) { return m_categories[category]; }

	static int CategoryCount();
	static const char* CategoryName(int category);
	static const char* CategorySummary(int category);
	static unsigned int CategoryColor(int category);

	static bool IsAvailable();
	static const char* StatusText();

protected:
	void BeforeDraw() override;
	void AfterDraw() override;
	void Draw() override;

private:
	static bool ShouldDraw();
	static int Classify(const HitboxData::Box& box, uint8_t invulnerability);

	void DrawObject(ImDrawList* drawList, const Camera::View& view, const HitboxData::Object& object);
	void DrawOrigin(ImDrawList* drawList, const Camera::View& view, const HitboxData::Object& object) const;
	void DrawBox(ImDrawList* drawList, const Camera::View& view, const HitboxData::Object& object,
		const HitboxData::Box& box, uint8_t invulnerability) const;

	CategorySettings m_categories[Category_COUNT];
	HitboxData::Object m_objects[HitboxData::kMaxObjects];
	HitboxData::Box m_boxes[HitboxData::kMaxBoxes];
};
