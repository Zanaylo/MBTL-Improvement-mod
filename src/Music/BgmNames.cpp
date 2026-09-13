#include "Music/BgmNames.h"

#include "Music/BgmTable.h"

#include <cstdio>

namespace {

constexpr int kFirstStage = 1;
constexpr int kLastStage = 35;

struct Scene
{
	int id;
	const char* name;
};

constexpr Scene kScenes[] = {
	{ 40, "Main menu" },
	{ 41, "Map / replay" },
	{ 45, "Customize" },
	{ 46, "Options / gallery" },
	{ 82, "Continue" },
	{ 83, "Game over" },
	{ 84, "VS demo" },
	{ 90, "Training stage" },
	{ 91, "Training stage outside training" },
	{ 98, "Win demo" },
	{ 99, "Character select" },
};

}

const char* BgmNames::SceneOf(int id)
{
	for (const Scene& scene : kScenes)
	{
		if (scene.id == id)
			return scene.name;
	}

	return nullptr;
}

void BgmNames::Label(int id, const char* file, char* out, size_t size)
{
	const char* const scene = SceneOf(id);

	if (scene != nullptr)
	{
		sprintf_s(out, size, "%s", scene);
		return;
	}

	if (id >= kFirstStage && id <= kLastStage)
	{
		sprintf_s(out, size, "Stage %d theme", id);
		return;
	}

	if (file != nullptr && file[0] != '\0')
	{
		sprintf_s(out, size, "%s", file);
		return;
	}

	sprintf_s(out, size, "Track %03d", id);
}

void BgmNames::Describe(int id, char* out, size_t size)
{
	BgmTable::Slot slot = {};
	const bool known = BgmTable::Read(id, slot) && slot.present;

	Label(id, known ? slot.file : nullptr, out, size);
}
