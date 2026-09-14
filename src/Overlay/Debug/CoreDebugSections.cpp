#include "Overlay/Debug/CoreDebugSections.h"

#include "Core/utils.h"
#include "D3D9/DeviceHooks.h"
#include "D3D9/PresentTuning.h"
#include "Game/Anchors.h"
#include "Hooks/HookManager.h"
#include "Overlay/UiScale.h"
#include "Overlay/UiText.h"
#include "Training/FrameStepper.h"

#include <imgui.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

constexpr int kBytesPerRow = 16;
constexpr int kDwordsPerRow = 4;
constexpr int kMostRows = 64;
constexpr ImGuiTableFlags kTableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
	ImGuiTableFlags_SizingStretchProp;

char Printable(uint8_t byte)
{
	return byte >= 0x20 && byte < 0x7f ? static_cast<char>(byte) : '.';
}

void DrawByteRow(uintptr_t address)
{
	uint8_t bytes[kBytesPerRow] = {};

	if (!TryReadMemory(bytes, reinterpret_cast<const void*>(address), sizeof(bytes)))
	{
		ImGui::TextDisabled("%08X  unreadable", static_cast<unsigned>(address));
		return;
	}

	char hex[kBytesPerRow * 3 + 1] = {};
	char text[kBytesPerRow + 1] = {};

	for (int i = 0; i < kBytesPerRow; ++i)
	{
		sprintf_s(hex + i * 3, sizeof(hex) - i * 3, "%02X ", bytes[i]);
		text[i] = Printable(bytes[i]);
	}

	ImGui::Text("%08X  %s %s", static_cast<unsigned>(address), hex, text);
}

void DrawDwordRow(uintptr_t address)
{
	uint32_t values[kDwordsPerRow] = {};

	if (!TryReadMemory(values, reinterpret_cast<const void*>(address), sizeof(values)))
	{
		ImGui::TextDisabled("%08X  unreadable", static_cast<unsigned>(address));
		return;
	}

	ImGui::Text("%08X  %08X %08X %08X %08X", static_cast<unsigned>(address), values[0], values[1], values[2],
		values[3]);
}

}

void HooksDebugSection::Draw()
{
	std::vector<Anchors::Anchor> anchors;
	Anchors::Snapshot(anchors);

	ImGui::SeparatorText("Game addresses found");

	if (anchors.empty())
		UiText::Muted("No addresses found yet.");

	if (!anchors.empty() && ImGui::BeginTable("##anchors", 3, kTableFlags))
	{
		ImGui::TableSetupColumn("Name");
		ImGui::TableSetupColumn("Where");
		ImGui::TableSetupColumn("Note");
		ImGui::TableHeadersRow();

		for (const Anchors::Anchor& anchor : anchors)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(anchor.name.c_str());
			ImGui::TableNextColumn();

			if (anchor.address == 0)
				UiText::Warn("not found");
			else
				ImGui::TextUnformatted(DescribeAddress(anchor.address).c_str());

			ImGui::TableNextColumn();
			ImGui::TextUnformatted(anchor.note.c_str());
		}

		ImGui::EndTable();
	}

	std::vector<HookManager::Hook> hooks;
	HookManager::Snapshot(hooks);

	ImGui::SeparatorText("Hooks");

	if (!ImGui::BeginTable("##hooks", 3, kTableFlags))
		return;

	ImGui::TableSetupColumn("Hook");
	ImGui::TableSetupColumn("Target");
	ImGui::TableSetupColumn("State");
	ImGui::TableHeadersRow();

	for (const HookManager::Hook& hook : hooks)
	{
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(hook.label.c_str());
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(hook.where.c_str());
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(hook.enabled ? "on" : "not on yet");
	}

	ImGui::EndTable();
}

void DeviceDebugSection::Draw()
{
	const D3DPRESENT_PARAMETERS& present = DeviceHooks::GetPresentParameters();

	if (!DeviceHooks::IsInstalled())
		UiText::Warn("No Direct3D device hooked yet.");

	ImGui::Text("Back buffer %ux%u, %s, %u Hz", present.BackBufferWidth, present.BackBufferHeight,
		present.Windowed ? "windowed" : "fullscreen", present.FullScreen_RefreshRateInHz);
	ImGui::Text("Presents %lu, overlay %.1f FPS, overlay scale %.2f", DeviceHooks::PresentCount(),
		ImGui::GetIO().Framerate, DeviceHooks::OverlayScale());
	ImGui::Text("Display tuning: %s", PresentTuning::GetLastDecision());
	ImGui::Text("ImGui %s", IMGUI_VERSION);

	ImGui::SeparatorText("Battle tick");
	ImGui::Text("Frame stepper: %s", FrameStepper::StatusText());
	ImGui::Text("Tick calls %llu, suppressed %llu", FrameStepper::CallCount(), FrameStepper::SuppressedCount());
}

void MemoryDebugSection::Draw()
{
	Ui::SetItemWidth(110.0f);
	ImGui::InputText("Address", m_address, sizeof(m_address), ImGuiInputTextFlags_CharsHexadecimal);

	ImGui::SameLine();

	if (ImGui::SmallButton("MBTL.exe"))
		sprintf_s(m_address, "%08X", static_cast<unsigned>(GetGameBaseAddress()));

	ImGui::SameLine();

	if (ImGui::SmallButton("Follow"))
	{
		uint32_t pointer = 0;

		if (TryRead(static_cast<uintptr_t>(strtoul(m_address, nullptr, 16)), pointer))
			sprintf_s(m_address, "%08X", pointer);
	}

	Ui::SetItemWidth(110.0f);
	ImGui::SliderInt("Rows", &m_rows, 1, kMostRows);

	ImGui::SameLine();
	ImGui::Checkbox("4 byte values", &m_dwords);

	DrawRows();
}

void MemoryDebugSection::DrawRows()
{
	const uintptr_t start = static_cast<uintptr_t>(strtoul(m_address, nullptr, 16));

	for (int row = 0; row < m_rows; ++row)
	{
		const uintptr_t address = start + static_cast<uintptr_t>(row * kBytesPerRow);

		if (m_dwords)
			DrawDwordRow(address);
		else
			DrawByteRow(address);
	}
}
