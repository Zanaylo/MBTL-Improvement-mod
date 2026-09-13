#pragma once

#include "Overlay/Debug/DebugSection.h"
#include "Training/HitboxData.h"

class TrainingDebugSection : public IDebugSection
{
public:
	const char* Title() const override { return "Battle, camera and hitbox data"; }
	void Draw() override;

private:
	void DrawState();
	void DrawCamera();
	void DrawObjects();

	HitboxData::Object m_objects[HitboxData::kMaxObjects];
	HitboxData::Box m_boxes[HitboxData::kMaxBoxes];
};
