#include "Music/BgmTextOverlay.h"

#include "Core/logger.h"
#include "Music/UserTracks.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace {

constexpr const char* kKey = "bgm\\bgm.txt";

bool Holds(const std::vector<uint8_t>& content, const char* text)
{
	const size_t length = std::strlen(text);
	return std::search(content.begin(), content.end(), text, text + length) != content.end();
}

void Append(std::vector<uint8_t>& content, const char* text)
{
	content.insert(content.end(), text, text + std::strlen(text));
}

}

bool BgmTextOverlay::Covers(const std::string& key) const
{
	return key == kKey;
}

bool BgmTextOverlay::Apply(const std::string&, std::vector<uint8_t>& content) const
{
	if (content.empty())
		return false;

	const std::vector<UserTracks::Track> tracks = UserTracks::Snapshot();
	bool changed = false;

	for (const UserTracks::Track& track : tracks)
	{
		char header[16] = {};
		sprintf_s(header, "[BGM_%03d]", track.id);

		if (Holds(content, header))
		{
			LOG("BgmTextOverlay: bgm.txt already has %s, so %s is not added under that number", header,
				track.file.c_str());
			continue;
		}

		char section[160] = {};
		sprintf_s(section, "\r\n%s\r\nFile = %s\r\nIsLoop = %d\r\nLoopPos = %.3f\r\n", header, track.file.c_str(),
			track.loop ? 1 : 0, track.loopPosition);

		Append(content, section);
		changed = true;
	}

	return changed;
}

uint32_t BgmTextOverlay::Version() const
{
	return UserTracks::Version();
}
