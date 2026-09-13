#include "Overlay/Window/DebugWindow.h"

#include "Core/utils.h"
#include "Overlay/UiScale.h"

#include <windows.h>

DebugWindow::DebugWindow(const std::string& title, bool closable, ImGuiWindowFlags windowFlags)
	: IWindow(title, closable, windowFlags)
{
}

void DebugWindow::BeforeDraw()
{
	ImGui::SetNextWindowSize(Ui::Scaled(620.0f, 520.0f), ImGuiCond_FirstUseEver);
}

void DebugWindow::Draw()
{
	if (ImGui::Button("Open the log folder"))
		ShellExecuteA(nullptr, "open", GetModRootPath("Logs").c_str(), nullptr, nullptr, SW_SHOWNORMAL);

	DrawSection(m_hooks);
	DrawSection(m_device);

	for (IDebugSection* section : DebugSections::All())
		DrawSection(*section);

	DrawSection(m_memory);
}

void DebugWindow::DrawSection(IDebugSection& section)
{
	if (!ImGui::CollapsingHeader(section.Title()))
		return;

	ImGui::PushID(&section);
	section.Draw();
	ImGui::PopID();
}
