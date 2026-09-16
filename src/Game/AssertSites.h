#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

struct AssertSite
{
	uint8_t* block = nullptr;
	size_t length = 0;
	const wchar_t* file = nullptr;
	const wchar_t* expression = nullptr;
	unsigned line = 0;
	int bound = -1;
};

namespace AssertSites
{
	uint8_t* ImportSlot();
	std::vector<AssertSite> Find();

	bool FileIs(const wchar_t* path, const wchar_t* name);
	void Narrow(const wchar_t* text, char* out, size_t size);
	void Describe(const wchar_t* file, unsigned line, char* out, size_t size);
}
