#include "Performance/StageColor.h"

#include "Core/logger.h"
#include "Hooks/HookManager.h"
#include "Performance/RenderMap.h"

#include <windows.h>

namespace {

constexpr uint32_t kOpaque = 0xFF000000u;
constexpr uint32_t kColourMask = 0x00FFFFFFu;

using StageGate_t = bool(__fastcall*)(void*, void*);

StageGate_t oStageGate = nullptr;

bool g_installed = false;
volatile LONG g_enabled = 0;
volatile LONG g_rgb = 0;
const char* g_status = "the stage ready check was not found in this build";

bool __fastcall HookedStageGate(void* self, void* unused)
{
	if (InterlockedCompareExchange(&g_enabled, 0, 0) != 0)
		return false;

	return oStageGate(self, unused);
}

}

bool StageColor::Install()
{
	const uint8_t* const gate = RenderMap::Addresses().stageGate;

	if (!gate)
	{
		LOG("StageColor: %s", g_status);
		return false;
	}

	g_installed = HookManager::CreateHook(const_cast<uint8_t*>(gate), reinterpret_cast<void*>(&HookedStageGate),
		reinterpret_cast<void**>(&oStageGate), "stage ready check");

	g_status = g_installed ? "ready" : "the stage ready check could not be hooked";
	return g_installed;
}

bool StageColor::IsAvailable()
{
	return g_installed;
}

void StageColor::SetEnabled(bool enabled)
{
	InterlockedExchange(&g_enabled, enabled && g_installed ? 1 : 0);
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

const char* StageColor::StatusText()
{
	return g_status;
}
