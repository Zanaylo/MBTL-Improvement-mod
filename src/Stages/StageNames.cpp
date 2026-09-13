#include "Stages/StageNames.h"

#include "Stages/StageArchive.h"

namespace {

struct Named
{
	int number;
	const char* name;
};

const Named kUni2[] = {
	{ 1, "Metropolitan Center: Intersection" },
	{ 2, "High-Rise Building: Rooftop" },
	{ 3, "Quiet Park" },
	{ 4, "Riverside Area" },
	{ 5, "Lower Freeway" },
	{ 6, "Beneath the Amber Overpass" },
	{ 7, "Bright Commercial District" },
	{ 8, "Shrine of the Protector" },
	{ 9, "Altar of Light and Dark" },
	{ 10, "Crimson Parking Lot" },
	{ 11, "Abandoned Building" },
	{ 12, "Rumbling Factory" },
	{ 13, "Sleepy Fountain Plaza" },
	{ 14, "Bloom-Strewn Former School" },
	{ 15, "Cinema Cafeteria" },
	{ 16, "In Front of Momiji Alley" },
	{ 17, "East Kanzakai: Playground" },
	{ 18, "Cathedral of the Far East" },
	{ 19, "Museum of Light and Water" },
	{ 20, "Moonlit Observatory" },
	{ 21, "Evening School Grounds: Main Entrance" },
	{ 22, "The Abyss: Altar of Aeon" },
	{ 23, "Night Pool of Glistening Haze" },
	{ 24, "Forest of Amnesia - Desolate Sacred Ground" },
	{ 25, "North Metropolis - Central Street" },
	{ 26, "Deep Azure Aquarium" },
	{ 27, "Tower in the Dark Night - Under Construction" },
	{ 90, "Training Stage" },
	{ 99, "Debug Stage" },
};

constexpr int kLastUniStage = 19;

const Named kMbtl[] = {
	{ 1, "Tohno Mansion" },
	{ 2, "School" },
	{ 3, "Park" },
	{ 4, "Fountain" },
	{ 5, "Back Alley" },
	{ 6, "Catacombs" },
	{ 7, "Danger Zone B" },
	{ 8, "Tohno Mansion Grounds" },
	{ 9, "Danger Zone A" },
	{ 10, "Tohno Mansion Grounds, Night" },
	{ 11, "Tohno Mansion Interior, Day" },
	{ 12, "School at Dusk" },
	{ 13, "Park, Day" },
	{ 14, "Catacombs (Boss Rush)" },
	{ 15, "Grassland, Day" },
	{ 16, "Grassland, Night" },
	{ 17, "Grassland, Night (Star Trails)" },
	{ 18, "Church" },
	{ 19, "Armed Ciel's Stage" },
	{ 20, "Rooftop" },
	{ 21, "Back Alley, Night" },
	{ 22, "Souya Station (Half Moon)" },
	{ 23, "Souya Station (Full Moon)" },
	{ 24, "Great Cats Village / Lumina" },
	{ 25, "Park at Dusk" },
	{ 26, "Fountain at Dusk" },
	{ 27, "Rooftop, Day" },
	{ 28, "Dan-no-ura" },
	{ 29, "Prison Tower" },
	{ 30, "Colosseum" },
	{ 31, "Ciel's Room" },
	{ 32, "Dojo" },
	{ 33, "Miyako's Room" },
	{ 34, "Waiting Room" },
	{ 35, "Tohno Mansion, Night (Time Stopped)" },
	{ 90, "Training Stage" },
	{ 99, "Debug Stage" },
};

template <size_t N>
const char* Find(const Named (&table)[N], int number)
{
	for (const Named& named : table)
	{
		if (named.number == number)
			return named.name;
	}

	return nullptr;
}

bool UniShares(int number)
{
	return (number > 0 && number <= kLastUniStage) || number == 90;
}

}

std::string StageNames::English(FbGameFolder::Game game, const std::string& folder)
{
	const int number = StageArchive::NumberOf(folder);
	const bool known = game == FbGameFolder::Game_UNI2 || (game == FbGameFolder::Game_UNI && UniShares(number));
	const char* const name = known ? Find(kUni2, number) : nullptr;

	return name == nullptr ? std::string() : name;
}

const char* StageNames::Mbtl(int number)
{
	return Find(kMbtl, number);
}
