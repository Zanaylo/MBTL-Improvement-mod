#pragma once

#include <string>
#include <vector>

namespace BgListText
{
	struct Span
	{
		int number;
		size_t start;
		size_t open;
		size_t end;
	};

	void Blocks(const std::string& list, std::vector<Span>& out);

	bool SelectList(const std::string& list, std::vector<int>& numbers, size_t& valueAt, size_t& valueEnd);
	std::string JoinList(const std::vector<int>& numbers);

	void SetValue(std::string& block, const char* key, const std::string& value);
	void RemoveValue(std::string& block, const char* key);

	std::string Newline(const std::string& text);
}
