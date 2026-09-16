#include "Game/SceneWatch.h"

#include "Core/logger.h"
#include "Core/utils.h"
#include "Game/GameOffsets.h"
#include "Game/SceneMap.h"

namespace {

constexpr unsigned kSettleFrames = 4;
constexpr int kMaxLogged = 48;

uint32_t g_settled = SceneWatch::kNone;
uint32_t g_first = SceneWatch::kNone;
uint32_t g_candidate = SceneWatch::kNone;
unsigned g_held = 0;
int g_logged = 0;

}

void SceneWatch::OnFrame()
{
	const uintptr_t sceneId = SceneMap::Addresses().sceneId;
	uint32_t scene = 0;

	if (sceneId == 0 || !TryRead(sceneId, scene))
		return;

	if (scene != g_candidate)
	{
		g_candidate = scene;
		g_held = 0;
		return;
	}

	if (g_held < kSettleFrames)
		++g_held;

	if (g_held < kSettleFrames || scene == g_settled)
		return;

	const uint32_t previous = g_settled;
	g_settled = scene;

	if (g_first == kNone)
		g_first = scene;

	if (g_logged >= kMaxLogged)
		return;

	++g_logged;
	LOG("SceneWatch: scene %u, was %u", scene, previous);
}

uint32_t SceneWatch::Current()
{
	return g_settled;
}

uint32_t SceneWatch::First()
{
	return g_first;
}
