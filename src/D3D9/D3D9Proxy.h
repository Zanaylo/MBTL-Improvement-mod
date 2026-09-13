#pragma once

#include <windows.h>

namespace D3D9Proxy
{
	bool IsActive();
	const char* LoadedAs();
	HMODULE RealModule();
}
