#include "D3D9/D3D9Hooks.h"

#include "Core/logger.h"
#include "D3D9/D3D9Proxy.h"
#include "D3D9/DeviceHooks.h"
#include "D3D9/PresentTuning.h"
#include "Hooks/HookManager.h"

#include <cstring>

namespace {

constexpr int kCreateDeviceIndex = 16;

using Direct3DCreate9_t = IDirect3D9*(WINAPI*)(UINT);
using Direct3DCreate9Ex_t = HRESULT(WINAPI*)(UINT, IDirect3D9Ex**);
using CreateDevice_t = HRESULT(STDMETHODCALLTYPE*)(IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD,
	D3DPRESENT_PARAMETERS*, IDirect3DDevice9**);

Direct3DCreate9_t oDirect3DCreate9 = nullptr;
Direct3DCreate9Ex_t oDirect3DCreate9Ex = nullptr;
CreateDevice_t oCreateDevice = nullptr;
bool g_createDeviceHooked = false;

HRESULT STDMETHODCALLTYPE HookedCreateDevice(IDirect3D9* self, UINT adapter, D3DDEVTYPE type, HWND focus,
	DWORD behavior, D3DPRESENT_PARAMETERS* parameters, IDirect3DDevice9** device)
{
	if (parameters == nullptr)
		return oCreateDevice(self, adapter, type, focus, behavior, parameters, device);

	const D3DPRESENT_PARAMETERS asked = *parameters;
	PresentTuning::Apply(self, adapter, *parameters);

	HRESULT result = oCreateDevice(self, adapter, type, focus, behavior, parameters, device);

	if (FAILED(result) && std::memcmp(&asked, parameters, sizeof(asked)) != 0)
	{
		LOG("CreateDevice refused the tuned parameters (0x%08lX), retrying with the game's own", static_cast<unsigned long>(result));
		*parameters = asked;
		result = oCreateDevice(self, adapter, type, focus, behavior, parameters, device);
	}

	if (FAILED(result) || device == nullptr || *device == nullptr)
	{
		LOG("CreateDevice failed: 0x%08lX", static_cast<unsigned long>(result));
		return result;
	}

	LOG("CreateDevice: %ux%u windowed=%d format=%d focus=0x%p", parameters->BackBufferWidth,
		parameters->BackBufferHeight, parameters->Windowed, parameters->BackBufferFormat, static_cast<void*>(focus));

	DeviceHooks::Install(*device, *parameters, focus);
	return result;
}

IDirect3D9* WINAPI HookedDirect3DCreate9(UINT sdkVersion)
{
	IDirect3D9* const d3d9 = oDirect3DCreate9(sdkVersion);
	LOG("Direct3DCreate9(sdk %u) -> 0x%p", sdkVersion, static_cast<void*>(d3d9));

	D3D9Hooks::OnDirect3D9Created(d3d9);
	return d3d9;
}

HRESULT WINAPI HookedDirect3DCreate9Ex(UINT sdkVersion, IDirect3D9Ex** out)
{
	const HRESULT result = oDirect3DCreate9Ex(sdkVersion, out);
	LOG("Direct3DCreate9Ex(sdk %u) -> 0x%08lX", sdkVersion, static_cast<unsigned long>(result));

	if (SUCCEEDED(result) && out != nullptr)
		D3D9Hooks::OnDirect3D9Created(*out);

	return result;
}

}

bool D3D9Hooks::Install()
{
	if (D3D9Proxy::IsActive())
	{
		LOG("Loaded as d3d9.dll, so the game calls the mod's own Direct3DCreate9");
		return true;
	}

	if (GetModuleHandleW(L"d3d9.dll") == nullptr)
	{
		LOG("d3d9.dll is not loaded yet, so the overlay cannot attach this run");
		return false;
	}

	const bool plain = HookManager::CreateApiHook(L"d3d9.dll", "Direct3DCreate9", &HookedDirect3DCreate9,
		reinterpret_cast<void**>(&oDirect3DCreate9));
	HookManager::CreateApiHook(L"d3d9.dll", "Direct3DCreate9Ex", &HookedDirect3DCreate9Ex,
		reinterpret_cast<void**>(&oDirect3DCreate9Ex));

	return plain;
}

void D3D9Hooks::OnDirect3D9Created(IDirect3D9* d3d9)
{
	if (d3d9 == nullptr || g_createDeviceHooked)
		return;

	void** const vtable = *reinterpret_cast<void***>(d3d9);

	g_createDeviceHooked = HookManager::CreateAndEnableHook(vtable[kCreateDeviceIndex], &HookedCreateDevice,
		reinterpret_cast<void**>(&oCreateDevice), "IDirect3D9::CreateDevice");
}
