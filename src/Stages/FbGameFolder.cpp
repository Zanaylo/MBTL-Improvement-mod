#include "Stages/FbGameFolder.h"

#include <windows.h>

#include <string>

namespace {

bool Exists(const std::string& folder, const char* name)
{
	std::string path = folder;

	if (!path.empty() && path.back() != '\\' && path.back() != '/')
		path.push_back('\\');

	path += name;

	return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

}

FbGameFolder::Game FbGameFolder::Detect(const char* folder)
{
	if (folder == nullptr || folder[0] == '\0')
		return Game_None;

	const std::string root = folder;
	const bool archive = Exists(root, "d");

	if (archive && Exists(root, "uni2.exe"))
		return Game_UNI2;

	if (archive && (Exists(root, "UNIst.exe") || Exists(root, "UNIclr.exe")))
		return Game_UNI;

	if (Exists(root, "UNIEL.exe"))
		return Game_UNIEL;

	if (Exists(root, "Bgm\\bgm.txt") && (Exists(root, "RingGame.exe") || Exists(root, "eboot.bin")))
		return Game_DFCI;

	return Game_None;
}

const char* FbGameFolder::Name(Game game)
{
	switch (game)
	{
	case Game_UNI2:
		return "UNDER NIGHT IN-BIRTH II Sys:Celes";
	case Game_UNI:
		return "UNDER NIGHT IN-BIRTH Exe:Late[st] / [cl-r]";
	case Game_UNIEL:
		return "UNDER NIGHT IN-BIRTH Exe:Late";
	case Game_DFCI:
		return "DENGEKI BUNKO FIGHTING CLIMAX IGNITION";
	default:
		return "nothing the mod can take stages from";
	}
}

const char* FbGameFolder::Tag(Game game)
{
	switch (game)
	{
	case Game_UNI2:
		return " (UNI2)";
	case Game_UNI:
		return " (UNICLR)";
	case Game_UNIEL:
		return " (UNIEL)";
	case Game_DFCI:
		return " (DFCI)";
	default:
		return "";
	}
}
