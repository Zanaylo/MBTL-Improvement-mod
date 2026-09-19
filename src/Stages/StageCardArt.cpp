#include "Stages/StageCardArt.h"

#include "Core/logger.h"
#include "Core/utils.h"

#include <windows.h>

#include <cctype>
#include <cstring>

namespace {

constexpr const char* kType = "STAGECARD";
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

std::string ResourceName(const std::string& path)
{
	std::string out;

	for (char c : path)
	{
		const unsigned char byte = static_cast<unsigned char>(c);
		out += byte < 0x80 && std::isalnum(byte) ? static_cast<char>(std::toupper(byte)) : '_';
	}

	return out;
}

bool Embedded(const std::string& path, const uint8_t*& outData, DWORD& outSize)
{
	const std::string name = ResourceName(path);

	const HMODULE module = GetModModuleHandle();
	const HRSRC found = module != nullptr ? FindResourceA(module, name.c_str(), kType) : nullptr;

	if (found == nullptr)
		return false;

	const HGLOBAL loaded = LoadResource(module, found);
	outData = loaded != nullptr ? static_cast<const uint8_t*>(LockResource(loaded)) : nullptr;
	outSize = SizeofResource(module, found);

	return outData != nullptr && outSize != 0;
}

int CopyFrom(const std::string& prefix, const std::string& target)
{
	int copied = 0;

	for (const char* picture : kPictures)
	{
		const uint8_t* data = nullptr;
		DWORD size = 0;

		if (!Embedded(prefix + picture, data, size))
			continue;

		copied += WriteWholeFile(target + "\\" + picture, data, size) ? 1 : 0;
	}

	return copied;
}

}

int StageCardArt::Apply(FbGameFolder::Game game, const std::string& name, const std::string& target)
{
	const std::string plain = WithoutTag(name, FbGameFolder::Tag(game));
	const std::string chosen = plain.empty() ? name : plain;
	const std::string candidates[] = { chosen, Ascii(chosen), name };

	for (const std::string& candidate : candidates)
	{
		const std::string prefix = std::string(FbGameFolder::Short(game)) + "/" + candidate + "/";
		const int copied = candidate.empty() ? 0 : CopyFrom(prefix, target);

		if (copied == 0)
			continue;

		LOG("StageCardArt: %s uses %d built-in card picture(s) for %s", name.c_str(), copied, candidate.c_str());
		return copied;
	}

	return 0;
}
