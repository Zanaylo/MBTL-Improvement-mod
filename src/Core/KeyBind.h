#pragma once

#include <string>

struct KeyBind
{
	int key = 0;
	bool function = false;
};

namespace KeyBinds
{
	KeyBind Parse(const std::string& text);
	std::string Format(const KeyBind& bind);
	bool SameKey(const KeyBind& a, const KeyBind& b);
}
