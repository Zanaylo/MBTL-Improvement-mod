#include "Training/BattleHud.h"

#include "Core/Settings.h"
#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Core/utils.h"
#include "D3D9/DeviceHooks.h"
#include "Game/Anchors.h"
#include "Game/GameOffsets.h"
#include "Hooks/ImageScanner.h"
#include "Training/GameState.h"

#include <cstring>
#include <vector>

namespace {

namespace Cockpit = GameOffsets::Cockpit;

uintptr_t g_global = 0;
uintptr_t g_viewOffset = 0;
const char* g_status = "does not work in this game version";

uintptr_t ResolveGlobal(const uint8_t* native)
{
	std::vector<uintptr_t> globals;

	for (uint8_t* target : ImageScanner::CallTargets(native))
	{
		const uintptr_t global = ImageScanner::MemoryGetterValue(target);

		if (global != 0)
			globals.push_back(global);
	}

	if (globals.size() == 1)
		return globals.front();

	LOG("BattleHud: the cockpit getter has %u candidate(s), expected exactly one", static_cast<unsigned>(globals.size()));
	return 0;
}

uintptr_t ResolveViewOffset(const uint8_t* native)
{
	std::vector<uintptr_t> offsets;

	for (uint8_t* target : ImageScanner::CallTargets(native))
	{
		if (!ImageScanner::InCode(target, Cockpit::kSetterWindow))
			continue;

		const size_t length = ImageScanner::FunctionLength(target);

		for (size_t i = 0; i + sizeof(Cockpit::kSetterStore) < length && i < Cockpit::kSetterWindow; ++i)
		{
			if (std::memcmp(target + i, Cockpit::kSetterStore, sizeof(Cockpit::kSetterStore)) != 0)
				continue;

			offsets.push_back(target[i + sizeof(Cockpit::kSetterStore)]);
			break;
		}
	}

	if (offsets.size() == 1)
		return offsets.front();

	LOG("BattleHud: the cockpit view setter has %u candidate(s), expected exactly one",
		static_cast<unsigned>(offsets.size()));
	return 0;
}

bool CanTouch()
{
	return GameState::IsBattleRunning() && GameState::IsOnlineKnown() && !GameState::IsOnline();
}

void WriteView(uint32_t view)
{
	uint32_t cockpit = 0;
	uint32_t current = 0;

	if (g_global == 0 || !TryRead(g_global, cockpit) || cockpit == 0)
		return;

	const uintptr_t field = cockpit + g_viewOffset;

	if (TryRead(field, current) && current == view)
		return;

	TryWrite(field, view);
}

class HudListener final : public IDeviceListener
{
public:
	void OnPresent(IDirect3DDevice9*) override
	{
		if (!g_settings.hideHud || !CanTouch())
			return;

		WriteView(Cockpit::kViewHidden);
	}
};

HudListener g_listener;

}

void BattleHud::Install()
{
	const uint8_t* const native = ImageScanner::NativeFunction(Cockpit::kViewNative);

	if (native == nullptr)
		LOG("BattleHud: the script native %s was not found", Cockpit::kViewNative);

	g_global = native ? ResolveGlobal(native) : 0;
	g_viewOffset = native ? ResolveViewOffset(native) : 0;

	Anchors::Record("Battle cockpit", g_global, "hide the HUD");

	if (!IsAvailable())
		return;

	g_status = "";
	DeviceHooks::AddListener(&g_listener);
	LOG("BattleHud: the HUD view is at +0x%X of the cockpit", static_cast<unsigned>(g_viewOffset));
}

bool BattleHud::IsAvailable()
{
	return g_global != 0 && g_viewOffset != 0;
}

bool BattleHud::IsHidden()
{
	return g_settings.hideHud;
}

void BattleHud::SetHidden(bool hidden)
{
	g_settings.hideHud = hidden;
	Settings::SaveBool("Training", "HideHud", hidden);

	if (!hidden && CanTouch())
		WriteView(Cockpit::kViewShown);
}

void BattleHud::Toggle()
{
	SetHidden(!IsHidden());
}

const char* BattleHud::StatusText()
{
	return g_status;
}
