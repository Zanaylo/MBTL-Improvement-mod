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

#include <algorithm>
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
	uint8_t store = 0;

	for (size_t i = 0; i + 2 <= length && i < Tint::kEntryWindow; ++i)
	{
		if (loader[i] != Tint::kMoveFromEcx || (loader[i + 1] & Tint::kFromEcxMask) != Tint::kFromEcxBase)
			continue;

		store = static_cast<uint8_t>((((loader[i + 1] >> 3) & 7) << 3) | Tint::kStoreGlobalModRm);
		break;
	}

	if (store == 0)
		return 0;

	std::vector<uintptr_t> globals;

	for (size_t i = 0; i + 2 + sizeof(uint32_t) <= length; ++i)
	{
		if (loader[i] != Tint::kStoreGlobal || loader[i + 1] != store)
			continue;

		const uintptr_t global = ImageScanner::ReadDword(loader + i + 2);

		if (ImageScanner::InData(global) && std::find(globals.begin(), globals.end(), global) == globals.end())
			globals.push_back(global);
	}

	return globals.size() == 1 ? globals.front() : 0;
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
