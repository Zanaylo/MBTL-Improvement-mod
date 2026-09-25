#include "Performance/PerformanceModule.h"

#include "Core/Compat.h"
#include "Core/interfaces.h"
#include "D3D9/DeviceHooks.h"
#include "D3D9/Post/PostChain.h"
#include "D3D9/Post/SceneUpscale.h"
#include "D3D9/Post/ShaderPack.h"
#include "Game/GameOffsets.h"
#include "Performance/EngineQuality.h"
#include "Performance/InputLagMeter.h"
#include "Performance/PotatoMode.h"
#include "Performance/PumpWait.h"
#include "Performance/RenderMap.h"
#include "Performance/StageColor.h"

namespace {

namespace Render = GameOffsets::Render;

bool TargetIsScene(IDirect3DDevice9* device)
{
	IDirect3DSurface9* target = nullptr;

	if (FAILED(device->GetRenderTarget(0, &target)) || target == nullptr)
		return false;

	D3DSURFACE_DESC desc = {};
	const bool described = SUCCEEDED(target->GetDesc(&desc));
	target->Release();

	return described && desc.Width == static_cast<UINT>(Render::kSceneWidth) &&
		desc.Height == static_cast<UINT>(Render::kSceneHeight);
}

class PerformanceListener final : public IDeviceListener
{
public:
	void OnDeviceReady(IDirect3DDevice9*, const D3DPRESENT_PARAMETERS&) override
	{
		if (m_shadersScanned)
			return;

		m_shadersScanned = true;
		ShaderPack::Refresh();
	}

	void OnDeviceLost() override
	{
		PostChain::OnDeviceLost();
		SceneUpscale::OnDeviceLost();
	}

	void OnPresent(IDirect3DDevice9* device) override
	{
		PostChain::Apply(device);
		SceneUpscale::OnPresent();
		PotatoMode::OnFrame();
	}

	Profiler::Section ProfileSection() const override
	{
		return Profiler::Section_PresentPost;
	}

private:
	bool m_shadersScanned = false;
};

class PerformanceFilter final : public IDrawFilter
{
public:
	IDirect3DBaseTexture9* OnSetTexture(IDirect3DDevice9* device, DWORD stage, IDirect3DBaseTexture9* texture) override
	{
		return SceneUpscale::OnSetTexture(device, stage, texture);
	}

	D3DCOLOR OnClear(IDirect3DDevice9* device, DWORD count, DWORD, D3DCOLOR color) override
	{
		if (count != 0 || !StageColor::IsEnabled() || !TargetIsScene(device))
			return color;

		return StageColor::GetClearColor();
	}
};

PerformanceListener g_listener;
PerformanceFilter g_filter;

}

void PerformanceModule::Install()
{
	RenderMap::Initialize();
	StageColor::Install();
	InputLagMeter::Initialize();

	PotatoMode::ApplySaved();
	EngineQuality::Apply();

	StageColor::Apply();

	if (!Compat::SafeMode())
		PumpWait::Install();

	PumpWait::Apply();

	DeviceHooks::AddListener(&g_listener);
	DeviceHooks::AddFilter(&g_filter);
}
