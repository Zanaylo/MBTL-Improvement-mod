#include "Training/Commands.h"

namespace {

struct Entry
{
	uint32_t first;
	uint32_t last;
	Commands::Kind kind;
	const char* name;
};

constexpr Entry kEntries[] = {
	{ 90, 90, Commands::Kind_Dash, "Forward Dash" },
	{ 91, 91, Commands::Kind_Dash, "Back Dash" },
	{ 400, 400, Commands::Kind_Dash, "Forward Dash" },
	{ 401, 401, Commands::Kind_Dash, "Back Dash" },
	{ 410, 410, Commands::Kind_Dash, "Forward Dash" },
	{ 411, 411, Commands::Kind_Dash, "Back Dash" },
	{ 397, 397, Commands::Kind_AirMovement, "Air Dash" },
	{ 398, 398, Commands::Kind_AirMovement, "Air Back Dash" },
	{ 412, 412, Commands::Kind_AirMovement, "Air Dash" },
	{ 413, 413, Commands::Kind_AirMovement, "Air Back Dash" },
	{ 774, 777, Commands::Kind_Jump, "Double Jump" },
	{ 779, 782, Commands::Kind_Jump, "Double Jump Cancel" },
	{ 2000, 2002, Commands::Kind_Jump, "High Jump" },
	{ 2010, 2012, Commands::Kind_Jump, "Jump" },
	{ 2543, 2545, Commands::Kind_Jump, "High Jump Cancel" },
	{ 2553, 2555, Commands::Kind_Jump, "Jump Cancel" },
	{ 84, 84, Commands::Kind_Action, "Veil Off" },
	{ 122, 122, Commands::Kind_Action, "Moon Drive" },
	{ 306, 307, Commands::Kind_Action, "Throw" },
	{ 681, 683, Commands::Kind_Action, "Shield" },
	{ 691, 693, Commands::Kind_Action, "Shield Cancel" },
};

const Entry* Find(uint32_t command)
{
	for (const Entry& entry : kEntries)
	{
		if (command >= entry.first && command <= entry.last)
			return &entry;
	}

	return nullptr;
}

}

Commands::Kind Commands::KindOf(uint32_t command)
{
	const Entry* const entry = Find(command);
	return entry != nullptr ? entry->kind : Kind_None;
}

const char* Commands::NameOf(uint32_t command)
{
	const Entry* const entry = Find(command);
	return entry != nullptr ? entry->name : "";
}
