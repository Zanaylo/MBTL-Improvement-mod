#include "Stages/StageCardArt.h"

#include "Core/logger.h"
#include "Core/utils.h"

#include <cstring>
#include <vector>

namespace {

constexpr const char* kRoot = "Cards";
constexpr const char* kPictures[] = { "thumbnail.png", "vs_background.png", "menu_background.png" };

std::string WithoutTag(const std::string& name, const char* tag)
{
	const size_t length = std::strlen(tag);

	if (tag[0] == '\0' || name.size() <= length || _stricmp(name.c_str() + name.size() - length, tag) != 0)
		return std::string();

	size_t end = name.size() - length;

	while (end != 0 && name[end - 1] == ' ')
		--end;

	return name.substr(0, end);
}

std::string Ascii(const std::string& name)
{
	std::string out;
	bool space = false;

	for (char c : name)
	{
		if (static_cast<unsigned char>(c) >= 0x80)
			continue;

		if (c == ' ' && space)
			continue;

		space = c == ' ';
		out += c;
	}

	const size_t last = out.find_last_not_of(' ');

	return last == std::string::npos ? std::string() : out.substr(0, last + 1);
}

int CopyFrom(const std::string& folder, const std::string& target)
{
	int copied = 0;

	for (const char* picture : kPictures)
	{
		std::vector<uint8_t> data;

		if (!ReadWholeFile(folder + "\\" + picture, data) || data.empty())
			continue;

		copied += WriteWholeFile(target + "\\" + picture, data.data(), data.size()) ? 1 : 0;
	}

	return copied;
}

}

int StageCardArt::Apply(FbGameFolder::Game game, const std::string& name, const std::string& target)
{
	const std::string root = GetModRootPath(kRoot) + "\\" + FbGameFolder::Short(game) + "\\";
	const std::string plain = WithoutTag(name, FbGameFolder::Tag(game));

	const std::string chosen = plain.empty() ? name : plain;
	const std::string candidates[] = { chosen, Ascii(chosen), name };

	for (const std::string& candidate : candidates)
	{
		const int copied = candidate.empty() ? 0 : CopyFrom(root + candidate, target);

		if (copied == 0)
			continue;

		LOG("StageCardArt: %s uses %d card picture(s) from %s", name.c_str(), copied, (root + candidate).c_str());
		return copied;
	}

	return 0;
}
