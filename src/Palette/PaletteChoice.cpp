#include "Palette/PaletteChoice.h"

#include "Core/Settings.h"
#include "Core/logger.h"
#include "Palette/Characters.h"
#include "Palette/EffectPaint.h"
#include "Palette/PaletteControl.h"
#include "Palette/PaletteFile.h"
#include "Palette/PaletteLibrary.h"
#include "Palette/PaletteOwner.h"
#include "Palette/PalettePaint.h"

#include <windows.h>

#include <cstring>
#include <memory>

namespace {

constexpr const char* kSection = "PaletteChoice";
constexpr size_t kFileLength = PaletteFile::kNameLength + 8;

char g_remembered[kFileLength] = {};

struct Seat
{
	int chara = -1;
	bool tried = false;
	bool dressedHere = false;
	unsigned generation = 0;
	char worn[kFileLength] = {};
};

Seat g_seats[PaletteChoice::kSeats] = {};

bool Valid(int seat)
{
	return seat >= 0 && seat < PaletteChoice::kSeats;
}

int WornIndex(int seat, int chara)
{
	const char* const worn = PaletteChoice::WornFile(seat);

	if (worn[0] == '\0')
		return -1;

	for (int i = 0; i < PaletteLibrary::GetCount(chara); ++i)
	{
		if (strcmp(PaletteLibrary::GetName(chara, i), worn) == 0)
			return i;
	}

	return -1;
}

void Dress(int seat, int chara)
{
	char file[kFileLength] = {};
	strncpy_s(file, PaletteChoice::Remembered(chara), _TRUNCATE);

	if (file[0] == '\0')
		return;

	if (!PaletteChoice::Apply(seat, chara, file))
	{
		LOG("palettes: %s's remembered '%s' could not be read", Characters::Name(chara), file);
		PaletteChoice::Forget(chara);
		return;
	}

	++g_seats[seat].generation;
	g_seats[seat].dressedHere = true;
	LOG("palettes: %s on p%d is wearing '%s' again", Characters::Name(chara), PaletteOwner::SideOf(seat) + 1, file);
}

void Undress(int seat, int chara)
{
	Seat& entry = g_seats[seat];

	entry.chara = chara;
	entry.tried = false;
	entry.dressedHere = false;

	PalettePaint::Clear(seat);
	EffectPaint::Clear(seat);
	PaletteChoice::NoteBare(seat);

	++entry.generation;
}

void Follow(int seat)
{
	Seat& entry = g_seats[seat];
	const int chara = PaletteOwner::CharaNumber(seat);

	if (chara < 0)
	{
		entry.tried = false;
		return;
	}

	if (chara != entry.chara)
		Undress(seat, chara);

	const bool mine = PaletteControl::CanEdit(seat);

	if (entry.dressedHere && !mine)
	{
		LOG("palettes: p%d turned out not to be ours, so what this machine put on came off",
			PaletteOwner::SideOf(seat) + 1);
		Undress(seat, chara);
		return;
	}

	if (entry.tried || !mine)
		return;

	entry.tried = true;

	if (!PalettePaint::IsStaged(seat))
		Dress(seat, chara);
}

}

const char* PaletteChoice::Remembered(int chara)
{
	g_remembered[0] = '\0';

	if (chara < 0)
		return g_remembered;

	GetPrivateProfileStringA(kSection, Characters::Name(chara), "", g_remembered, sizeof(g_remembered),
		Settings::IniPath().c_str());

	return g_remembered;
}

void PaletteChoice::Remember(int chara, const char* file)
{
	if (chara < 0 || file == nullptr || file[0] == '\0')
		return;

	Settings::SaveString(kSection, Characters::Name(chara), file);
}

void PaletteChoice::Forget(int chara)
{
	if (chara >= 0)
		Settings::SaveString(kSection, Characters::Name(chara), "");
}

bool PaletteChoice::Apply(int seat, int chara, const char* file)
{
	if (!Valid(seat) || chara < 0 || file == nullptr || file[0] == '\0')
		return false;

	const std::unique_ptr<PaletteFile::Content> content = std::make_unique<PaletteFile::Content>();

	if (!PaletteFile::Load(PaletteLibrary::FolderFor(chara) + "\\" + file, *content))
		return false;

	PalettePaint::Clear(seat);

	for (int sub = 0; sub < PaletteFile::kSubPalettes; ++sub)
	{
		if ((content->subMask & (1u << sub)) != 0)
			PalettePaint::Stage(seat, sub, content->pages[sub]);
	}

	EffectPaint::SetBlock(seat, content->hasEffects ? content->effects : nullptr);

	NoteWorn(seat, file);
	return true;
}

bool PaletteChoice::Wear(int seat, const char* file)
{
	if (!Valid(seat))
		return false;

	const int chara = PaletteOwner::CharaNumber(seat);

	if (!Apply(seat, chara, file))
		return false;

	Remember(chara, file);

	g_seats[seat].chara = chara;
	g_seats[seat].tried = true;
	++g_seats[seat].generation;

	return true;
}

void PaletteChoice::Bare(int seat)
{
	if (!Valid(seat))
		return;

	PalettePaint::Clear(seat);
	EffectPaint::Clear(seat);

	Forget(PaletteOwner::CharaNumber(seat));
	NoteBare(seat);

	g_seats[seat].tried = true;
	++g_seats[seat].generation;
}

bool PaletteChoice::Step(int seat, int steps)
{
	if (!Valid(seat) || steps == 0 || !PaletteControl::CanEdit(seat))
		return false;

	const int chara = PaletteOwner::CharaNumber(seat);
	const int count = chara >= 0 ? PaletteLibrary::GetCount(chara) : 0;

	if (count <= 0)
		return false;

	const int slots = count + 1;
	int target = (WornIndex(seat, chara) + 1 + steps) % slots;

	if (target < 0)
		target += slots;

	if (target > 0)
		return Wear(seat, PaletteLibrary::GetName(chara, target - 1));

	Bare(seat);
	return true;
}

int PaletteChoice::LocalPlayer()
{
	const int local = PaletteControl::LocalPlayer();
	return local >= 0 ? local : 0;
}

const char* PaletteChoice::WornFile(int seat)
{
	return Valid(seat) ? g_seats[seat].worn : "";
}

void PaletteChoice::NoteWorn(int seat, const char* file)
{
	if (Valid(seat))
		strncpy_s(g_seats[seat].worn, file != nullptr ? file : "", _TRUNCATE);
}

void PaletteChoice::NoteBare(int seat)
{
	if (Valid(seat))
		g_seats[seat].worn[0] = '\0';
}

unsigned PaletteChoice::GetGeneration(int seat)
{
	return Valid(seat) ? g_seats[seat].generation : 0;
}

void PaletteChoice::OnFrame()
{
	for (int seat = 0; seat < kSeats; ++seat)
		Follow(seat);
}
