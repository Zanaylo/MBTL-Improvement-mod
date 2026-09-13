#pragma once

#include "Core/KeyBind.h"

#include <string>

struct SettingsIni
{
#define SETTING_INT(member, section, key, fallback) int member = fallback;
#define SETTING_BOOL(member, section, key, fallback) bool member = fallback;
#define SETTING_FLOAT(member, section, key, fallback) float member = fallback;
#define SETTING_STRING(member, section, key, fallback) std::string member = fallback;
#define SETTING_KEY(member, section, key, fallback) KeyBind member;
#include "Core/settings.def"
#undef SETTING_INT
#undef SETTING_BOOL
#undef SETTING_FLOAT
#undef SETTING_STRING
#undef SETTING_KEY
};

namespace Settings
{
	void Load();
	const std::string& IniPath();

	void SaveInt(const char* section, const char* key, int value);
	void SaveBool(const char* section, const char* key, bool value);
	void SaveFloat(const char* section, const char* key, float value);
	void SaveString(const char* section, const char* key, const char* value);
	void SaveKey(const char* section, const char* key, const KeyBind& bind);

	std::string ReadSection(const char* section);
}
