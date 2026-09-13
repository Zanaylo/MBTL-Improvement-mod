#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace StageArchive
{
	struct Stage
	{
		std::string folder;
		std::string name;
		uint32_t bytes;
	};

	class Source
	{
	public:
		virtual ~Source() = default;

		virtual void Stages(std::vector<Stage>& out) = 0;
		virtual void Files(const std::string& stage, std::vector<std::string>& out) = 0;
		virtual bool Read(const std::string& stage, const std::string& file, std::vector<uint8_t>& out) = 0;
		virtual bool BgList(std::string& out) = 0;
	};

	std::unique_ptr<Source> Open(const char* folder);

	bool MagicOk(const std::string& file, const std::vector<uint8_t>& data);

	int NumberOf(const std::string& stage);

	size_t MatchPair(const std::string& text, size_t open);

	bool FieldSpan(const std::string& block, const char* key, size_t& keyAt, size_t& valueAt, size_t& valueEnd);

	bool Block(const std::string& bgList, const std::string& stage, std::string& out);
	bool Field(const std::string& block, const char* key, std::string& out);

	std::string Unquoted(const std::string& value);
}
