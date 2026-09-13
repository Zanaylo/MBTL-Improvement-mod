#pragma once

#if defined(_DEBUG) || defined(FORCE_LOGGING)
#define MBTL_IM_FORCE_LOGGING 1
#else
#define MBTL_IM_FORCE_LOGGING 0
#endif

void OpenLogger();
void CloseLogger();
bool LoggerEnabled();
void WriteLog(const char* format, ...);

#define LOG(...) WriteLog(__VA_ARGS__)
