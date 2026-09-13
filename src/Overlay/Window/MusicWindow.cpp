#include "Overlay/Window/MusicWindow.h"

#include "Overlay/UiScale.h"

MusicWindow::MusicWindow(const std::string& title, bool closable, ImGuiWindowFlags windowFlags)
	: IWindow(title, closable, windowFlags)
{
}

void MusicWindow::BeforeDraw()
{
	ImGui::SetNextWindowSize(Ui::Scaled(780.0f, 560.0f), ImGuiCond_FirstUseEver);
}

void MusicWindow::Draw()
{
	m_panel.Draw();
}
