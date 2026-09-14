#include "Overlay/Window/GraphicsPanel.h"

#include "Core/Settings.h"
#include "Core/interfaces.h"
#include "D3D9/Post/PostChain.h"
#include "D3D9/Post/PostOptions.h"
#include "D3D9/Post/SceneUpscale.h"
#include "D3D9/Post/ShaderPack.h"
#include "D3D9/Post/UpscaleFilter.h"
#include "Overlay/ComboNav.h"
#include "Overlay/UiScale.h"
#include "Overlay/UiText.h"
#include "Performance/EngineQuality.h"
#include "Performance/Improvements.h"
#include "Performance/PotatoMode.h"
#include "Performance/StageColor.h"

#include <imgui.h>

using UiText::Help;
using UiText::Muted;

namespace {

constexpr float kSliderWidth = 240.0f;
constexpr const char* kGraphics = "Graphics";

bool RadioRow(const char* id, int count, const char* (*name)(int), int current, int& outChosen)
{
	ImGui::PushID(id);

	bool changed = false;

	for (int candidate = 0; candidate < count; ++candidate)
	{
		if (candidate > 0)
			ImGui::SameLine();

		if (!ImGui::RadioButton(name(candidate), current == candidate) || candidate == current)
			continue;

		outChosen = candidate;
		changed = true;
	}

	ImGui::PopID();
	return changed;
}

bool SavedSlider(const char* label, int* value, int lowest, int highest, const char* key, const char* format)
{
	Ui::SetItemWidth(kSliderWidth);

	const bool changed = ImGui::SliderInt(label, value, lowest, highest, format);

	if (ImGui::IsItemDeactivatedAfterEdit())
		Settings::SaveInt(kGraphics, key, *value);

	return changed;
}

void DrawUpscaleFilter()
{
	ImGui::SeparatorText("Upscale filter");

	const int current = UpscaleFilter::Clamp(g_settings.upscaleFilter);
	int chosen = current;

	if (RadioRow("filter", UpscaleFilter::Kind_COUNT, &UpscaleFilter::GetName, current, chosen))
	{
		g_settings.upscaleFilter = chosen;
		Settings::SaveInt(kGraphics, "UpscaleFilter", chosen);
	}

	Help("When the game draws its scene into a texture smaller than the window, it stretches it with a basic "
		"bilinear filter. This replaces that stretch with a better one. Nothing in the game is patched.\n\nIt "
		"needs a back buffer bigger than the scene texture, so raise the Improvements level first.");

	Muted("%s", UpscaleFilter::Describe(current));
	Muted("%s", SceneUpscale::GetStatusText());
}

void DrawAntiAliasing()
{
	ImGui::SeparatorText("Anti-aliasing");

	const int current = AntiAlias::Clamp(g_settings.antiAliasing);
	int chosen = current;

	if (RadioRow("aa", AntiAlias::Level_COUNT, &AntiAlias::GetName, current, chosen))
	{
		g_settings.antiAliasing = chosen;
		Settings::SaveInt(kGraphics, "AntiAliasing", chosen);
	}

	Help("FXAA over the finished frame. It also softens the HUD text a little.");

	Muted("%s", AntiAlias::Describe(current));
}

void DrawBloom()
{
	ImGui::SeparatorText("Bloom");

	if (ImGui::Checkbox("Bloom", &g_settings.bloomEnabled))
		Settings::SaveBool(kGraphics, "Bloom", g_settings.bloomEnabled);

	Help("Cuts out the bright parts of the picture, blurs them at a quarter size and blends them back on top.\n\n"
		"Threshold is how bright a pixel must be to glow. Lower makes the whole frame hazy. Higher makes only real "
		"highlights glow.");

	if (!g_settings.bloomEnabled)
		return;

	ImGui::Indent();
	SavedSlider("Intensity", &g_settings.bloomIntensity, 0, 100, "BloomIntensity", "%d%%");
	SavedSlider("Threshold", &g_settings.bloomThreshold, 0, 100, "BloomThreshold", "%d%%");
	ImGui::Unindent();
}

void DrawSharpening()
{
	ImGui::SeparatorText("Sharpening");

	const int mode = SharpenMode::Clamp(g_settings.sharpenMode);
	int chosen = mode;

	if (RadioRow("sharpen", SharpenMode::Kind_COUNT, &SharpenMode::GetName, mode, chosen))
	{
		g_settings.sharpenMode = chosen;
		Settings::SaveInt(kGraphics, "SharpenMode", chosen);
	}

	Help("Restores the edge contrast lost when the picture is stretched to your window. 40 to 60% works best.");

	Muted("%s", SharpenMode::Describe(mode));

	if (mode == SharpenMode::Kind_Off)
		return;

	ImGui::Indent();
	SavedSlider("Strength", &g_settings.sharpenStrength, 0, 100, "Sharpen", g_settings.sharpenStrength > 0 ? "%d%%" : "off");
	ImGui::Unindent();
}

void DrawLook()
{
	ImGui::SeparatorText("Colour and display");

	if (ImGui::Checkbox("Colour and display", &g_settings.lookEnabled))
		Settings::SaveBool(kGraphics, "Look", g_settings.lookEnabled);

	Help("One pass over the finished frame. When off, the values below do nothing and no pass is drawn.");

	if (!g_settings.lookEnabled)
		return;

	ImGui::SameLine();

	if (ImGui::Button("Reset"))
		PostChain::ResetLook();

	ImGui::Indent();

	SavedSlider("Brightness", &g_settings.lookBrightness, -100, 100, "LookBrightness", "%d");
	SavedSlider("Contrast", &g_settings.lookContrast, -100, 100, "LookContrast", "%d");
	SavedSlider("Gamma", &g_settings.lookGamma, 25, 400, "LookGamma", "%d%%");
	SavedSlider("Saturation", &g_settings.lookSaturation, -100, 100, "LookSaturation", "%d");

	SavedSlider("Vibrance", &g_settings.lookVibrance, -100, 100, "LookVibrance", "%d");
	Help("Boosts dull colours and leaves colours that are already vivid alone.");

	SavedSlider("Warmth", &g_settings.lookTemperature, -100, 100, "LookTemperature", "%d");
	SavedSlider("Vignette", &g_settings.lookVignette, 0, 100, "LookVignette", "%d%%");
	SavedSlider("Scanlines", &g_settings.lookScanlines, 0, 100, "LookScanlines", "%d%%");

	if (ImGui::Checkbox("Dither", &g_settings.lookDither))
		Settings::SaveBool(kGraphics, "LookDither", g_settings.lookDither);

	Help("Adds a tiny amount of noise to hide colour banding in gradients.");

	ImGui::Unindent();
}

void DrawShaderPacks()
{
	ImGui::SeparatorText("Shader pack");

	const int selected = ShaderPack::GetSelected();

	Ui::SetItemWidth(kSliderWidth);

	if (ImGui::BeginCombo("Pack", selected < 0 ? "Off" : ShaderPack::GetName(selected)))
	{
		if (ImGui::Selectable("Off", selected < 0))
			ShaderPack::Select(-1);

		ComboNav::KeepSelectedInView(selected < 0);

		for (int i = 0; i < ShaderPack::Count(); ++i)
		{
			const bool chosen = i == selected;

			ImGui::PushID(i);

			if (ImGui::Selectable(ShaderPack::GetName(i), chosen))
				ShaderPack::Select(i);

			ComboNav::KeepSelectedInView(chosen);
			ImGui::PopID();
		}

		ImGui::EndCombo();
	}

	const int steps = ComboNav::WheelSteps();
	const int target = selected + steps;

	if (steps != 0 && target >= -1 && target < ShaderPack::Count())
		ShaderPack::Select(target);

	ImGui::SameLine();

	if (ImGui::Button("Rescan"))
		ShaderPack::Refresh();

	Help("Put a shader in the Shaders folder next to the ini and pick it here. It compiles when you select it and "
		"runs last, over the finished frame.\n\nAccepted: .hlsl and .ps as they are. .fx, .slang, .glsl, .frag and "
		".fsh are translated, and the result is saved in the Translated folder.\n\nOnly single pass shaders work. "
		"A shader that needs a second pass, a lookup texture, the depth buffer or the previous frame will "
		"translate but look wrong. The folder's README lists the bindings.\n\nCompiling needs d3dcompiler_47.dll, "
		"which comes with Windows and with Proton.");

	Muted("%s", ShaderPack::GetStatusText());
}

}

bool GraphicsPanel::DrawEverythingOff()
{
	if (!ImGui::Button("Everything off"))
	{
		Help("Resets every graphics setting in the mod to the game's defaults, on this tab and the others: present "
			"size, all shader stages, back buffer multisampling, stage multisampling, the empty stage and POTATO "
			"MODE.");
		return false;
	}

	PostChain::TurnOff();

	g_settings.upscaleFilter = UpscaleFilter::Kind_Off;
	g_settings.disableBackBufferAa = false;
	g_settings.plainStage = false;
	g_settings.simpleStage = false;

	Settings::SaveInt(kGraphics, "UpscaleFilter", 0);
	Settings::SaveBool(kGraphics, "DisableBackBufferAA", false);
	Settings::SaveBool(kGraphics, "PlainStage", false);
	Settings::SaveBool(kGraphics, "SimpleStage", false);

	StageColor::SetEnabled(false);
	EngineQuality::Apply();
	PotatoMode::Apply(PotatoMode::Level_Off);
	Improvements::Apply(Improvements::Level_Off);

	return true;
}

void GraphicsPanel::DrawShadersTab()
{
	ImGui::Spacing();

	DrawEverythingOff();

	ImGui::SameLine();
	Muted("Nothing here reaches the simulation, the inputs, or anything an opponent sees.");

	DrawUpscaleFilter();
	DrawAntiAliasing();
	DrawBloom();
	DrawSharpening();
	DrawLook();
	DrawShaderPacks();

	ImGui::Spacing();
	ImGui::SeparatorText("In force now");
	Muted("%s", PostChain::GetStatusText());
}
