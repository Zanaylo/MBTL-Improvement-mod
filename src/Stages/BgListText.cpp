#include "Stages/BgListText.h"

#include "Stages/StageArchive.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

constexpr const char* kMark = "Bg_";
constexpr const char* kSelectList = "BgSelectList";
constexpr const char* kAnchorKey = "StageSelTex";
constexpr const char* kIndent = "\t\t";

size_t Skip(const std::string& text, size_t at, const char* of)
{
	while (at < text.size() && strchr(of, text[at]) != nullptr)
		++at;

	return at;
}

size_t LineStart(const std::string& text, size_t at)
{
	const size_t newline = at == 0 ? std::string::npos : text.rfind('\n', at - 1);

	return newline == std::string::npos ? 0 : newline + 1;
}

bool BlankBetween(const std::string& text, size_t from, size_t to)
{
	return Skip(text, from, " \t") >= to;
}

bool RestOfLineIsTail(const std::string& text, size_t at)
{
	const size_t after = Skip(text, Skip(text, at, " \t,"), " \t");

	return after >= text.size() || text[after] == '\r' || text[after] == '\n' ||
		text.compare(after, 2, "//") == 0;
}

size_t LineEnd(const std::string& text, size_t at)
{
	const size_t newline = text.find('\n', at);

	return newline == std::string::npos ? text.size() : newline + 1;
}

void Insert(std::string& block, const char* key, const std::string& value)
{
	const std::string newline = BgListText::Newline(block);
	size_t keyAt = 0;
	size_t valueAt = 0;
	size_t valueEnd = 0;

	if (StageArchive::FieldSpan(block, kAnchorKey, keyAt, valueAt, valueEnd))
	{
		const size_t line = LineStart(block, keyAt);
		const std::string indent = block.substr(line, keyAt - line);

		block.insert(line, indent + key + " = " + value + "," + newline);
		return;
	}

	const size_t close = block.rfind('}');
	const size_t at = close == std::string::npos ? block.size() : LineStart(block, close);

	block.insert(at, std::string(kIndent) + key + " = " + value + "," + newline);
}

}

void BgListText::Blocks(const std::string& list, std::vector<Span>& out)
{
	out.clear();

	const size_t length = strlen(kMark);

	for (size_t at = list.find(kMark); at != std::string::npos; at = list.find(kMark, at + length))
	{
		size_t digits = at + length;

		while (digits < list.size() && isdigit(static_cast<unsigned char>(list[digits])) != 0)
			++digits;

		if (digits == at + length || (at > 0 && (isalnum(static_cast<unsigned char>(list[at - 1])) != 0)))
			continue;

		const size_t equals = Skip(list, digits, " \t");
		const size_t open = Skip(list, equals + 1, " \t\r\n");

		if (equals >= list.size() || list[equals] != '=' || open >= list.size() || list[open] != '{')
			continue;

		const size_t end = StageArchive::MatchPair(list, open);

		if (end == std::string::npos)
			return;

		Span span = {};
		span.number = atoi(list.c_str() + at + length);
		span.start = at;
		span.open = open;
		span.end = end;

		out.push_back(span);
		at = end - length;
	}
}

bool BgListText::SelectList(const std::string& list, std::vector<int>& numbers, size_t& valueAt, size_t& valueEnd)
{
	numbers.clear();

	size_t keyAt = 0;

	if (!StageArchive::FieldSpan(list, kSelectList, keyAt, valueAt, valueEnd) || list[valueAt] != '[')
		return false;

	for (size_t at = valueAt + 1; at + 1 < valueEnd; ++at)
	{
		if (isdigit(static_cast<unsigned char>(list[at])) == 0)
			continue;

		numbers.push_back(atoi(list.c_str() + at));

		while (at + 1 < valueEnd && isdigit(static_cast<unsigned char>(list[at + 1])) != 0)
			++at;
	}

	return true;
}

std::string BgListText::JoinList(const std::vector<int>& numbers)
{
	std::string out = "[ ";

	for (size_t i = 0; i < numbers.size(); ++i)
	{
		char text[16] = {};
		sprintf_s(text, "%s%d", i == 0 ? "" : ",", numbers[i]);
		out += text;
	}

	return out + " ]";
}

void BgListText::SetValue(std::string& block, const char* key, const std::string& value)
{
	size_t keyAt = 0;
	size_t valueAt = 0;
	size_t valueEnd = 0;

	if (!StageArchive::FieldSpan(block, key, keyAt, valueAt, valueEnd))
	{
		Insert(block, key, value);
		return;
	}

	block.replace(valueAt, valueEnd - valueAt, value);
}

void BgListText::RemoveValue(std::string& block, const char* key)
{
	size_t keyAt = 0;
	size_t valueAt = 0;
	size_t valueEnd = 0;

	if (!StageArchive::FieldSpan(block, key, keyAt, valueAt, valueEnd))
		return;

	const size_t line = LineStart(block, keyAt);

	if (BlankBetween(block, line, keyAt) && RestOfLineIsTail(block, valueEnd))
	{
		block.erase(line, LineEnd(block, valueEnd) - line);
		return;
	}

	const size_t comma = Skip(block, valueEnd, " \t");
	const size_t tail = comma < block.size() && block[comma] == ',' ? Skip(block, comma + 1, " \t") : comma;

	block.erase(keyAt, tail - keyAt);
}

std::string BgListText::Newline(const std::string& text)
{
	return text.find("\r\n") == std::string::npos ? "\n" : "\r\n";
}
