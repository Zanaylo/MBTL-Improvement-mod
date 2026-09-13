#include "Core/Settings.h"

#include "Core/interfaces.h"
#include "Core/utils.h"

#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

constexpr char kMissing[] = "\x01missing\x01";
constexpr DWORD kValueBytes = 1024;

std::string g_path;

bool HasKey(const char* section, const char* key)
{
	char buffer[32] = {};
	GetPrivateProfileStringA(section, key, kMissing, buffer, sizeof(buffer), g_path.c_str());
	return std::strcmp(buffer, kMissing) != 0;
}

std::string ReadString(const char* section, const char* key, const char* fallback)
{
	char buffer[kValueBytes] = {};
	GetPrivateProfileStringA(section, key, fallback, buffer, kValueBytes, g_path.c_str());
	return buffer;
}

void EnsureKey(const char* section, const char* key, const std::string& value)
{
	if (HasKey(section, key))
		return;

	WritePrivateProfileStringA(section, key, value.c_str(), g_path.c_str());
}

std::string FloatText(float value)
{
	char text[32] = {};
	sprintf_s(text, "%.3f", value);
	return text;
}

int ReadInt(const char* section, const char* key, int fallback)
{
	EnsureKey(section, key, std::to_string(fallback));
	return static_cast<int>(GetPrivateProfileIntA(section, key, fallback, g_path.c_str()));
}

float ReadFloat(const char* section, const char* key, float fallback)
{
	const std::string text = FloatText(fallback);
	EnsureKey(section, key, text);
	return static_cast<float>(atof(ReadString(section, key, text.c_str()).c_str()));
}

std::string ReadText(const char* section, const char* key, const char* fallback)
{
	EnsureKey(section, key, fallback);
	return ReadString(section, key, fallback);
}

}

const std::string& Settings::IniPath()
{
	if (g_path.empty())
		g_path = GetModRootPath("MBTL_IM.ini");

	return g_path;
}

void Settings::Load()
{
	IniPath();

#define SETTING_INT(member, section, key, fallback) g_settings.member = ReadInt(section, key, fallback);
#define SETTING_BOOL(member, section, key, fallback) g_settings.member = ReadInt(section, key, fallback) != 0;
#define SETTING_FLOAT(member, section, key, fallback) g_settings.member = ReadFloat(section, key, fallback);
#define SETTING_STRING(member, section, key, fallback) g_settings.member = ReadText(section, key, fallback);
#define SETTING_KEY(member, section, key, fallback) g_settings.member = KeyBinds::Parse(ReadText(section, key, fallback));
#include "Core/settings.def"
#undef SETTING_INT
#undef SETTING_BOOL
#undef SETTING_FLOAT
#undef SETTING_STRING
#undef SETTING_KEY
}

void Settings::SaveInt(const char* section, const char* key, int value)
{
	WritePrivateProfileStringA(section, key, std::to_string(value).c_str(), IniPath().c_str());
}

void Settings::SaveBool(const char* section, const char* key, bool value)
{
	SaveInt(section, key, value ? 1 : 0);
}

void Settings::SaveFloat(const char* section, const char* key, float value)
{
	WritePrivateProfileStringA(section, key, FloatText(value).c_str(), IniPath().c_str());
}

void Settings::SaveString(const char* section, const char* key, const char* value)
{
	WritePrivateProfileStringA(section, key, value, IniPath().c_str());
}

void Settings::SaveKey(const char* section, const char* key, const KeyBind& bind)
{
	SaveString(section, key, KeyBinds::Format(bind).c_str());
}

std::string Settings::ReadSection(const char* section)
{
	std::string buffer(8192, '\0');

	for (;;)
	{
		const DWORD read = GetPrivateProfileSectionA(section, &buffer[0],
			static_cast<DWORD>(buffer.size()), IniPath().c_str());

		if (read < buffer.size() - 2)
		{
			buffer.resize(read);
			return buffer;
		}

		buffer.assign(buffer.size() * 2, '\0');
	}
}
