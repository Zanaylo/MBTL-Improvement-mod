#include "Overlay/Window/UpdateNotifierWindow.h"

#include "Core/Settings.h"
#include "Core/info.h"
#include "Core/interfaces.h"
#include "Overlay/UiText.h"
#include "Web/UpdateCheck.h"
#include "Web/UpdateInstall.h"

#include <Windows.h>

#include <cfloat>
#include <cstdio>

#include <imgui.h>

UpdateNotifierWindow::UpdateNotifierWindow(const std::string& title, bool closable,
	ImGuiWindowFlags windowFlags)
	: IWindow(title, closable, windowFlags)
{
}

void UpdateNotifierWindow::BeforeDraw()
{
	ImGui::SetNextWindowSizeConstraints(ImVec2(360.0f, 0.0f), ImVec2(560.0f, FLT_MAX));
}

void UpdateNotifierWindow::DrawProgress(const UpdateInstall::Snapshot& snapshot)
{
	const Web::Job::Status& job = snapshot.job;

	if (job.state == Web::Job::State_Idle)
		return;

	if (job.state == Web::Job::State_Running && job.percent >= 0)
	{
		char overlay[64] = {};
		sprintf_s(overlay, "%d%%", job.percent);

		ImGui::ProgressBar(job.percent / 100.0f, ImVec2(-1.0f, 0.0f), overlay);
	}
	else if (job.state == Web::Job::State_Running)
	{
		ImGui::ProgressBar(-1.0f * static_cast<float>(ImGui::GetTime()), ImVec2(-1.0f, 0.0f), "");
	}

	if (job.source[0] != '\0')
		UiText::Muted("%s (from %s)", job.step, job.source);
	else if (job.step[0] != '\0')
		UiText::Muted("%s", job.step);

	if (job.state == Web::Job::State_Failed && job.error[0] != '\0')
		UiText::Warn("%s", job.error);
}

void UpdateNotifierWindow::DrawInstall()
{
	UpdateInstall::Snapshot snapshot = {};
	UpdateInstall::Read(snapshot);

	DrawProgress(snapshot);

	if (snapshot.busy)
	{
		if (ImGui::Button("Cancel"))
			UpdateInstall::Cancel();

		return;
	}

	if (snapshot.staged)
	{
		UiText::Good("Update ready. The game will close, install it and start again through Steam.");

		return;
	}

	if (ImGui::Button("Update now"))
		UpdateInstall::Start();

	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Downloads and checks the update, then closes the game to install it. "
			"Steam starts the game again when it is done.");
	}
}

void UpdateNotifierWindow::Draw()
{
	ImGui::Text("%s %s is available.", MBTL_IM_NAME, UpdateCheck::GetLatestVersion());
	ImGui::TextDisabled("You have %s.", MBTL_IM_VERSION);

	ImGui::Spacing();

	DrawInstall();

	if (ImGui::Button("Open the release page"))
	{
		ShellExecuteA(nullptr, "open", UpdateCheck::GetReleaseUrl(), nullptr, nullptr,
			SW_SHOWNORMAL);
	}

	ImGui::SameLine();

	if (ImGui::Button("Later"))
	{
		UpdateCheck::Dismiss();
		Close();
	}

	ImGui::SameLine();

	if (ImGui::Button("Stop checking"))
	{
		g_settings.checkForUpdates = false;
		Settings::SaveBool("Mod", "CheckForUpdates", false);
		UpdateCheck::Dismiss();
		Close();
	}

	ImGui::TextDisabled("To install an update by hand, close the game first.");
}
