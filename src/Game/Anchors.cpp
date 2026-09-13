#include "Game/Anchors.h"

#include <windows.h>

namespace {

std::vector<Anchors::Anchor> g_anchors;
SRWLOCK g_lock = SRWLOCK_INIT;

}

void Anchors::Record(const char* name, uintptr_t address, const char* note)
{
	AcquireSRWLockExclusive(&g_lock);

	for (Anchor& anchor : g_anchors)
	{
		if (anchor.name != name)
			continue;

		anchor.address = address;
		anchor.note = note;
		ReleaseSRWLockExclusive(&g_lock);
		return;
	}

	g_anchors.push_back({ name, address, note });
	ReleaseSRWLockExclusive(&g_lock);
}

void Anchors::Snapshot(std::vector<Anchor>& out)
{
	AcquireSRWLockShared(&g_lock);
	out = g_anchors;
	ReleaseSRWLockShared(&g_lock);
}
