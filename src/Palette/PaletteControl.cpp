#include "Palette/PaletteControl.h"

#include "Core/interfaces.h"
#include "Network/SteamNetwork.h"
#include "Palette/PaletteOwner.h"
#include "Training/GameState.h"

namespace {

bool g_online = false;
bool g_spectating = false;
int g_local = -1;

bool Valid(int seat)
{
	return seat >= 0 && seat < PaletteOwner::kSeats;
}

bool IsTheirs(int seat)
{
	return g_local >= 0 && PaletteOwner::SideOf(seat) != g_local;
}

}

void PaletteControl::OnFrame()
{
	g_online = GameState::IsOnline();

	const int side = g_online ? SteamNetwork::GetOwnSide() : -1;

	g_spectating = g_online && SteamNetwork::IsHooked() && side < 0;
	g_local = side >= 0 && side < kSides ? side : -1;
}

bool PaletteControl::IsOnline()
{
	return g_online;
}

bool PaletteControl::IsSpectating()
{
	return g_spectating;
}

int PaletteControl::LocalPlayer()
{
	return g_local;
}

bool PaletteControl::CanEdit(int seat)
{
	return Valid(seat) && !g_spectating && !IsTheirs(seat);
}

bool PaletteControl::CanWear(int seat)
{
	if (!Valid(seat))
		return false;

	if (g_spectating || IsTheirs(seat))
		return g_settings.showOnlinePalettes;

	return true;
}

const char* PaletteControl::WhyNot(int seat)
{
	if (!Valid(seat))
		return "";

	if (g_spectating)
		return "You are watching this match, so neither character's colours are yours to choose. What the players are "
			"wearing arrives from them.";

	if (IsTheirs(seat))
		return "This is the other player's character. They choose its colours, and what they pick arrives here if they "
			"are running the mod.";

	return "";
}
