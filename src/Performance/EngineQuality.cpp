#include "Performance/EngineQuality.h"

#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Core/utils.h"
#include "Performance/RenderMap.h"

namespace {

constexpr int kPollInterval = 60;
constexpr uint8_t kOff = 0;

uint8_t g_userMultisample = 1;
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

void FollowUserValues()
{
	const RenderAddresses& addresses = RenderMap::Addresses();

	ReadByte(addresses.stageMultisample, g_userMultisample);
	ReadByte(addresses.stageFxaa, g_userFxaa);
}

bool HoldOff()
{
	const RenderAddresses& addresses = RenderMap::Addresses();
	const bool multisample = WriteByte(addresses.stageMultisample, kOff);

	if (addresses.stageFxaa != 0)
		WriteByte(addresses.stageFxaa, kOff);

	return multisample;
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
		g_status = "stage multisampling and FXAA left as the game has them";
		return;
	}

	const RenderAddresses& addresses = RenderMap::Addresses();

	WriteByte(addresses.stageMultisample, g_userMultisample);
	WriteByte(addresses.stageFxaa, g_userFxaa);
	g_forcing = false;

	SetStatus("stage multisampling and FXAA put back");
}

}

bool EngineQuality::IsAvailable()
{
	return RenderMap::Addresses().stageMultisample != 0;
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
	uint8_t value = 0;

	if (!ReadByte(RenderMap::Addresses().stageMultisample, value))
		return false;

	outEnabled = value != kOff;
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
		g_status = "the stage multisampling switch was not found in this build";
		return;
	}

	if (!g_forcing)
		FollowUserValues();

	if (!HoldOff())
	{
		SetStatus("could not write the stage multisampling switch");
		return;
	}

	g_forcing = true;
	SetStatus("stage multisampling and FXAA held off");
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
