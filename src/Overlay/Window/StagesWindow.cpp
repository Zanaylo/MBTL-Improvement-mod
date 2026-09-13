#include "Overlay/Window/StagesWindow.h"

#include "Overlay/UiScale.h"

#include <cfloat>

StagesWindow::StagesWindow(const std::string& title, bool closable, ImGuiWindowFlags windowFlags)
	: IWindow(title, closable, windowFlags)
{
}

void StagesWindow::BeforeDraw()
{
	ImGui::SetNextWindowSize(Ui::Scaled(880.0f, 560.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSizeConstraints(Ui::Scaled(560.0f, 320.0f), ImVec2(FLT_MAX, FLT_MAX));
}

void StagesWindow::Draw()
{
	m_panel.Draw();
}
