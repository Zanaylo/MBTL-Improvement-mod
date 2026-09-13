#include "Core/logger.h"

#include "Core/interfaces.h"
#include "Core/utils.h"

#include <windows.h>
#include <share.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

constexpr size_t kKeptLogs = 20;
constexpr size_t kLineBytes = 2048;

CRITICAL_SECTION g_lock;
bool g_lockReady = false;
FILE* g_file = nullptr;
char g_lastLine[kLineBytes] = {};
unsigned g_repeats = 0;

void PruneOldLogs(const std::string& folder)
{
	std::vector<std::string> names;
	WIN32_FIND_DATAA entry;
	const HANDLE search = FindFirstFileA((folder + "\\MBTL_IM_*.log").c_str(), &entry);
	if (search == INVALID_HANDLE_VALUE)
		return;

	do
	{
		if (!(entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			names.emplace_back(entry.cFileName);
	} while (FindNextFileA(search, &entry));

	FindClose(search);

	if (names.size() < kKeptLogs)
		return;

	std::sort(names.begin(), names.end());
	const size_t excess = names.size() - kKeptLogs + 1;
	for (size_t i = 0; i < excess; ++i)
		DeleteFileA((folder + "\\" + names[i]).c_str());
}

void FlushRepeats()
{
	if (g_repeats == 0)
		return;

	fprintf(g_file, "(the line above repeated %u more time%s)\n", g_repeats, g_repeats == 1 ? "" : "s");
	g_repeats = 0;
}

}

bool LoggerEnabled()
{
	return g_file != nullptr;
}

void OpenLogger()
{
	if (g_file)
		return;

	if (!MBTL_IM_FORCE_LOGGING && !g_settings.logging)
		return;

	if (!g_lockReady)
	{
		InitializeCriticalSection(&g_lock);
		g_lockReady = true;
	}

	const std::string folder = GetModRootPath("Logs");
	PruneOldLogs(folder);

	SYSTEMTIME now;
	GetLocalTime(&now);

	char name[64] = {};
	sprintf_s(name, "MBTL_IM_%04u%02u%02u_%02u%02u%02u.log", now.wYear, now.wMonth, now.wDay, now.wHour,
		now.wMinute, now.wSecond);

	g_file = _fsopen((folder + "\\" + name).c_str(), "w", _SH_DENYWR);
}

void CloseLogger()
{
	if (!g_file)
		return;

	EnterCriticalSection(&g_lock);
	FlushRepeats();
	fclose(g_file);
	g_file = nullptr;
	LeaveCriticalSection(&g_lock);
}

void WriteLog(const char* format, ...)
{
	if (!g_file)
		return;

	char message[kLineBytes] = {};
	va_list args;
	va_start(args, format);
	vsnprintf(message, sizeof(message), format, args);
	va_end(args);

	EnterCriticalSection(&g_lock);

	if (!g_file)
	{
		LeaveCriticalSection(&g_lock);
		return;
	}

	if (std::strcmp(message, g_lastLine) == 0)
	{
		++g_repeats;
		LeaveCriticalSection(&g_lock);
		return;
	}

	FlushRepeats();
	strcpy_s(g_lastLine, message);

	SYSTEMTIME now;
	GetLocalTime(&now);
	fprintf(g_file, "[%02u:%02u:%02u.%03u] %s\n", now.wHour, now.wMinute, now.wSecond, now.wMilliseconds, message);
	fflush(g_file);

	LeaveCriticalSection(&g_lock);
}
