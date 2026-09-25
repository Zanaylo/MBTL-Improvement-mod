#include "Performance/EngineQuality.h"

#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Core/utils.h"
#include "Game/GameOffsets.h"
#include "Hooks/CodePatch.h"
#include "Performance/RenderMap.h"

#include <cstring>

namespace {

namespace Render = GameOffsets::Render;

constexpr int kPollInterval = 60;
constexpr uint8_t kOff = 0;

uint8_t g_originalRead[Render::kLoadSamplesLength] = {};
bool g_readSaved = false;
uint8_t g_userFxaa = 0;
bool g_forcing = false;
int g_framesUntilPoll = 0;
const char* g_status = "not applied yet";

bool ReadByte(uintptr_t address, uint8_t& out)
{
	return address != 0 && TryRead(address, out);
}

bool WriteByte(uintptr_t address, uint8_t value)
{
	uint8_t current = 0;

	if (address == 0)
		return false;

	return (ReadByte(address, current) && current == value) || TryWrite(address, value);
}

bool PatchSampleRead(bool plain)
{
	uint8_t* const read = RenderMap::Addresses().stageSampleRead;

	if (!read)
		return false;

	if (!g_readSaved)
	{
		std::memcpy(g_originalRead, read, sizeof(g_originalRead));
		g_readSaved = true;
	}

	const uint8_t* const wanted = plain ? Render::kNoSamples : g_originalRead;

	if (std::memcmp(read, wanted, sizeof(g_originalRead)) == 0)
		return true;

	return CodePatch::Write(read, wanted, sizeof(g_originalRead));
}

void FollowUserValues()
{
	ReadByte(RenderMap::Addresses().stageFxaa, g_userFxaa);
}

bool HoldOff()
{
	WriteByte(RenderMap::Addresses().stageFxaa, kOff);
	return PatchSampleRead(true);
}

void SetStatus(const char* status)
{
	g_status = status;
	LOG("[EngineQuality] %s", g_status);
}

void Restore()
{
	if (!g_forcing)
	{
		g_status = "stage multisampling and FXAA use the game's settings";
		return;
	}

	PatchSampleRead(false);
	WriteByte(RenderMap::Addresses().stageFxaa, g_userFxaa);
	g_forcing = false;

	SetStatus("stage multisampling and FXAA restored");
}

}

bool EngineQuality::IsAvailable()
{
	return RenderMap::Addresses().stageSampleRead != nullptr;
}

const char* EngineQuality::LeverName()
{
	return "Stage multisampling";
}

bool EngineQuality::WantsStageEffects()
{
	return !g_settings.plainStage;
}

bool EngineQuality::ReadStageEffects(bool& outEnabled)
{
	const uintptr_t samples = RenderMap::Addresses().stageSamples;
	uint32_t value = 0;

	if (samples == 0 || !TryRead(samples, value))
		return false;

	outEnabled = value != 0;
	return true;
}

void EngineQuality::Apply()
{
	g_framesUntilPoll = kPollInterval;

	if (WantsStageEffects())
	{
		Restore();
		return;
	}

	if (!IsAvailable())
	{
		g_status = "the stage multisampling setting was not found in this game version";
		return;
	}

	if (!g_forcing)
		FollowUserValues();

	if (!HoldOff())
	{
		SetStatus("could not change the stage multisampling setting");
		return;
	}

	g_forcing = true;
	SetStatus("stage multisampling and FXAA kept off. Multisampling changes on the next stage load");
}

void EngineQuality::OnFrame()
{
	if (--g_framesUntilPoll > 0)
		return;

	g_framesUntilPoll = kPollInterval;

	if (!g_forcing)
	{
		FollowUserValues();
		return;
	}

	HoldOff();
}

const char* EngineQuality::GetStatusText()
{
	return g_status;
}
