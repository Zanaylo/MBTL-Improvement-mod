#include "Game/FileCache.h"

#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Core/utils.h"

uint64_t FileCache::StampOf(const std::string& path)
{
	WIN32_FILE_ATTRIBUTE_DATA info = {};

	if (!GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &info))
		return 0;

	const uint64_t written = (static_cast<uint64_t>(info.ftLastWriteTime.dwHighDateTime) << 32) |
		info.ftLastWriteTime.dwLowDateTime;
	const uint64_t size = (static_cast<uint64_t>(info.nFileSizeHigh) << 32) | info.nFileSizeLow;

	return written ^ (size * 0x9E3779B97F4A7C15ull);
}

const std::vector<uint8_t>* FileCache::Find(const std::string& key, uint64_t stamp) const
{
	AcquireSRWLockShared(&m_lock);

	const auto cached = m_files.find(key);
	const std::vector<uint8_t>* const hit = cached != m_files.end() && cached->second.stamp == stamp
		? cached->second.data.get() : nullptr;

	ReleaseSRWLockShared(&m_lock);
	return hit;
}

const std::vector<uint8_t>* FileCache::Keep(const std::string& key, uint64_t stamp, std::vector<uint8_t>&& data)
{
	auto held = std::make_shared<std::vector<uint8_t>>(std::move(data));

	AcquireSRWLockExclusive(&m_lock);

	Held& slot = m_files[key];

	if (slot.data)
		m_retired.push_back(slot.data);

	slot.stamp = stamp;
	slot.data = held;

	ReleaseSRWLockExclusive(&m_lock);
	return held.get();
}

const std::vector<uint8_t>* FileCache::Get(const std::string& key, const std::string& path)
{
	const uint64_t stamp = StampOf(path);
	const std::vector<uint8_t>* const hit = Find(key, stamp);

	if (hit)
		return hit;

	std::vector<uint8_t> data;

	if (!ReadWholeFile(path, data))
	{
		LOG("Could not read %s", path.c_str());
		return nullptr;
	}

	if (g_settings.logServedFiles)
		LOG("Serving %s (%u bytes)", key.c_str(), static_cast<unsigned>(data.size()));

	return Keep(key, stamp, std::move(data));
}
