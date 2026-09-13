#pragma once

#include "Game/FileOverlay.h"

#include <string>

namespace ModFiles
{
	bool Initialize();

	void AddOverlay(IFileOverlay* overlay);
	void Rescan();
	void OnFrame();

	int Count();
	int OwnCount();
	int Served();

	const char* Root();
	const char* StatusText();
}
