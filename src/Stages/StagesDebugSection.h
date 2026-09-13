#pragma once

#include "Overlay/Debug/DebugSection.h"

class StagesDebugSection : public IDebugSection
{
public:
	const char* Title() const override { return "Stages"; }
	void Draw() override;

private:
	void DrawLibrary();
	void DrawNumbers();
	void DrawOwn();
	void DrawOverlays();
};
