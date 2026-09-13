#pragma once

namespace PaletteControl
{
	constexpr int kSides = 2;

	void OnFrame();

	bool IsOnline();
	bool IsSpectating();

	int LocalPlayer();

	bool CanEdit(int seat);
	bool CanWear(int seat);

	const char* WhyNot(int seat);
}
