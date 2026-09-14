#include "Stages/StageTint.h"

#include "Core/logger.h"
#include "Core/utils.h"
#include "Game/Anchors.h"
#include "Game/GameOffsets.h"
#include "Hooks/ImageScanner.h"
#include "Stages/StageArchive.h"
#include "Stages/StageLibrary.h"
#include "Training/BattleMap.h"
#include "Training/GameState.h"
#include "Training/TickListener.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

namespace Tint = GameOffsets::Tint;
namespace Objects = GameOffsets::Objects;

constexpr const char* kKey = "CharaTint";
constexpr const char* kNoTint = "No stage asks for a character tint.";
constexpr int32_t kNoStage = -2;
constexpr uint8_t kSlotActive = 1;
constexpr uint32_t kColourMask = 0xFFFFFF;

uintptr_t g_currentStage = 0;
int32_t g_stage = kNoStage;
uint32_t g_colour = 0;
bool g_wanted = false;
char g_status[160] = "Not started.";

uintptr_t ResolveCurrentStage()
{
	const std::vector<uint8_t*> loaders = ImageScanner::FunctionsReferencing(ImageScanner::FindString(Tint::kLoaderAnchor));

	if (loaders.size() != 1)
		return 0;

	const uint8_t* const loader = loaders.front();
	const size_t length = ImageScanner::FunctionLength(loader);
	std::vector<uintptr_t> globals;

	for (size_t i = 0; i + Tint::kStoreStageDispAt + sizeof(uint32_t) <= length; ++i)
	{
		if (std::memcmp(loader + i, Tint::kStoreStage, sizeof(Tint::kStoreStage)) == 0)
			globals.push_back(ImageScanner::ReadDword(loader + i + Tint::kStoreStageDispAt));
	}

	return globals.size() == 1 && ImageScanner::InData(globals.front()) ? globals.front() : 0;
}

bool ReadTint(int32_t stage, uint32_t& out)
{
	std::vector<uint8_t> blob;
	std::string value;

	if (stage <= 0 || !ReadWholeFile(StageLibrary::NoteOf(stage), blob) ||
		!StageArchive::Field(std::string(blob.begin(), blob.end()), kKey, value))
	{
		return false;
	}

	char* end = nullptr;
	out = strtoul(value.c_str(), &end, 0) & kColourMask;

	return end != value.c_str();
}

void Follow(int32_t stage)
{
	g_stage = stage;
	g_wanted = ReadTint(stage, g_colour);

	if (!g_wanted)
	{
		sprintf_s(g_status, "%s", kNoTint);
		return;
	}

	sprintf_s(g_status, "Stage %d tints the characters 0x%06X (offline only).", stage, g_colour);
	LOG("StageTint: %s", g_status);
}

void Paint(uintptr_t chara)
{
	const uintptr_t tint = chara + Tint::kTint;
	const int16_t hold = Tint::kHoldFrames;
	const int16_t fadeIn = 0;

	TryWrite(tint, g_colour);
	TryWrite(tint + Tint::kTimeAt, hold);
	TryWrite(tint + Tint::kTotalAt, hold);
	TryWrite(tint + Tint::kInAt, fadeIn);
	TryWrite(tint + Tint::kTypeAt, Tint::kTypeSolid);
}

class TintTicker : public ITickListener
{
public:
	void OnBattleTick() override
	{
		int32_t stage = kNoStage;

		if (g_currentStage == 0 || !TryRead(g_currentStage, stage))
			return;

		if (stage != g_stage)
			Follow(stage);

		if (!g_wanted || !GameState::IsOnlineKnown() || GameState::IsOnline())
			return;

		const BattleAddresses& addresses = BattleMap::Addresses();

		if (addresses.charaArray == 0 || addresses.charaStride == 0)
			return;

		for (int seat = 0; seat < Tint::kSeats; ++seat)
		{
			const uintptr_t chara = addresses.charaArray + static_cast<uintptr_t>(seat) * addresses.charaStride;
			uint8_t active = 0;

			if (TryRead(chara + Objects::kActive, active) && active == kSlotActive)
				Paint(chara);
		}
	}
};

TintTicker g_ticker;

}

void StageTint::Install()
{
	g_currentStage = ResolveCurrentStage();
	Anchors::Record("Current stage number", g_currentStage, "character tint");

	if (g_currentStage == 0)
	{
		sprintf_s(g_status, "Not available: the current stage number was not found.");
		LOG("StageTint: %s", g_status);
		return;
	}

	sprintf_s(g_status, "%s", kNoTint);
	LOG("StageTint: ready");
}

ITickListener* StageTint::Listener()
{
	return &g_ticker;
}

bool StageTint::IsAvailable()
{
	return g_currentStage != 0;
}

const char* StageTint::StatusText()
{
	return g_status;
}
