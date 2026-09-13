#pragma once

#include "Stages/FbGameFolder.h"

#include <string>

namespace StageNames
{
	std::string English(FbGameFolder::Game game, const std::string& folder);

	const char* Mbtl(int number);
}
