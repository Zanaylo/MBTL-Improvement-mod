#include "Core/crashdump.h"

#include "Core/logger.h"
#include "Core/utils.h"

#include <windows.h>
#include <dbghelp.h>

#include <cstring>
#include <string>

namespace {

LPTOP_LEVEL_EXCEPTION_FILTER g_previousFilter = nullptr;

void ModuleNameOf(void* address, char* name, size_t size)
{
	HMODULE module = nullptr;
	const DWORD flags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;
	if (!GetModuleHandleExA(flags, static_cast<LPCSTR>(address), &module))
	{
		strcpy_s(name, size, "an unknown module");
		return;
	}

	char path[MAX_PATH] = {};
	GetModuleFileNameA(module, path, MAX_PATH);
	const char* slash = std::strrchr(path, '\\');
	strcpy_s(name, size, slash ? slash + 1 : path);
}

void WriteDump(EXCEPTION_POINTERS* exception)
{
	SYSTEMTIME now;
	GetLocalTime(&now);

	char file[64] = {};
	sprintf_s(file, "crash_%04u%02u%02u_%02u%02u%02u.dmp", now.wYear, now.wMonth, now.wDay, now.wHour,
		now.wMinute, now.wSecond);

	const std::string path = GetModRootPath("Logs") + "\\" + file;
	const HANDLE handle = CreateFileA(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, nullptr);
	if (handle == INVALID_HANDLE_VALUE)
		return;

	MINIDUMP_EXCEPTION_INFORMATION information{ GetCurrentThreadId(), exception, FALSE };
	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), handle, MiniDumpNormal, &information,
		nullptr, nullptr);
	CloseHandle(handle);

	LOG("Crash dump written to %s", path.c_str());
}

LONG WINAPI CrashFilter(EXCEPTION_POINTERS* exception)
{
	char module[MAX_PATH] = {};
	ModuleNameOf(exception->ExceptionRecord->ExceptionAddress, module, sizeof(module));

	LOG("Unhandled exception 0x%08lX at 0x%p in %s", exception->ExceptionRecord->ExceptionCode,
		exception->ExceptionRecord->ExceptionAddress, module);

	WriteDump(exception);
	CloseLogger();

	if (g_previousFilter)
		return g_previousFilter(exception);

	return EXCEPTION_CONTINUE_SEARCH;
}

}

void InstallCrashHandler()
{
	g_previousFilter = SetUnhandledExceptionFilter(&CrashFilter);
}
