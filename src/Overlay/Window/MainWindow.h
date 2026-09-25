#pragma once

#include "Overlay/Window/ConfigPanel.h"
#include "Overlay/Window/IWindow.h"
#include "Overlay/WindowContainer/WindowType.h"

class WindowContainer;

class MainWindow : public IWindow
{
public:
	MainWindow(const std::string& title, bool closable, const WindowContainer& windows);

protected:
	void BeforeDraw() override;
	void Draw() override;
	bool GrowsToFitContent() const override { return true; }

private:
	void DrawTrainingSection();
	void DrawHitboxControls();
	void DrawHudControls();
	void DrawCharacterControls();
	void DrawHitboxTypes();
	void DrawFrameStepControls();
	void DrawExtras();
	void DrawStageColourControls();
	void DrawFrameMeterControls();
	void DrawFrameMeterOptions();
	void DrawStagesSection();
	void DrawMusicSection();
	void DrawPaletteSection();
	void DrawPaletteChooser(int player);
	void DrawPaletteOptions();
	void DrawModsSection();
	void DrawPerformanceSection();
	void DrawConfigSection();

	bool DrawWindowButton(WindowType type, const char* open, const char* close) const;

	const WindowContainer& m_windows;
	ConfigPanel m_config;
};
