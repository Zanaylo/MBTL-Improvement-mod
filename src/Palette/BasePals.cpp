#include "Palette/BasePals.h"

#include "Core/utils.h"

#include <windows.h>

#include <cstdio>

bool BasePals::Has(int chara, int sub)
{
	const uint8_t* data = nullptr;
	size_t size = 0;

	return Get(chara, sub, data, size);
}

bool BasePals::Get(int chara, int sub, const uint8_t*& outData, size_t& outSize)
{
	outData = nullptr;
	outSize = 0;

	char name[24] = {};

	if (sub > 0)
		sprintf_s(name, "CHR%03d_P%d", chara, sub);
	else
		sprintf_s(name, "CHR%03d", chara);

	const HMODULE module = GetModModuleHandle();
	const HRSRC found = module != nullptr ? FindResourceA(module, name, MAKEINTRESOURCEA(10)) : nullptr;

	if (found == nullptr)
		return false;

	const DWORD size = SizeofResource(module, found);
	const HGLOBAL loaded = LoadResource(module, found);
	const void* const data = loaded != nullptr ? LockResource(loaded) : nullptr;

	if (data == nullptr || size == 0)
		return false;

	outData = static_cast<const uint8_t*>(data);
	outSize = size;
	return true;
}
