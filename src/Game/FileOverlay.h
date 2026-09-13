#pragma once

#include <cstdint>
#include <string>
#include <vector>

class IFileOverlay
{
public:
	virtual ~IFileOverlay() = default;

	virtual bool Covers(const std::string& key) const = 0;
	virtual std::string BasePath(const std::string&, const char* requested) const { return requested; }
	virtual bool Apply(const std::string& key, std::vector<uint8_t>& content) const = 0;
	virtual uint32_t Version() const = 0;
};
