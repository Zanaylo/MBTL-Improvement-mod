#pragma once

namespace PaletteShare
{
	void Initialize();
	void OnFrame();

	const char* GetStatusText();
	const char* GetRemoteName(int seat);
}
