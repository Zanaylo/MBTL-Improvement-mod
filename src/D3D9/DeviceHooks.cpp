#include "D3D9/DeviceHooks.h"

#include "Core/logger.h"
#include "D3D9/PresentTuning.h"
#include "Hooks/HookManager.h"

#include <cstring>
#include <vector>

namespace {

constexpr int kResetIndex = 16;
constexpr int kPresentIndex = 17;
constexpr int kClearIndex = 43;
constexpr int kSetTextureIndex = 65;
constexpr DWORD kScaleCacheMs = 200;
constexpr float kLeastScale = 0.1f;
constexpr float kMostScale = 8.0f;

using Reset_t = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
using Present_t = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);
using Clear_t = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*, DWORD, const D3DRECT*, DWORD, D3DCOLOR, float, DWORD);
using SetTexture_t = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*, DWORD, IDirect3DBaseTexture9*);

Reset_t oReset = nullptr;
Present_t oPresent = nullptr;
Clear_t oClear = nullptr;
SetTexture_t oSetTexture = nullptr;

std::vector<IDeviceListener*> g_listeners;
std::vector<IDrawFilter*> g_filters;
IDirect3DDevice9* g_device = nullptr;
HWND g_window = nullptr;
D3DPRESENT_PARAMETERS g_parameters = {};
volatile LONG g_presentCount = 0;
bool g_installed = false;

void NotifyReady(IDirect3DDevice9* device)
{
	for (IDeviceListener* listener : g_listeners)
		listener->OnDeviceReady(device, g_parameters);
}

void NotifyLost()
{
	for (IDeviceListener* listener : g_listeners)
		listener->OnDeviceLost();
}

void NotifyPresent(IDirect3DDevice9* device)
{
	for (IDeviceListener* listener : g_listeners)
	{
		Profiler::Scope scope(listener->ProfileSection());
		listener->OnPresent(device);
	}
}

HRESULT STDMETHODCALLTYPE HookedReset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* parameters)
{
	if (device != g_device || parameters == nullptr)
		return oReset(device, parameters);

	const D3DPRESENT_PARAMETERS asked = *parameters;
	PresentTuning::Apply(device, *parameters);
	NotifyLost();

	HRESULT result = oReset(device, parameters);

	if (FAILED(result) && std::memcmp(&asked, parameters, sizeof(asked)) != 0)
	{
		LOG("Reset refused the tuned parameters (0x%08lX), retrying with the game's own", static_cast<unsigned long>(result));
		*parameters = asked;
		result = oReset(device, parameters);
	}

	if (FAILED(result))
	{
		LOG("Reset failed: 0x%08lX", static_cast<unsigned long>(result));
		return result;
	}

	g_parameters = *parameters;
	LOG("Reset: %ux%u windowed=%d", g_parameters.BackBufferWidth, g_parameters.BackBufferHeight, g_parameters.Windowed);
	NotifyReady(device);
	return result;
}

HRESULT STDMETHODCALLTYPE HookedClear(IDirect3DDevice9* device, DWORD count, const D3DRECT* rects, DWORD flags,
	D3DCOLOR color, float z, DWORD stencil)
{
	if (device != g_device || (flags & D3DCLEAR_TARGET) == 0)
		return oClear(device, count, rects, flags, color, z, stencil);

	for (IDrawFilter* filter : g_filters)
		color = filter->OnClear(device, count, flags, color);

	return oClear(device, count, rects, flags, color, z, stencil);
}

HRESULT STDMETHODCALLTYPE HookedSetTexture(IDirect3DDevice9* device, DWORD stage, IDirect3DBaseTexture9* texture)
{
	if (device != g_device)
		return oSetTexture(device, stage, texture);

	for (IDrawFilter* filter : g_filters)
		texture = filter->OnSetTexture(device, stage, texture);

	return oSetTexture(device, stage, texture);
}

HRESULT STDMETHODCALLTYPE HookedPresent(IDirect3DDevice9* device, const RECT* source, const RECT* destination,
	HWND windowOverride, const RGNDATA* dirty)
{
	if (device != g_device)
		return oPresent(device, source, destination, windowOverride, dirty);

	InterlockedIncrement(&g_presentCount);
	NotifyPresent(device);

	HRESULT result = D3D_OK;

	{
		Profiler::Scope scope(Profiler::Section_PresentDevice);
		result = oPresent(device, source, destination, windowOverride, dirty);
	}

	Profiler::EndPresentFrame();
	return result;
}

}

void DeviceHooks::AddListener(IDeviceListener* listener)
{
	if (listener == nullptr)
		return;

	g_listeners.push_back(listener);
}

void DeviceHooks::AddFilter(IDrawFilter* filter)
{
	if (filter == nullptr)
		return;

	g_filters.push_back(filter);
}

bool DeviceHooks::Install(IDirect3DDevice9* device, const D3DPRESENT_PARAMETERS& parameters, HWND window)
{
	if (device == nullptr)
		return false;

	g_device = device;
	g_parameters = parameters;
	g_window = window != nullptr ? window : parameters.hDeviceWindow;

	if (g_installed)
	{
		LOG("A new Direct3D device 0x%p replaced the tracked one", static_cast<void*>(device));
		NotifyReady(device);
		return true;
	}

	void** const vtable = *reinterpret_cast<void***>(device);

	const bool reset = HookManager::CreateAndEnableHook(vtable[kResetIndex], &HookedReset,
		reinterpret_cast<void**>(&oReset), "IDirect3DDevice9::Reset");
	const bool present = HookManager::CreateAndEnableHook(vtable[kPresentIndex], &HookedPresent,
		reinterpret_cast<void**>(&oPresent), "IDirect3DDevice9::Present");

	if (!g_filters.empty())
	{
		HookManager::CreateAndEnableHook(vtable[kClearIndex], &HookedClear, reinterpret_cast<void**>(&oClear),
			"IDirect3DDevice9::Clear");
		HookManager::CreateAndEnableHook(vtable[kSetTextureIndex], &HookedSetTexture,
			reinterpret_cast<void**>(&oSetTexture), "IDirect3DDevice9::SetTexture");
	}

	if (!reset || !present)
	{
		LOG("Device hooks incomplete (reset=%d present=%d), so the overlay stays off", reset, present);
		return false;
	}

	g_installed = true;
	LOG("Device hooks installed on 0x%p, window 0x%p", static_cast<void*>(device), static_cast<void*>(g_window));
	NotifyReady(device);
	return true;
}

bool DeviceHooks::IsInstalled()
{
	return g_installed;
}

IDirect3DDevice9* DeviceHooks::Device()
{
	return g_device;
}

HWND DeviceHooks::Window()
{
	return g_window;
}

const D3DPRESENT_PARAMETERS& DeviceHooks::GetPresentParameters()
{
	return g_parameters;
}

unsigned long DeviceHooks::PresentCount()
{
	return static_cast<unsigned long>(InterlockedCompareExchange(&g_presentCount, 0, 0));
}

float DeviceHooks::OverlayScale()
{
	static float cached = 1.0f;
	static DWORD cachedTick = 0;

	const DWORD now = GetTickCount();
	if (cachedTick != 0 && now - cachedTick < kScaleCacheMs)
		return cached;

	cachedTick = now;
	cached = 1.0f;

	RECT client = {};
	if (g_parameters.BackBufferWidth == 0 || g_window == nullptr || !GetClientRect(g_window, &client) || client.right <= 0)
		return cached;

	const float scale = static_cast<float>(g_parameters.BackBufferWidth) / static_cast<float>(client.right);
	cached = scale > kLeastScale && scale < kMostScale ? scale : 1.0f;
	return cached;
}
