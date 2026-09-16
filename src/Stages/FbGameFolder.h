#pragma once

namespace FbGameFolder
{
	enum Game
	{
		Game_None,
		Game_UNI2,
		Game_UNI,
		Game_UNIEL,
		Game_DFCI,
	};

	Game Detect(const char* folder);

	const char* Name(Game game);
	const char* Tag(Game game);
	const char* Short(Game game);
}
