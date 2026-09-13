#include "Core/KeyBind.h"

#include "Core/keycodes.h"

#include <cstring>

namespace {

constexpr const char* kFunctionPrefix = "Fn+";
constexpr size_t kFunctionPrefixLength = 3;

}

KeyBind KeyBinds::Parse(const std::string& text)
{
	KeyBind bind;
	bind.function = _strnicmp(text.c_str(), kFunctionPrefix, kFunctionPrefixLength) == 0;
	bind.key = GetVirtualKeyFromName(text);

	if (bind.key == 0)
		bind.function = false;

	return bind;
}

std::string KeyBinds::Format(const KeyBind& bind)
{
	if (bind.key == 0)
		return std::string();

	const std::string name = GetNameFromVirtualKey(bind.key);
	return bind.function ? std::string(kFunctionPrefix) + name : name;
}

bool KeyBinds::SameKey(const KeyBind& a, const KeyBind& b)
{
	return a.key != 0 && a.key == b.key && a.function == b.function;
}
