#pragma once

#include <cstdint>

namespace Commands
{
	enum Kind
	{
		Kind_None,
		Kind_Dash,
		Kind_AirMovement,
		Kind_Jump,
		Kind_Action,
	};

	Kind KindOf(uint32_t command);
	const char* NameOf(uint32_t command);
}
