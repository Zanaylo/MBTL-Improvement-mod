#include "Stages/StageReplacements.h"

#include "Core/utils.h"
#include "Stages/GameStages.h"
#include "Stages/StageArchive.h"
#include "Stages/StageLibrary.h"

#include <cstdint>

void StageReplacements::Snapshot(std::vector<Replacement>& out)
{
	out.clear();

	std::vector<GameStages::Own> own;
	GameStages::Snapshot(own);

	for (const GameStages::Own& stage : own)
	{
		std::vector<uint8_t> blob;

		if (stage.number == StageLibrary::kRandomStage || !ReadWholeFile(StageLibrary::NoteOf(stage.number), blob))
			continue;

		const std::string note(blob.begin(), blob.end());
		std::string name;
		const bool renamed = StageArchive::Field(note, "Name", name);

		out.push_back({ stage.number, renamed, renamed ? StageArchive::Unquoted(name) : stage.name });
	}
}
