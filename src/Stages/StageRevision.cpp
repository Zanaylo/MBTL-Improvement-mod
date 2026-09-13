#include "Stages/StageRevision.h"

#include <atomic>

namespace {

constexpr uint32_t kFirst = 1;

std::atomic<uint32_t> g_revision{ kFirst };

}

void StageRevision::Bump()
{
	++g_revision;
}

uint32_t StageRevision::Current()
{
	return g_revision.load();
}

bool StageRevision::Changed()
{
	return g_revision.load() != kFirst;
}
