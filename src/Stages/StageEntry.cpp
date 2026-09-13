#include "Stages/StageEntry.h"

#include "Core/TextEncoding.h"
#include "Stages/BgListText.h"
#include "Stages/FbGameFolder.h"
#include "Stages/StageArchive.h"

#include <algorithm>
#include <cstdio>
#include <iterator>

namespace {

constexpr size_t kNameBytes = 62;
constexpr const char* kZero = "0.0";
constexpr const char* kFogOff = "0";

const char* const kTakenFields[] = {
	"Scale", "Position", "FOV", "VanishingPoint", "IsFog", "FogStart", "FogEnd", "FogColor", "MSAA",
	"IsBloom", "BGBloomEnable", "BGBloomBlightness", "BGBloomPower", "BGBloomBiassR", "BGBloomBiassG",
	"BGBloomBiassB", "BGBloomBlurRadius", "BGBloomTextureSize", "BGBloomAlpha",
	"BGTinyFXAAEnable", "BGTinyFXAAThreshold", "BGTinyFXAALerpT", "StageW", "TargetW", "TargetH",
};

const char* const kZeroedFields[] = { "ViewRotationX", "ViewTranslationY" };

bool CarriesAny(const std::string& source)
{
	std::string value;

	return std::any_of(std::begin(kTakenFields), std::end(kTakenFields),
		[&source, &value](const char* key) { return StageArchive::Field(source, key, value); });
}

void TakeFields(std::string& block, const std::string& source)
{
	if (!CarriesAny(source))
		return;

	for (const char* key : kTakenFields)
	{
		std::string value;

		if (StageArchive::Field(source, key, value))
			BgListText::SetValue(block, key, value);
		else
			BgListText::RemoveValue(block, key);
	}
}

std::string Retagged(const std::string& templateBlock, int number)
{
	char tag[16] = {};
	sprintf_s(tag, "Bg_%03d", number);

	const size_t digits = templateBlock.find_first_not_of("Bg_0123456789");

	if (templateBlock.compare(0, 3, "Bg_") != 0 || digits == std::string::npos)
		return templateBlock;

	return tag + templateBlock.substr(digits);
}

bool FromGame(const std::string& source, FbGameFolder::Game game)
{
	std::string from;

	return StageArchive::Field(source, "From", from) && StageArchive::Unquoted(from) == FbGameFolder::Name(game);
}

}

std::string StageEntry::Compose(const std::string& templateBlock, const std::string& source, int number,
	const std::string& shiftJisName, int card)
{
	std::string block = Retagged(templateBlock, number);

	char dataFile[24] = {};
	sprintf_s(dataFile, "\"bg%03d\"", number);

	BgListText::SetValue(block, "Name", "\"" + shiftJisName + "\"");
	BgListText::SetValue(block, "DataFile", dataFile);

	if (card >= 0)
		BgListText::SetValue(block, "StageSelTex", std::to_string(card));

	TakeFields(block, source);

	if (FromGame(source, FbGameFolder::Game_DFCI))
		BgListText::SetValue(block, "IsFog", kFogOff);

	for (const char* key : kZeroedFields)
		BgListText::SetValue(block, key, kZero);

	return block;
}

std::string StageEntry::ShiftJis(const std::string& utf8)
{
	std::string out;

	if (!TextEncoding::Utf8ToShiftJis(utf8, out))
		out = utf8;

	out.erase(std::remove_if(out.begin(), out.end(), [](char c) { return c == '"' || c == '\r' || c == '\n'; }),
		out.end());
	out.erase(TextEncoding::ShiftJisBoundary(out, kNameBytes));

	return out;
}
