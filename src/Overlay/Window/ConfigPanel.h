#pragma once

#include "Core/Hotkeys.h"

class ConfigPanel
{
public:
	void Draw();
	void CancelCapture();

private:
	void DrawGeneralTab();
	void DrawOverlayOptions();
	void DrawStepOptions();
	void DrawRosterOptions();
	void DrawKeybindsTab();
	void DrawFunctionRow();
	void DrawBindRow(Hotkeys::Action action);
	void DrawConflicts();
	void Capture();
	void BeginCapture(int row);

	int m_capture = -1;
};
