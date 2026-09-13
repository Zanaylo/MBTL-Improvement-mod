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
constexpr const char* kAutomatic = "Automatic - leave the desktop's own mode";

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
		"Fixes what alt-tab leaves behind. No downside.",
		"Since Windows 10 2004 a 1 ms timer request belongs to the process that made it, and Windows "
		"takes it back while that process sits in the background. After an alt-tab every sleep can "
		"last 15.6 ms instead of 1. This holds the request and asks again when the window comes back.",
	},
	{
		&g_settings.powerThrottlingOptOut,
		"PowerThrottlingOptOut",
		"Stop Windows throttling the game in the background",
		"The other half of the same fix. Costs a little power out of focus.",
		"EcoQoS parks a background process on the efficient cores and clamps its timer resolution. "
		"Opting out keeps the game on the fast cores and keeps the millisecond timer above.",
	},
	{
		&g_settings.pumpWait,
		"PumpWait",
		"Keep the game's frame sleep on a precise timer",
		"The sleep after each Present lasts one millisecond, not fifteen. No CPU cost.",
		"MBTL runs its window, its simulation and its drawing on one thread, and after every Present that thread "
		"sleeps for a millisecond. Once an alt-tab has clamped the Windows timer that sleep can last 15.6 ms, "
		"which is a dropped frame.\n\n"
		"This puts every one and two millisecond sleep in the game on a high resolution waitable timer, which "
		"Windows does not clamp. It costs no CPU, patches no game code and switches off cleanly.",
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
	ImGui::TextUnformatted("Within half a ms of 16.67");

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
		StatRow("Spread (standard deviation)", now.stddevMs, before.stddevMs, hasBaseline);
		StatRow("Off target, average", now.madMs, before.madMs, hasBaseline);
	}

	StatRow("99th percentile", now.p99Ms, before.p99Ms, hasBaseline);
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
	UiText::Muted("The same frames in quarter-millisecond buckets from %.2f to %.2f ms. Smooth is one spike in the "
		"middle. Two spikes either side of it is judder, and it is invisible to the median.", base,
		base + Profiler::kFineBuckets * Profiler::kFineBucketMs);

	double firstMs = 0.0;
	double secondMs = 0.0;
	double separationMs = 0.0;

	if (!Profiler::FindModes(firstMs, secondMs, separationMs))
		return;

	Warn("Two clusters, %.2f ms and %.2f ms, %.2f ms apart. Frames are landing either side of the target rather "
		"than on it.", firstMs, secondMs, separationMs);
}

void DrawSections(bool hasBaseline)
{
	ImGui::SeparatorText("Where the time in a frame goes");
	Muted("The mod's own work in milliseconds, averaged over recent frames. oPresent and oBattleTick are the game "
		"itself, for comparison.");

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

	if (!ImGui::BeginCombo("Fullscreen refresh", preview))
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
	UiText::Muted("The characters and the stage are drawn into a %dx%d scene target first and stretched afterwards, "
		"so they gain no detail beyond that size.", GameOffsets::Render::kSceneWidth, GameOffsets::Render::kSceneHeight);
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
		Warn("No Direct3D device found. Something else is wrapping Direct3D, and none of the display settings "
			"below are in force.");
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

	ImGui::Text("Presenting %.1f frames a second", fps);

	if (g_settings.pumpWait)
		ImGui::Text("Precise frame sleep %s", PumpWait::IsActive() ? "in force" : "not in force");

	if (present.Windowed)
		Muted("Windowed, the desktop compositor owns the presentation, so the display settings below do nothing.");

	if (!present.Windowed && VsyncIsOn(present) && present.FullScreen_RefreshRateInHz % 60 != 0)
	{
		Warn("%u Hz cannot show 60 frames a second evenly, so with vsync on they land alternately early and late. "
			"Pick a refresh below that divides by 60, or turn the game's vsync off.",
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

	if (ImGui::Checkbox("Let the mod choose the display parameters", &g_settings.displayTuning))
	{
		SaveVideo("DisplayTuning", g_settings.displayTuning);
		changed = true;
	}

	Help("Off leaves the refresh rate and the back buffer count exactly as the game asked Direct3D for them.");

	ImGui::BeginDisabled(!g_settings.displayTuning);

	changed = DrawRefreshCombo() || changed;

	Help("With the game's vsync off Present never waits, so the refresh rate only decides how long a frame takes "
		"to reach the glass, and leaving the desktop's own mode alone is both the fastest and the cheapest to "
		"alt-tab out of.\n\n"
		"With vsync on and a rate that does not divide by 60, frames land alternately early and late. Automatic "
		"then picks the highest listed rate that does divide by 60.\n\nRestart the game to apply.");

	ImGui::BeginDisabled(!vsync);

	if (ImGui::Checkbox("A second back buffer", &g_settings.extraBackBuffer))
	{
		SaveVideo("ExtraBackBuffer", g_settings.extraBackBuffer);
		changed = true;
	}

	Help("A second buffer gives a frame that misses the vblank somewhere to wait instead of costing a whole "
		"refresh, and costs up to a frame of input latency to get.\n\n"
		"It only does anything with vsync on. With vsync off there is no vblank to miss, so the buffer becomes a "
		"queued frame of delay and nothing else.");

	ImGui::EndDisabled();

	if (!vsync)
	{
		ImGui::Indent();
		Muted("Greyed out because the game's vsync is off, where a second buffer is pure latency.");
		ImGui::Unindent();
	}

	ImGui::EndDisabled();
	ImGui::EndDisabled();

	if (windowed)
	{
		Muted("Greyed out because the game is windowed. Direct3D requires a zero refresh rate there, and the "
			"compositor already holds a frame of its own.");
	}

	return changed;
}

bool PerformanceWindow::DrawAdvanced()
{
	if (!ImGui::CollapsingHeader("Advanced"))
		return false;

	ImGui::BeginDisabled(!g_settings.pumpWait);

	const bool changed = ImGui::Checkbox("End the frame sleep as soon as input arrives", &g_settings.pumpWaitAllInput);

	ImGui::EndDisabled();

	if (changed)
		SaveVideo("PumpWaitAllInput", g_settings.pumpWaitAllInput);

	Help("The game reads input at the start of its next frame. Ending the sleep the moment a keyboard or mouse "
		"message arrives lets that read come up to a millisecond sooner, and the game's own limiter still holds 60 "
		"frames a second. Costs CPU in proportion to how much the mouse moves. Needs the option above switched on.");

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
		ImGui::SetTooltip("Everything above on, the display left as the desktop has it, and the game's own single "
			"back buffer. Nothing here costs a core.");
	}

	ImGui::SameLine();

	if (ImGui::Button("As the game ships"))
	{
		ApplyPreset(false, false, false, false);
		changed = true;
	}

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Everything off. This is the thing to measure against.");

	return changed;
}

void PerformanceWindow::DrawPerformanceTab()
{
	ImGui::Spacing();
	ImGui::SeparatorText("What is actually happening");
	DrawWhatIsHappening();

	ImGui::Spacing();
	ImGui::SeparatorText("Choices");

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

	Muted("Every level here changes how the frame is drawn and nothing else. None of them reaches the simulation, "
		"none of them is visible to an opponent, and the stage keeps drawing at all of them.");

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

	Muted("The drawing size takes effect the next time the game builds its display - restart it, or touch any video "
		"option in its own menu. Everything else is immediate. In exclusive fullscreen the back buffer has to name "
		"a mode your monitor really has, so the size is rounded up to the smallest listed one that fits.");

	ImGui::Spacing();
	changed = DrawEmptyStage() || changed;

	ImGui::Spacing();
	ImGui::SeparatorText("In force now");
	DrawPotatoState();

	if (!changed)
		return;

	Profiler::Reset();
}

bool PerformanceWindow::DrawEmptyStage()
{
	const bool available = StageColor::IsAvailable();

	ImGui::BeginDisabled(!available);
	const bool changed = ImGui::Checkbox("Draw the empty stage", &g_settings.simpleStage);
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
		Muted("Deliberately not part of any level above: a match with no background is a worse trade than a soft "
			"one. Here for a machine that still cannot hold 60 on Potato.");
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
	Muted("POTATO MODE the other way round: the frame is drawn larger than your window and fitted back down.");

	ImGui::Spacing();
	ImGui::SeparatorText("Present size");

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

	Help("Everything drawn straight into the back buffer - the HUD, the menus and this overlay - is supersampled. "
		"Windowed only, takes effect after a restart, and it costs fill rate: 4K is nine times 720p.");

	UiText::Muted("%s", Improvements::Describe(Improvements::GetLevel()));
	DrawSceneTargetNote();

	ImGui::Spacing();
	ImGui::SeparatorText("In force now");
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
		ImGui::Text("Drawing at %ux%u, which is the game's own display option", present.BackBufferWidth,
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
			ImGui::TextColored(kGoodColour, "Drawing at %ux%u and %s it to the window", present.BackBufferWidth,
				present.BackBufferHeight, DeviceHooks::OverlayScale() > 1.0f ? "fitting" : "stretching");
		}
		else if (rounded)
		{
			ImGui::TextColored(kGoodColour, "Drawing at %ux%u - the smallest mode your monitor lists at or above the "
				"%dx%d asked for", present.BackBufferWidth, present.BackBufferHeight, g_settings.presentWidth,
				g_settings.presentHeight);
		}
		else
		{
			Warn("Asked to draw at %dx%d, still drawing at %ux%u. The display is built once - restart the game, or "
				"touch any video option in its own menu.", g_settings.presentWidth, g_settings.presentHeight,
				present.BackBufferWidth, present.BackBufferHeight);
		}
	}

	if (!present.Windowed)
	{
		Muted("Exclusive fullscreen: the size names a real display mode, so your monitor changes mode rather than "
			"the picture being stretched inside a window.");
	}

	ImGui::Text("Back buffer multisampling %s", present.MultiSampleType == D3DMULTISAMPLE_NONE ? "off" : "on");

	bool stageEffects = false;

	if (!EngineQuality::IsAvailable() || !EngineQuality::ReadStageEffects(stageEffects))
		return;

	ImGui::Text("%s is %s", EngineQuality::LeverName(), stageEffects ? "on" : "off");

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

	Help("Times the gap between finished frames, how long Present blocks, and every step of the mod's own work. Off "
		"by default so a session nobody is measuring pays nothing for it.");

	if (!measuring)
	{
		ImGui::Spacing();
		Muted("Nothing is being measured. Switch Measure on and play for a few seconds.");
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

	ImGui::Text("Presenting %.1f frames a second", fps);

	ImGui::Spacing();
	DrawIntervalTable("Frame interval", frame, frameBaseline, hasBaseline, true);
	Muted("The gap between one finished frame and the next, which is what smoothness is. Median is the typical "
		"frame; the spread and the off-target average are the judder. Frames over 20 ms are dropped frames.");

	ImGui::Spacing();
	ImGui::SeparatorText("Frame interval, close up");
	DrawFineHistogram();

	ImGui::Spacing();
	ImGui::SeparatorText("Frame interval, whole range");
	DrawHistogram(&Profiler::GetHistogramBucket, Profiler::kHistogramBuckets, "##frameinterval");
	Muted("Every frame since the last reset, in 1 ms buckets from 0 to 40 ms. A dropped frame shows up as a tail to "
		"the right.");

	ImGui::Spacing();
	DrawIntervalTable("Present", block, Profiler::Stats(), false, false);
	Muted("How long the call that hands the frame to the driver takes. With vsync on this is the wait for the "
		"vblank, and two clusters here are a display that cannot divide 60 evenly.");

	ImGui::Spacing();
	DrawIntervalTable("Battle tick", tick, tickBaseline, hasBaseline, false);
	Muted("The gap between two runs of the battle simulation. Only runs during a match, and reads zero while frame "
		"stepping has the game paused.");

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
		ImGui::TextDisabled("The character's input field has not been found, so nothing is measured.");
	}
	else if (InputLagMeter::GetAverageMs() > 0.0f)
	{
		const float average = InputLagMeter::GetAverageMs();

		ImGui::Text("%.1f ms average over %d trusted samples (%.1f frames), last %.1f ms", average,
			InputLagMeter::GetTrustedCount(), average / kFrameMs, InputLagMeter::GetLastMs());
	}
	else
	{
		ImGui::TextDisabled("Start a match and press something as player 1 to measure.");
	}

	Muted("Measured from a physical press to the character's own input field changing, so it is the half of the "
		"delay the simulation can see. It cannot see the swap chain or the scan-out - Direct3D 9 without the Ex "
		"interfaces reports neither - so the display half below is arithmetic, not a measurement.");

	const D3DPRESENT_PARAMETERS& present = DeviceHooks::GetPresentParameters();
	const double refreshMs = present.FullScreen_RefreshRateInHz != 0 ? 1000.0 / present.FullScreen_RefreshRateInHz
		: kFrameMs;

	ImGui::Text("Display side, estimated: %.1f ms", presentBlockMs + present.BackBufferCount * refreshMs);
}

void PerformanceWindow::DrawBaseline(bool hasBaseline)
{
	ImGui::SeparatorText("Before and after");

	Muted("Turn Measure on. Go to training mode, same character, same corner. Play thirty seconds. Press Capture "
		"baseline. Change one thing. Play the same thirty seconds. Read the Change column, then Copy summary.");

	if (hasBaseline)
		ImGui::Text("Baseline: %s", Profiler::GetBaselineLabel());
	else
		ImGui::TextDisabled("No baseline captured yet.");

	ImGui::Text("Live sample: %d frames", Profiler::GetSampleCount());

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

	if (ImGui::Button("Reset live"))
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
