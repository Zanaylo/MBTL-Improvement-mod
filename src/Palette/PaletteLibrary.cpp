#include "Palette/PaletteLibrary.h"

#include "Core/utils.h"
#include "Palette/Characters.h"
#include "Palette/PaletteFile.h"

#include <windows.h>

#include <algorithm>

namespace {

constexpr int kShelves = 2;

struct Shelf
{
	int chara = -1;
	int count = 0;
	std::string names[PaletteLibrary::kMaxFiles];
};

Shelf g_shelves[kShelves];
int g_next = 0;

void Fill(Shelf& shelf, int chara)
{
	shelf.chara = chara;
	shelf.count = 0;

	if (chara < 0)
		return;

	const std::string folder = PaletteLibrary::FolderFor(chara);
	CreateDirectoryTree(folder);

	WIN32_FIND_DATAA found = {};
	const HANDLE search = FindFirstFileA((folder + "\\*" + PaletteFile::kExtension).c_str(), &found);

	if (search == INVALID_HANDLE_VALUE)
		return;

	do
	{
		if ((found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 && shelf.count < PaletteLibrary::kMaxFiles)
			shelf.names[shelf.count++] = found.cFileName;
	} while (FindNextFileA(search, &found));

	FindClose(search);
	std::sort(shelf.names, shelf.names + shelf.count);
}

Shelf& ShelfFor(int chara)
{
	for (Shelf& shelf : g_shelves)
	{
		if (shelf.chara == chara)
			return shelf;
	}

	Shelf& fresh = g_shelves[g_next];
	g_next = (g_next + 1) % kShelves;

	Fill(fresh, chara);
	return fresh;
}

}

std::string PaletteLibrary::FolderFor(int chara)
{
	return GetModRootPath("Palettes\\") + Characters::Name(chara);
}

void PaletteLibrary::Rescan(int chara)
{
	Fill(ShelfFor(chara), chara);
}

int PaletteLibrary::GetCount(int chara)
{
	return chara < 0 ? 0 : ShelfFor(chara).count;
}

const char* PaletteLibrary::GetName(int chara, int index)
{
	if (chara < 0 || index < 0)
		return "";

	const Shelf& shelf = ShelfFor(chara);
	return index < shelf.count ? shelf.names[index].c_str() : "";
}
