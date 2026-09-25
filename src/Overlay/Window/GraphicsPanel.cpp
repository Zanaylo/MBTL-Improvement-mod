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

	Help("The game draws the fight at 1280x720 and stretches it to your window with a basic filter. This uses a "
		"better filter for that stretch, so the picture is sharper.\n\nIt only works when the drawing size is "
		"bigger than 1280x720, so raise the Improvements level first.");

	Muted("%s", UpscaleFilter::Describe(current));
	Muted("%s", SceneUpscale::GetStatusText());
}

void DrawAntiAliasing()
{
	ImGui::SeparatorText("Antialiasing");

	const int current = AntiAlias::Clamp(g_settings.antiAliasing);
	int chosen = current;

	if (RadioRow("aa", AntiAlias::Level_COUNT, &AntiAlias::GetName, current, chosen))
	{
		g_settings.antiAliasing = chosen;
		Settings::SaveInt(kGraphics, "AntiAliasing", chosen);
	}

	Help("Smooths jagged edges (FXAA). It also softens the HUD text a little.");

	Muted("%s", AntiAlias::Describe(current));
}

void DrawBloom()
{
	ImGui::SeparatorText("Bloom");

	if (ImGui::Checkbox("Bloom", &g_settings.bloomEnabled))
		Settings::SaveBool(kGraphics, "Bloom", g_settings.bloomEnabled);

	Help("Makes the bright parts of the picture glow.\n\nIntensity is how strong the glow is. Threshold is how "
		"bright a pixel must be to glow: lower makes the whole picture hazy, higher makes only the brightest spots "
		"glow.");

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

	Help("Makes the picture sharper after it is stretched to your window. 40 to 60% works best.");

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

	Help("Adjusts brightness, colour and other effects on the final picture. When off, none of these changes are "
		"used.");

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

	Help("Adds a tiny bit of noise to hide colour bands in smooth gradients.");

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

	Help("Put a shader file in the Shaders folder (next to the ini), press Rescan, then pick it here. It runs on "
		"the final picture, after every other effect.\n\nFile types: .hlsl and .ps work as they are. .fx, .slang, "
		".glsl, .frag and .fsh are converted first, and the converted file is saved in the Translated folder.\n\n"
		"Only single pass shaders work. Shaders that need more passes, extra textures, depth or the previous "
		"frame will look wrong. See README.txt in the Shaders folder for details.\n\nNeeds d3dcompiler_47.dll, "
		"which comes with Windows and with Proton.");

	Muted("%s", ShaderPack::GetStatusText());
}

}

bool GraphicsPanel::DrawEverythingOff()
{
	if (!ImGui::Button("Everything off"))
	{
		Help("Turns off every graphics setting in the mod, on all tabs: drawing size, all shader effects, back "
			"buffer multisampling, stage multisampling, the black background and POTATO MODE.");
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

	StageColor::Apply();
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
	Muted("Nothing here affects gameplay, inputs or what your opponent sees.");

	DrawUpscaleFilter();
	DrawAntiAliasing();
	DrawBloom();
	DrawSharpening();
	DrawLook();
	DrawShaderPacks();

	ImGui::Spacing();
	ImGui::SeparatorText("Active now");
	Muted("%s", PostChain::GetStatusText());
}
