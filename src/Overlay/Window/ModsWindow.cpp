#include "Overlay/Window/ModsWindow.h"

#include "Overlay/UiScale.h"

ModsWindow::ModsWindow(const std::string& title, bool closable, ImGuiWindowFlags windowFlags)
	: IWindow(title, closable, windowFlags)
{
}

void ModsWindow::BeforeDraw()
{
	ImGui::SetNextWindowSize(Ui::Scaled(640.0f, 460.0f), ImGuiCond_FirstUseEver);
}

void ModsWindow::Draw()
{
	m_panel.Draw();
}
