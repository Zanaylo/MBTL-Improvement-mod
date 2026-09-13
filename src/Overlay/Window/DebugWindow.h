#pragma once

#include "Overlay/Debug/CoreDebugSections.h"
#include "Overlay/Window/IWindow.h"

class DebugWindow : public IWindow
{
public:
	DebugWindow(const std::string& title, bool closable, ImGuiWindowFlags windowFlags = 0);

protected:
	void BeforeDraw() override;
	void Draw() override;

private:
	void DrawSection(IDebugSection& section);

	HooksDebugSection m_hooks;
	DeviceDebugSection m_device;
	MemoryDebugSection m_memory;
};
