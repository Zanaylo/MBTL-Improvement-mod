#include "Overlay/Window/PerformanceWindow.h"

#include "Core/ProcessTuning.h"
#include "Core/Profiler.h"
#include "Core/Settings.h"
#include "Core/interfaces.h"
#include "Core/utils.h"
#include "D3D9/DeviceHooks.h"
#include "D3D9/PresentTuning.h"
#include "Game/GameOffsets.h"
#include "Overlay/UiScale.h"
#include "Overlay/UiText.h"
#include "Overlay/Window/GraphicsPanel.h"
#include "Performance/EngineQuality.h"
#include "Performance/Improvements.h"
#include "Performance/InputLagMeter.h"
#include "Performance/PotatoMode.h"
#include "Performance/PumpWait.h"
#include "Performance/StageColor.h"

#include <cstdio>
#include <vector>

namespace {

constexpr float kHistogramHeight = 110.0f;
constexpr float kDefaultWidth = 780.0f;
constexpr float kDefaultHeight = 600.0f;
constexpr DWORD kRefreshIntervalMs = 250;
constexpr double kFrameMs = 1000.0 / 60.0;
constexpr const char* kVideo = "Video";
constexpr const char* kGraphics = "Graphics";
constexpr const char* kAutomatic = "Automatic (same as the desktop)";

const ImVec4 kGoodColour(0.45f, 0.80f, 0.50f, 1.0f);
const ImVec4 kWarnColour(0.95f, 0.55f, 0.45f, 1.0f);
const ImVec4 kSameColour(0.70f, 0.70f, 0.70f, 1.0f);
constexpr ImGuiTableFlags kTableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
	ImGuiTableFlags_SizingStretchProp;

struct Option
{
	bool* value;
	const char* key;
	const char* label;
	const char* summary;
	const char* help;
};

const Option kOptions[] = {
	{
		&g_settings.timerResolution,
		"TimerResolution",
		"Keep the Windows timer at 1 ms",
		"Fixes the slowdown Alt+Tab leaves behind. No downside.",
		"After Alt+Tab, Windows can make each short pause in the game last 15.6 ms instead of 1 ms. This keeps "
		"the 1 ms timer on, even after the window loses focus and comes back.",
	},
	{
		&g_settings.powerThrottlingOptOut,
		"PowerThrottlingOptOut",
		"Stop Windows slowing the game in the background",
		"Goes with the option above. Uses a little more power while the game is in the background.",
		"Windows moves background programs to slower CPU cores and limits their timer. This keeps the game on the "
		"fast cores and keeps the 1 ms timer working.",
	},
	{
		&g_settings.pumpWait,
		"PumpWait",
		"Use a precise timer for the frame pause",
		"The short pause after each frame lasts 1 ms, not 15 ms. No CPU cost.",
		"The game pauses for 1 ms after every frame. After Alt+Tab, Windows can stretch that pause to 15.6 ms, "
		"which drops a frame.\n\nThis moves the game's short pauses to a precise timer that Windows does not "
		"stretch. No CPU cost, and no game code is changed.",
	},
};

using UiText::Help;
using UiText::Warn;

void Muted(const char* text)
{
	UiText::Muted("%s", text);
}

bool VsyncIsOn(const D3DPRESENT_PARAMETERS& present)
{
	return present.PresentationInterval != D3DPRESENT_INTERVAL_IMMEDIATE;
}

bool ShouldRefresh(DWORD& lastTick)
{
	const DWORD now = GetTickCount();

	if (lastTick != 0 && now - lastTick < kRefreshIntervalMs)
		return false;

	lastTick = now;
	return true;
}

void SaveVideo(const char* key, bool value)
{
	Settings::SaveBool(kVideo, key, value);
}

void DeltaText(double now, double before)
{
	if (before <= 0.0)
	{
		ImGui::TextDisabled("-");
		return;
	}

	const double delta = now - before;
	const ImVec4 colour = delta == 0.0 ? kSameColour : (delta < 0.0 ? kGoodColour : kWarnColour);

	ImGui::TextColored(colour, "%+.2f ms (%+.0f%%)", delta, delta / before * 100.0);
}

void BaselineCell(bool hasBaseline, const char* format, double value)
{
	ImGui::TableNextColumn();

	if (!hasBaseline)
	{
		ImGui::TextDisabled("-");
		return;
	}

	ImGui::Text(format, value);
}

void StatRow(const char* name, double now, double before, bool hasBaseline)
{
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::TextUnformatted(name);

	BaselineCell(hasBaseline, "%.2f ms", before);

	ImGui::TableNextColumn();
	ImGui::Text("%.2f ms", now);

	ImGui::TableNextColumn();

	if (hasBaseline)
		DeltaText(now, before);
	else
		ImGui::TextDisabled("-");
}

void OnTargetRow(const Profiler::Stats& now, const Profiler::Stats& before, bool hasBaseline)
{
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::TextUnformatted("Within 0.5 ms of 16.67 ms");

	BaselineCell(hasBaseline, "%.0f%%", before.onTargetPercent);

	ImGui::TableNextColumn();
	ImGui::Text("%.0f%%", now.onTargetPercent);

	ImGui::TableNextColumn();

	if (!hasBaseline)
	{
		ImGui::TextDisabled("-");
		return;
	}

	const double delta = now.onTargetPercent - before.onTargetPercent;
	ImGui::TextColored(delta >= 0.0 ? kGoodColour : kWarnColour, "%+.0f%%", delta);
}

void SlowFramesRow(const Profiler::Stats& now, const Profiler::Stats& before, bool hasBaseline)
{
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::TextUnformatted("Frames over 20 ms");
	ImGui::TableNextColumn();

	if (hasBaseline)
		ImGui::Text("%d of %d", before.slowFrames, before.samples);
	else
		ImGui::TextDisabled("-");

	ImGui::TableNextColumn();
	ImGui::Text("%d of %d", now.slowFrames, now.samples);
	ImGui::TableNextColumn();
	ImGui::TextDisabled("-");
}

void DrawIntervalTable(const char* title, const Profiler::Stats& now, const Profiler::Stats& before, bool hasBaseline,
	bool withSpread)
{
	ImGui::SeparatorText(title);

	if (!ImGui::BeginTable(title, 4, kTableFlags))
		return;

	ImGui::TableSetupColumn("What");
	ImGui::TableSetupColumn("Baseline");
	ImGui::TableSetupColumn("Now");
	ImGui::TableSetupColumn("Change");
	ImGui::TableHeadersRow();

	StatRow("Median", now.medianMs, before.medianMs, hasBaseline);

	if (withSpread)
	{
		StatRow("Spread", now.stddevMs, before.stddevMs, hasBaseline);
		StatRow("Average miss", now.madMs, before.madMs, hasBaseline);
	}

	StatRow("Slowest 1%", now.p99Ms, before.p99Ms, hasBaseline);
	StatRow("Worst", now.maxMs, before.maxMs, hasBaseline);
	StatRow("Average", now.averageMs, before.averageMs, hasBaseline);

	if (withSpread)
		OnTargetRow(now, before, hasBaseline);

	SlowFramesRow(now, before, hasBaseline);

	ImGui::EndTable();
}

void DrawHistogram(int (*bucket)(int), int count, const char* id)
{
	std::vector<float> values(static_cast<size_t>(count), 0.0f);
	float peak = 1.0f;

	for (int i = 0; i < count; ++i)
	{
		values[i] = static_cast<float>(bucket(i));
		peak = values[i] > peak ? values[i] : peak;
	}

	ImGui::PlotHistogram(id, values.data(), count, 0, nullptr, 0.0f, peak,
		ImVec2(-1.0f, Ui::Scaled(kHistogramHeight)));
}

void DrawFineHistogram()
{
	DrawHistogram(&Profiler::GetFineHistogramBucket, Profiler::kFineBuckets, "##fineinterval");

	const double base = Profiler::GetFineHistogramBaseMs();
	UiText::Muted("The same frames in 0.25 ms steps, from %.2f to %.2f ms. One spike in the middle means smooth. "
		"Two spikes, one on each side, means judder (the median does not show it).", base,
		base + Profiler::kFineBuckets * Profiler::kFineBucketMs);

	double firstMs = 0.0;
	double secondMs = 0.0;
	double separationMs = 0.0;

	if (!Profiler::FindModes(firstMs, secondMs, separationMs))
		return;

	Warn("Two groups: %.2f ms and %.2f ms (%.2f ms apart). Frames land on both sides of the target, not on it. "
		"This is judder.", firstMs, secondMs, separationMs);
}

void DrawSections(bool hasBaseline)
{
	ImGui::SeparatorText("Time spent per frame");
	Muted("Time the mod spends on each task, in ms, averaged over recent frames. oPresent and oBattleTick are the "
		"game's own work, for comparison.");

	if (!ImGui::BeginTable("##sections", 4, kTableFlags))
		return;

	ImGui::TableSetupColumn("Section");
	ImGui::TableSetupColumn("Baseline");
	ImGui::TableSetupColumn("Now");
	ImGui::TableSetupColumn("Change");
	ImGui::TableHeadersRow();

	for (int i = 0; i < Profiler::Section_COUNT; ++i)
	{
		const Profiler::Section section = static_cast<Profiler::Section>(i);
		const double now = Profiler::GetSectionMs(section);
		const double before = Profiler::GetBaselineSectionMs(section);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(Profiler::GetSectionName(section));

		BaselineCell(hasBaseline, "%.3f", before);

		ImGui::TableNextColumn();
		ImGui::Text("%.3f", now);

		ImGui::TableNextColumn();

		if (hasBaseline && before > 0.0)
			DeltaText(now, before);
		else
			ImGui::TextDisabled("-");
	}

	ImGui::EndTable();
}

void BuildBaselineLabel(char* out, size_t size)
{
	const D3DPRESENT_PARAMETERS& present = DeviceHooks::GetPresentParameters();

	sprintf_s(out, size, "%s %u Hz, %u buffer(s), vsync %s, timer %d, throttling %d, pump %d, potato %d",
		present.Windowed ? "windowed" : "fullscreen", present.FullScreen_RefreshRateInHz, present.BackBufferCount,
		VsyncIsOn(present) ? "on" : "off", g_settings.timerResolution ? 1 : 0,
		g_settings.powerThrottlingOptOut ? 1 : 0, g_settings.pumpWait ? 1 : 0, PotatoMode::GetLevel());
}

void ApplyPreset(bool timer, bool throttling, bool pump, bool tuning)
{
	g_settings.timerResolution = timer;
	g_settings.powerThrottlingOptOut = throttling;
	g_settings.pumpWait = pump;
	g_settings.displayTuning = tuning;
	g_settings.extraBackBuffer = false;
	g_settings.fullscreenRefreshHz = 0;

	SaveVideo("TimerResolution", timer);
	SaveVideo("PowerThrottlingOptOut", throttling);
	SaveVideo("PumpWait", pump);
	SaveVideo("DisplayTuning", tuning);
	SaveVideo("ExtraBackBuffer", false);
	Settings::SaveInt(kVideo, "FullscreenRefreshHz", 0);
}

void CollectRefreshRates(std::vector<UINT>& out)
{
	out.clear();

	IDirect3DDevice9* const device = DeviceHooks::Device();
	IDirect3D9* d3d9 = nullptr;

	if (device == nullptr || FAILED(device->GetDirect3D(&d3d9)) || d3d9 == nullptr)
		return;

	D3DDEVICE_CREATION_PARAMETERS creation = {};
	const UINT adapter = SUCCEEDED(device->GetCreationParameters(&creation)) ? creation.AdapterOrdinal
		: D3DADAPTER_DEFAULT;

	const D3DPRESENT_PARAMETERS& present = DeviceHooks::GetPresentParameters();
	const UINT modeCount = d3d9->GetAdapterModeCount(adapter, present.BackBufferFormat);

	for (UINT i = 0; i < modeCount; ++i)
	{
		D3DDISPLAYMODE mode = {};

		if (FAILED(d3d9->EnumAdapterModes(adapter, present.BackBufferFormat, i, &mode)))
			continue;

		if (mode.Width != present.BackBufferWidth || mode.Height != present.BackBufferHeight || mode.RefreshRate == 0)
			continue;

		bool seen = false;

		for (const UINT rate : out)
			seen = seen || rate == mode.RefreshRate;

		if (!seen)
			out.push_back(mode.RefreshRate);
	}

	d3d9->Release();
}

bool DrawRefreshCombo()
{
	static std::vector<UINT> rates;
	static DWORD ratesTick = 0;

	if (ShouldRefresh(ratesTick))
		CollectRefreshRates(rates);

	char preview[64] = {};

	if (g_settings.fullscreenRefreshHz == 0)
		sprintf_s(preview, "%s", kAutomatic);
	else
		sprintf_s(preview, "%d Hz", g_settings.fullscreenRefreshHz);

	Ui::SetItemWidth(320.0f);

	if (!ImGui::BeginCombo("Fullscreen refresh rate", preview))
		return false;

	bool changed = false;

	if (ImGui::Selectable(kAutomatic, g_settings.fullscreenRefreshHz == 0))
	{
		g_settings.fullscreenRefreshHz = 0;
		changed = true;
	}

	for (const UINT rate : rates)
	{
		char label[32] = {};
		sprintf_s(label, "%u Hz%s", rate, rate % 60 == 0 ? " (divides by 60)" : "");

		if (!ImGui::Selectable(label, g_settings.fullscreenRefreshHz == static_cast<int>(rate)))
			continue;

		g_settings.fullscreenRefreshHz = static_cast<int>(rate);
		changed = true;
	}

	ImGui::EndCombo();

	if (changed)
		Settings::SaveInt(kVideo, "FullscreenRefreshHz", g_settings.fullscreenRefreshHz);

	return changed;
}

void DrawSceneTargetNote()
{
	UiText::Muted("The game draws the characters and the stage at %dx%d, then stretches them. They get no extra "
		"detail above that size.", GameOffsets::Render::kSceneWidth, GameOffsets::Render::kSceneHeight);
}

}

PerformanceWindow::PerformanceWindow(const std::string& title, bool closable, ImGuiWindowFlags windowFlags)
	: IWindow(title, closable, windowFlags)
{
}

void PerformanceWindow::BeforeDraw()
{
	const ImGuiViewport* const viewport = ImGui::GetMainViewport();

	const float width = Ui::Scaled(kDefaultWidth);
	const float height = Ui::Scaled(kDefaultHeight);

	ImGui::SetNextWindowSize(ImVec2(width < viewport->WorkSize.x ? width : viewport->WorkSize.x,
		height < viewport->WorkSize.y ? height : viewport->WorkSize.y), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSizeConstraints(Ui::Scaled(440.0f, 280.0f), viewport->WorkSize);
}

void PerformanceWindow::Draw()
{
	if (!ImGui::BeginTabBar("##performance"))
		return;

	if (ImGui::BeginTabItem("Performance"))
	{
		DrawPerformanceTab();
		ImGui::EndTabItem();
	}

	if (ImGui::BeginTabItem("POTATO MODE"))
	{
		DrawPotatoTab();
		ImGui::EndTabItem();
	}

	if (!PotatoMode::IsActive() && ImGui::BeginTabItem("Improvements"))
	{
		DrawImprovementsTab();
		ImGui::EndTabItem();
	}

	if (ImGui::BeginTabItem("Shaders"))
	{
		GraphicsPanel::DrawShadersTab();
		ImGui::EndTabItem();
	}

	if (ImGui::BeginTabItem("Metrics"))
	{
		DrawMetricsTab();
		ImGui::EndTabItem();
	}

	ImGui::EndTabBar();
}

void PerformanceWindow::DrawWhatIsHappening()
{
	const D3DPRESENT_PARAMETERS& present = DeviceHooks::GetPresentParameters();

	if (present.BackBufferWidth == 0)
	{
		Warn("No Direct3D device found. Another program is wrapping Direct3D, so the display settings below do "
			"nothing.");
		return;
	}

	ImGui::Text("%ux%u %s, %s, %u back buffer(s)", present.BackBufferWidth, present.BackBufferHeight,
		present.Windowed ? "windowed" : "exclusive fullscreen", VsyncIsOn(present) ? "vsync on" : "vsync off",
		present.BackBufferCount);

	if (!present.Windowed)
	{
		ImGui::SameLine();
		ImGui::Text("at %u Hz", present.FullScreen_RefreshRateInHz);
	}

	static DWORD stateTick = 0;
	static double fps = 0.0;

	if (ShouldRefresh(stateTick))
		fps = Profiler::IsEnabled() ? Profiler::GetPresentedFps() : ImGui::GetIO().Framerate;

	ImGui::Text("Running at %.1f frames per second", fps);

	if (g_settings.pumpWait)
		ImGui::Text("Precise frame pause: %s", PumpWait::IsActive() ? "active" : "not active");

	if (present.Windowed)
		Muted("In windowed mode Windows controls how frames are shown, so the display settings below do nothing.");

	if (!present.Windowed && VsyncIsOn(present) && present.FullScreen_RefreshRateInHz % 60 != 0)
	{
		Warn("%u Hz cannot show 60 fps evenly, so with vsync on the game judders. Pick a refresh rate below that "
			"divides by 60, or turn off the game's vsync.",
			present.FullScreen_RefreshRateInHz);
	}
}

bool PerformanceWindow::DrawOptions()
{
	bool changed = false;

	for (const Option& option : kOptions)
	{
		ImGui::PushID(option.key);

		if (ImGui::Checkbox(option.label, option.value))
		{
			SaveVideo(option.key, *option.value);
			changed = true;
		}

		Help(option.help);

		ImGui::Indent();
		Muted(option.summary);
		ImGui::Unindent();

		ImGui::PopID();
	}

	return changed;
}

bool PerformanceWindow::DrawDisplayGroup()
{
	if (!ImGui::CollapsingHeader("Display"))
		return false;

	const D3DPRESENT_PARAMETERS& present = DeviceHooks::GetPresentParameters();
	const bool windowed = present.Windowed != FALSE;
	const bool vsync = VsyncIsOn(present);
	bool changed = false;

	ImGui::BeginDisabled(windowed);

	if (ImGui::Checkbox("Let the mod pick the display settings", &g_settings.displayTuning))
	{
		SaveVideo("DisplayTuning", g_settings.displayTuning);
		changed = true;
	}

	Help("When off, the refresh rate and the number of back buffers stay as the game sets them.");

	ImGui::BeginDisabled(!g_settings.displayTuning);

	changed = DrawRefreshCombo() || changed;

	Help("With the game's vsync off, the refresh rate only changes how fast a frame reaches the screen. Automatic "
		"is the fastest and the quickest to Alt+Tab out of.\n\nWith vsync on, use a rate that divides by 60, or "
		"the game judders. Automatic then picks the highest rate that divides by 60.\n\nRestart the game to "
		"apply.");

	ImGui::BeginDisabled(!vsync);

	if (ImGui::Checkbox("Extra back buffer", &g_settings.extraBackBuffer))
	{
		SaveVideo("ExtraBackBuffer", g_settings.extraBackBuffer);
		changed = true;
	}

	Help("Lets a late frame wait instead of skipping a whole refresh. Adds up to one frame of input lag.\n\nOnly "
		"useful with vsync on. With vsync off it only adds delay.");

	ImGui::EndDisabled();

	if (!vsync)
	{
		ImGui::Indent();
		Muted("Greyed out: the game's vsync is off, so an extra buffer would only add lag.");
		ImGui::Unindent();
	}

	ImGui::EndDisabled();
	ImGui::EndDisabled();

	if (windowed)
		Muted("Greyed out: the game is windowed, and Windows controls the refresh rate and buffers there.");

	return changed;
}

bool PerformanceWindow::DrawAdvanced()
{
	if (!ImGui::CollapsingHeader("Advanced"))
		return false;

	ImGui::BeginDisabled(!g_settings.pumpWait);

	const bool changed = ImGui::Checkbox("End the frame pause as soon as input arrives", &g_settings.pumpWaitAllInput);

	ImGui::EndDisabled();

	if (changed)
		SaveVideo("PumpWaitAllInput", g_settings.pumpWaitAllInput);

	Help("The game reads input at the start of each frame. Ending the pause as soon as a key or mouse input "
		"arrives lets the game read it up to 1 ms sooner. The game still runs at 60 fps. Uses more CPU the more "
		"you move the mouse. Needs \"Use a precise timer for the frame pause\" turned on.");

	return changed;
}

bool PerformanceWindow::DrawPresets()
{
	ImGui::SeparatorText("Presets");

	bool changed = false;

	if (ImGui::Button("Recommended"))
	{
		ApplyPreset(true, true, true, true);
		changed = true;
	}

	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Turns on every option above, keeps the desktop's display mode and uses one back buffer. "
			"None of it uses extra CPU.");
	}

	ImGui::SameLine();

	if (ImGui::Button("Game default"))
	{
		ApplyPreset(false, false, false, false);
		changed = true;
	}

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Turns everything off. Measure this first, so you have something to compare against.");

	return changed;
}

void PerformanceWindow::DrawPerformanceTab()
{
	ImGui::Spacing();
	ImGui::SeparatorText("Current state");
	DrawWhatIsHappening();

	ImGui::Spacing();
	ImGui::SeparatorText("Options");

	bool changed = DrawOptions();

	ImGui::Spacing();
	changed = DrawDisplayGroup() || changed;

	ImGui::Spacing();
	changed = DrawAdvanced() || changed;

	ImGui::Spacing();
	changed = DrawPresets() || changed;

	if (!changed)
		return;

	ProcessTuning::Apply();
	PumpWait::Apply();
	Profiler::Reset();
}

void PerformanceWindow::DrawPotatoTab()
{
	ImGui::Spacing();

	Muted("These levels only change how the game is drawn. They do not affect gameplay, your opponent cannot see "
		"them, and the stage is still drawn.");

	ImGui::Spacing();

	bool changed = false;
	const int level = PotatoMode::GetLevel();

	for (int candidate = 0; candidate < PotatoMode::Level_COUNT; ++candidate)
	{
		if (ImGui::RadioButton(PotatoMode::GetLevelName(candidate), level == candidate) && candidate != level)
		{
			PotatoMode::Apply(candidate);
			changed = true;
		}

		ImGui::Indent();
		UiText::Muted("%s", PotatoMode::Describe(candidate));

		if (candidate == PotatoMode::Level_Potato && level == PotatoMode::Level_Potato)
			changed = DrawPotatoHeight() || changed;

		ImGui::Unindent();
	}

	ImGui::Spacing();

	Muted("A new drawing size needs a game restart, or a change to any video option in the game's menu. Everything "
		"else applies at once. In exclusive fullscreen the size is rounded up to the nearest mode your monitor "
		"supports.");

	ImGui::Spacing();
	changed = DrawEmptyStage() || changed;

	ImGui::Spacing();
	ImGui::SeparatorText("Active now");
	DrawPotatoState();

	if (!changed)
		return;

	Profiler::Reset();
}

bool PerformanceWindow::DrawEmptyStage()
{
	const bool available = StageColor::IsAvailable();

	ImGui::BeginDisabled(!available);
	const bool changed = ImGui::Checkbox("Black background (hide the stage)", &g_settings.simpleStage);
	ImGui::EndDisabled();

	if (changed)
	{
		StageColor::SetColor(0x000000u);
		StageColor::SetEnabled(g_settings.simpleStage);
		Settings::SaveBool(kGraphics, "SimpleStage", g_settings.simpleStage);
	}

	ImGui::Indent();

	if (available)
	{
		Muted("Not part of any level above. Use it only if your PC still cannot hold 60 fps on Potato.");
	}
	else
	{
		UiText::Muted("Unavailable: %s", StageColor::StatusText());
	}

	ImGui::Unindent();
	return changed;
}

void PerformanceWindow::DrawImprovementsTab()
{
	ImGui::Spacing();

	bool changed = GraphicsPanel::DrawEverythingOff();

	ImGui::SameLine();
	Muted("The opposite of POTATO MODE: the game is drawn bigger than your window, then shrunk to fit.");

	ImGui::Spacing();
	ImGui::SeparatorText("Drawing size");

	const int level = Improvements::GetLevel();

	for (int candidate = 0; candidate < Improvements::Level_COUNT; ++candidate)
	{
		if (candidate > 0)
			ImGui::SameLine();

		if (!ImGui::RadioButton(Improvements::GetLevelName(candidate), level == candidate) || candidate == level)
			continue;

		Improvements::Apply(candidate);
		changed = true;
	}

	Help("Draws the HUD, the menus and this overlay at a higher resolution and shrinks them to fit, so they look "
		"sharper. Windowed mode only. Restart the game to apply. Costs GPU time: 4K has nine times the pixels of "
		"720p.");

	UiText::Muted("%s", Improvements::Describe(Improvements::GetLevel()));
	DrawSceneTargetNote();

	ImGui::Spacing();
	ImGui::SeparatorText("Active now");
	DrawPotatoState();

	if (!changed)
		return;

	Profiler::Reset();
}

bool PerformanceWindow::DrawPotatoHeight()
{
	ImGui::Spacing();

	const int current = PotatoMode::GetHeight();
	bool changed = false;

	for (const int height : PotatoMode::kHeights)
	{
		int width = 0;
		int rounded = 0;
		PotatoMode::SizeForHeight(height, width, rounded);

		char label[48] = {};
		snprintf(label, sizeof(label), "%dp (%dx%d)", height, width, rounded);

		if (ImGui::RadioButton(label, current == height) && current != height)
		{
			PotatoMode::SetHeight(height);
			changed = true;
		}

		ImGui::SameLine();
	}

	ImGui::NewLine();
	return changed;
}

void PerformanceWindow::DrawPotatoState()
{
	const D3DPRESENT_PARAMETERS& present = DeviceHooks::GetPresentParameters();

	if (g_settings.presentWidth <= 0 || g_settings.presentHeight <= 0)
	{
		ImGui::Text("Drawing at %ux%u (the game's own display setting)", present.BackBufferWidth,
			present.BackBufferHeight);
	}
	else
	{
		const unsigned askedWidth = static_cast<unsigned>(g_settings.presentWidth);
		const unsigned askedHeight = static_cast<unsigned>(g_settings.presentHeight);
		const bool exact = present.BackBufferWidth == askedWidth && present.BackBufferHeight == askedHeight;
		const bool rounded = !present.Windowed && !exact && present.BackBufferWidth >= askedWidth &&
			present.BackBufferHeight >= askedHeight;

		if (exact)
		{
			ImGui::TextColored(kGoodColour, "Drawing at %ux%u and %s it to fit the window", present.BackBufferWidth,
				present.BackBufferHeight, DeviceHooks::OverlayScale() > 1.0f ? "shrinking" : "stretching");
		}
		else if (rounded)
		{
			ImGui::TextColored(kGoodColour, "Drawing at %ux%u, the nearest monitor mode at or above the %dx%d you "
				"picked", present.BackBufferWidth, present.BackBufferHeight, g_settings.presentWidth,
				g_settings.presentHeight);
		}
		else
		{
			Warn("You picked %dx%d, but the game still draws at %ux%u. Restart the game, or change any video option "
				"in the game's menu.", g_settings.presentWidth, g_settings.presentHeight,
				present.BackBufferWidth, present.BackBufferHeight);
		}
	}

	if (!present.Windowed)
		Muted("Exclusive fullscreen: your monitor switches to this resolution instead of stretching the picture.");

	ImGui::Text("Back buffer multisampling: %s", present.MultiSampleType == D3DMULTISAMPLE_NONE ? "off" : "on");

	bool stageEffects = false;

	if (!EngineQuality::IsAvailable() || !EngineQuality::ReadStageEffects(stageEffects))
		return;

	ImGui::Text("%s: %s", EngineQuality::LeverName(), stageEffects ? "on" : "off");

	if (stageEffects && !EngineQuality::WantsStageEffects())
		Warn("It should be off. %s", EngineQuality::GetStatusText());
}

void PerformanceWindow::DrawMetricsTab()
{
	bool measuring = Profiler::IsEnabled();

	if (ImGui::Checkbox("Measure", &measuring))
	{
		g_settings.profiler = measuring;
		Profiler::SetEnabled(measuring);
		Settings::SaveBool("Debug", "Profiler", measuring);
	}

	Help("Measures the time between frames, how long each frame waits to be shown, and how long the mod's own work "
		"takes. Costs nothing while off.");

	if (!measuring)
	{
		ImGui::Spacing();
		Muted("Nothing is measured yet. Turn on Measure and play for a few seconds.");
		return;
	}

	ImGui::Spacing();

	const bool hasBaseline = Profiler::HasBaseline();

	static DWORD metricsTick = 0;
	static double fps = 0.0;
	static Profiler::Stats frame = {};
	static Profiler::Stats frameBaseline = {};
	static Profiler::Stats block = {};
	static Profiler::Stats tick = {};
	static Profiler::Stats tickBaseline = {};

	if (ShouldRefresh(metricsTick))
	{
		fps = Profiler::GetPresentedFps();
		frame = Profiler::GetPresentStats();
		frameBaseline = Profiler::GetBaselinePresentStats();
		block = Profiler::GetPresentBlockStats();
		tick = Profiler::GetTickStats();
		tickBaseline = Profiler::GetBaselineTickStats();
	}

	ImGui::Text("Running at %.1f frames per second", fps);

	ImGui::Spacing();
	DrawIntervalTable("Time between frames", frame, frameBaseline, hasBaseline, true);
	Muted("The time from one frame to the next. This is what smoothness is. Median is the typical frame. Spread "
		"and Average miss show judder. Frames over 20 ms are dropped frames.");

	ImGui::Spacing();
	ImGui::SeparatorText("Time between frames, close up");
	DrawFineHistogram();

	ImGui::Spacing();
	ImGui::SeparatorText("Time between frames, full range");
	DrawHistogram(&Profiler::GetHistogramBucket, Profiler::kHistogramBuckets, "##frameinterval");
	Muted("Every frame since the last reset, in 1 ms steps from 0 to 40 ms. Dropped frames show up as bars on the "
		"right.");

	ImGui::Spacing();
	DrawIntervalTable("Time to hand a frame to the driver", block, Profiler::Stats(), false, false);
	Muted("With vsync on, this is the wait for the monitor. Two groups here mean the refresh rate does not divide "
		"by 60.");

	ImGui::Spacing();
	DrawIntervalTable("Time between battle updates", tick, tickBaseline, hasBaseline, false);
	Muted("Only counts during a match. Reads zero while the game is paused for frame stepping.");

	ImGui::Spacing();
	DrawInputLag(block.medianMs);

	ImGui::Spacing();
	DrawSections(hasBaseline);

	ImGui::Spacing();
	DrawBaseline(hasBaseline);
}

void PerformanceWindow::DrawInputLag(double presentBlockMs)
{
	ImGui::SeparatorText("Input lag");
	InputLagMeter::KeepAlive(0);

	if (!InputLagMeter::IsAvailable())
	{
		ImGui::TextDisabled("Cannot measure: the game's input data was not found.");
	}
	else if (InputLagMeter::GetAverageMs() > 0.0f)
	{
		const float average = InputLagMeter::GetAverageMs();

		ImGui::Text("Average %.1f ms from %d good samples (%.1f frames). Last: %.1f ms", average,
			InputLagMeter::GetTrustedCount(), average / kFrameMs, InputLagMeter::GetLastMs());
	}
	else
	{
		ImGui::TextDisabled("Start a match and press a button as player 1 to measure.");
	}

	Muted("Measured from your button press until the game reads it. This is only the game's part of the delay. "
		"Direct3D 9 cannot report when the frame reaches the screen, so the display part below is an estimate.");

	const D3DPRESENT_PARAMETERS& present = DeviceHooks::GetPresentParameters();
	const double refreshMs = present.FullScreen_RefreshRateInHz != 0 ? 1000.0 / present.FullScreen_RefreshRateInHz
		: kFrameMs;

	ImGui::Text("Display part (estimate): %.1f ms", presentBlockMs + present.BackBufferCount * refreshMs);
}

void PerformanceWindow::DrawBaseline(bool hasBaseline)
{
	ImGui::SeparatorText("Before and after");

	Muted("To compare: turn on Measure. In training mode, play 30 seconds with the same character in the same "
		"corner. Press Capture baseline. Change one setting. Play the same 30 seconds again. Check the Change "
		"column, then press Copy summary.");

	if (hasBaseline)
		ImGui::Text("Baseline: %s", Profiler::GetBaselineLabel());
	else
		ImGui::TextDisabled("No baseline captured yet.");

	ImGui::Text("Frames measured: %d", Profiler::GetSampleCount());

	if (ImGui::Button("Capture baseline"))
	{
		char label[192] = {};
		BuildBaselineLabel(label, sizeof(label));
		Profiler::CaptureBaseline(label);
		Profiler::Reset();
		InputLagMeter::Reset();
	}

	ImGui::SameLine();

	if (ImGui::Button("Clear baseline"))
		Profiler::ClearBaseline();

	ImGui::SameLine();

	if (ImGui::Button("Restart measuring"))
	{
		Profiler::Reset();
		InputLagMeter::Reset();
	}

	ImGui::SameLine();

	if (ImGui::Button("Copy summary"))
	{
		char summary[2048] = {};
		Profiler::BuildSummary(summary, sizeof(summary));
		ImGui::SetClipboardText(summary);
	}

	ImGui::SameLine();

	if (ImGui::Button("Export CSV"))
		Profiler::ExportCsv(GetModRootPath("Logs\\performance.csv").c_str());

	ImGui::SameLine();

	if (ImGui::Button("Write to log"))
		Profiler::DumpToLog();
}
