#pragma once

#include <cstdint>

namespace StageOverlays
{
	struct Stats
	{
		uint32_t listBytes;
		int listApplied;
		int namesApplied;
		int musicApplied;
		int vsServed;
	};

	void Register();

	Stats Snapshot();
}
