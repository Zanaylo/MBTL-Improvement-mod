#include "Palette/Characters.h"

#include <cstdio>

namespace {

constexpr const char* kNames[Characters::kCount] = {
	"Arcueid",
	"Hisui",
	"Akiha",
	"Shiki",
	"Kohaku",
	"Roa",
	"Kouma",
	"Hisui & Kohaku",
	"Noel",
	"Vlov",
	"Red Arcueid",
	"Ciel",
	"Saber",
	"Miyako",
	"Dead Apostle Noel",
	"Aoko",
	"Powered Ciel",
	"Mario",
	"Sister",
	"Neco-Arc",
	"Mash",
	"Ushiwakamaru",
	"Edmond Dantes",
};

}

const char* Characters::Name(int chara)
{
	static char fallback[16] = {};

	if (chara >= 0 && chara < kCount && kNames[chara] != nullptr)
		return kNames[chara];

	sprintf_s(fallback, "chr%03d", chara);
	return fallback;
}
