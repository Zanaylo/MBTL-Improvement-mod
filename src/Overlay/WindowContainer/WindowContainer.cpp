#include "Overlay/WindowContainer/WindowContainer.h"

#include "Core/info.h"
#include "Overlay/Window/DebugWindow.h"
#include "Overlay/Window/FrameMeterLegendWindow.h"
#include "Overlay/Window/HitboxOverlay.h"
#include "Overlay/Window/MainWindow.h"
#include "Overlay/Window/ModsWindow.h"
#include "Overlay/Window/MusicWindow.h"
#include "Overlay/Window/PaletteWindow.h"
#include "Overlay/Window/PerformanceWindow.h"
#include "Overlay/Window/StagesWindow.h"

namespace {

constexpr ImGuiWindowFlags kOverlayFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
	ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs |
	ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
	ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBackground;

}

WindowContainer::WindowContainer()
{
	m_windows[WindowType_Main] = std::make_unique<MainWindow>(MBTL_IM_NAME " " MBTL_IM_VERSION, true, *this);
	m_windows[WindowType_HitboxOverlay] = std::make_unique<HitboxOverlay>("##hitboxoverlay", false, kOverlayFlags);
	m_windows[WindowType_Mods] = std::make_unique<ModsWindow>("Mods", true);
	m_windows[WindowType_Stages] = std::make_unique<StagesWindow>("Stages", true);
	m_windows[WindowType_Music] = std::make_unique<MusicWindow>("Music", true);
	m_windows[WindowType_Performance] = std::make_unique<PerformanceWindow>("Performance", true);
	m_windows[WindowType_Debug] = std::make_unique<DebugWindow>("Debug", true);
	m_windows[WindowType_FrameMeterLegend] = std::make_unique<FrameMeterLegendWindow>("Frame meter information", true);
	m_windows[WindowType_Palette] = std::make_unique<PaletteWindow>("Palette editor", true);
}

void WindowContainer::UpdateAll()
{
	for (const std::unique_ptr<IWindow>& window : m_windows)
		window->Update();
}

bool WindowContainer::AnyWindowOpen() const
{
	for (const std::unique_ptr<IWindow>& window : m_windows)
	{
		if (window->IsOpen())
			return true;
	}

	return false;
}

bool WindowContainer::AnyInteractiveWindowOpen() const
{
	for (const std::unique_ptr<IWindow>& window : m_windows)
	{
		if (window->IsOpen() && window->IsInteractive())
			return true;
	}

	return false;
}

IWindow* WindowContainer::GetWindow(WindowType type) const
{
	if (type < 0 || type >= WindowType_COUNT)
		return nullptr;

	return m_windows[type].get();
}

void WindowContainer::Toggle(WindowType type) const
{
	IWindow* const window = GetWindow(type);

	if (window != nullptr)
		window->Toggle();
}
