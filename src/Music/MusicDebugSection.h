#pragma once

#include "Overlay/Debug/DebugSection.h"

class MusicDebugSection : public IDebugSection
{
public:
	const char* Title() const override { return "Music"; }
	void Draw() override;
};
