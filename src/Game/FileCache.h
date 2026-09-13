#pragma once

#include <windows.h>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class FileCache
{
public:
	const std::vector<uint8_t>* Get(const std::string& key, const std::string& path);
	const std::vector<uint8_t>* Keep(const std::string& key, uint64_t stamp, std::vector<uint8_t>&& data);
	const std::vector<uint8_t>* Find(const std::string& key, uint64_t stamp) const;

	static uint64_t StampOf(const std::string& path);

private:
	struct Held
	{
		uint64_t stamp;
		std::shared_ptr<std::vector<uint8_t>> data;
	};

	mutable SRWLOCK m_lock = SRWLOCK_INIT;
	std::unordered_map<std::string, Held> m_files;
	std::vector<std::shared_ptr<std::vector<uint8_t>>> m_retired;
};
