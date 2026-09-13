#include "D3D9/D3D9Proxy.h"

#include "Core/logger.h"
#include "Core/utils.h"
#include "D3D9/D3D9Hooks.h"

#include <windows.h>

#include <cstring>
#include <string>

namespace {

constexpr char kProxyName[] = "d3d9.dll";

HMODULE g_real = nullptr;
bool g_loadTried = false;
SRWLOCK g_loadLock = SRWLOCK_INIT;

bool g_identityKnown = false;
bool g_isD3D9 = false;
char g_loadedAs[64] = "";

void ResolveIdentity()
{
	if (g_identityKnown)
		return;

	g_identityKnown = true;

	char path[MAX_PATH] = {};
	if (GetModuleFileNameA(GetModModuleHandle(), path, MAX_PATH) == 0)
		return;

	const char* slash = std::strrchr(path, '\\');
	const char* name = slash ? slash + 1 : path;

	strncpy_s(g_loadedAs, name, _TRUNCATE);
	g_isD3D9 = _stricmp(name, kProxyName) == 0;
}

void LoadRealRuntime()
{
	const std::string path = GetSystemDirectoryPath() + kProxyName;
	const HMODULE loaded = LoadLibraryA(path.c_str());

	if (!loaded)
	{
		LOG("d3d9 proxy: could not load the real runtime from %s (error %lu)", path.c_str(), GetLastError());
		return;
	}

	if (loaded == GetModModuleHandle())
	{
		LOG("d3d9 proxy: %s resolved back to this mod, so there is nothing to chain to", path.c_str());
		return;
	}

	g_real = loaded;
	LOG("d3d9 proxy: real runtime loaded from %s", path.c_str());
}

HMODULE RealRuntime()
{
	AcquireSRWLockExclusive(&g_loadLock);

	if (!g_loadTried)
	{
		g_loadTried = true;
		LoadRealRuntime();
	}

	const HMODULE real = g_real;
	ReleaseSRWLockExclusive(&g_loadLock);
	return real;
}

template <typename Fn>
Fn Real(const char* name, FARPROC& cache)
{
	if (cache)
		return reinterpret_cast<Fn>(cache);

	const HMODULE real = RealRuntime();
	if (!real)
		return nullptr;

	cache = GetProcAddress(real, name);
	if (!cache)
		LOG("d3d9 proxy: %s is missing from the real runtime", name);

	return reinterpret_cast<Fn>(cache);
}

}

bool D3D9Proxy::IsActive()
{
	ResolveIdentity();
	return g_isD3D9;
}

const char* D3D9Proxy::LoadedAs()
{
	ResolveIdentity();
	return g_loadedAs;
}

HMODULE D3D9Proxy::RealModule()
{
	return RealRuntime();
}

extern "C" IDirect3D9* WINAPI Direct3DCreate9(UINT sdkVersion)
{
	static FARPROC cache = nullptr;
	const auto real = Real<IDirect3D9* (WINAPI*)(UINT)>("Direct3DCreate9", cache);
	if (!real)
		return nullptr;

	IDirect3D9* const d3d9 = real(sdkVersion);
	LOG("d3d9 proxy: Direct3DCreate9(sdk %u) -> 0x%p", sdkVersion, static_cast<void*>(d3d9));

	D3D9Hooks::OnDirect3D9Created(d3d9);
	return d3d9;
}

extern "C" HRESULT WINAPI Direct3DCreate9Ex(UINT sdkVersion, IDirect3D9Ex** out)
{
	static FARPROC cache = nullptr;
	const auto real = Real<HRESULT(WINAPI*)(UINT, IDirect3D9Ex**)>("Direct3DCreate9Ex", cache);
	if (!real)
		return E_FAIL;

	const HRESULT result = real(sdkVersion, out);

	if (SUCCEEDED(result) && out != nullptr)
		D3D9Hooks::OnDirect3D9Created(*out);

	return result;
}

extern "C" int WINAPI D3DPERF_BeginEvent(DWORD color, LPCWSTR name)
{
	static FARPROC cache = nullptr;
	const auto real = Real<int(WINAPI*)(DWORD, LPCWSTR)>("D3DPERF_BeginEvent", cache);
	return real ? real(color, name) : -1;
}

extern "C" int WINAPI D3DPERF_EndEvent()
{
	static FARPROC cache = nullptr;
	const auto real = Real<int(WINAPI*)()>("D3DPERF_EndEvent", cache);
	return real ? real() : -1;
}

extern "C" void WINAPI D3DPERF_SetMarker(DWORD color, LPCWSTR name)
{
	static FARPROC cache = nullptr;
	const auto real = Real<void(WINAPI*)(DWORD, LPCWSTR)>("D3DPERF_SetMarker", cache);
	if (real)
		real(color, name);
}

extern "C" void WINAPI D3DPERF_SetRegion(DWORD color, LPCWSTR name)
{
	static FARPROC cache = nullptr;
	const auto real = Real<void(WINAPI*)(DWORD, LPCWSTR)>("D3DPERF_SetRegion", cache);
	if (real)
		real(color, name);
}

extern "C" BOOL WINAPI D3DPERF_QueryRepeatFrame()
{
	static FARPROC cache = nullptr;
	const auto real = Real<BOOL(WINAPI*)()>("D3DPERF_QueryRepeatFrame", cache);
	return real ? real() : FALSE;
}

extern "C" void WINAPI D3DPERF_SetOptions(DWORD options)
{
	static FARPROC cache = nullptr;
	const auto real = Real<void(WINAPI*)(DWORD)>("D3DPERF_SetOptions", cache);
	if (real)
		real(options);
}

extern "C" DWORD WINAPI D3DPERF_GetStatus()
{
	static FARPROC cache = nullptr;
	const auto real = Real<DWORD(WINAPI*)()>("D3DPERF_GetStatus", cache);
	return real ? real() : 0;
}
