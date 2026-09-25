#include "Performance/StageColor.h"

#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Game/GameOffsets.h"
#include "Hooks/CodePatch.h"
#include "Performance/RenderMap.h"

#include <windows.h>

#include <cstring>

namespace {

namespace Render = GameOffsets::Render;

constexpr uint32_t kOpaque = 0xFF000000u;
constexpr uint32_t kColourMask = 0x00FFFFFFu;
constexpr uint32_t kBlack = 0x000000u;

uint8_t* g_branch = nullptr;
uint8_t g_original[Render::kSkipOpcodeLength] = {};
volatile LONG g_enabled = 0;
volatile LONG g_rgb = 0;
const char* g_status = "not supported by this game version";

}

bool StageColor::Install()
{
	uint8_t* const branch = RenderMap::Addresses().stageSkip;

	if (!branch)
	{
		LOG("StageColor: %s", g_status);
		return false;
	}

	std::memcpy(g_original, branch, sizeof(g_original));
	g_branch = branch;
	g_status = "ready";
	LOG("StageColor: ready, the stage draw is skipped at MBTL.exe+0x%X",
		static_cast<unsigned>(reinterpret_cast<uintptr_t>(branch) - reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr))));
	return true;
}

bool StageColor::IsAvailable()
{
	return g_branch != nullptr;
}

void StageColor::SetEnabled(bool enabled)
{
	const LONG wanted = enabled && g_branch ? 1 : 0;

	if (InterlockedExchange(&g_enabled, wanted) == wanted || !g_branch)
		return;

	if (!CodePatch::Write(g_branch, wanted ? Render::kSkipAlways : g_original, Render::kSkipOpcodeLength))
		LOG("StageColor: the stage draw could not be %s", wanted ? "skipped" : "restored");
}

bool StageColor::IsEnabled()
{
	return InterlockedCompareExchange(&g_enabled, 0, 0) != 0;
}

void StageColor::SetColor(uint32_t rgb)
{
	InterlockedExchange(&g_rgb, static_cast<LONG>(rgb & kColourMask));
}

uint32_t StageColor::GetClearColor()
{
	return kOpaque | (static_cast<uint32_t>(InterlockedCompareExchange(&g_rgb, 0, 0)) & kColourMask);
}

void StageColor::Apply()
{
	SetColor(g_settings.flatStage ? static_cast<uint32_t>(g_settings.flatStageColour) : kBlack);
	SetEnabled(g_settings.flatStage || g_settings.simpleStage);
}

const char* StageColor::StatusText()
{
	return g_status;
}
