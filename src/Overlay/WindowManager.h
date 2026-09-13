#pragma once

#include "D3D9/DeviceListener.h"
#include "Overlay/WindowContainer/WindowContainer.h"

#include <imgui.h>

#include <memory>

class WindowManager : public IDeviceListener
{
public:
	static WindowManager& GetInstance();

	void OnDeviceReady(IDirect3DDevice9* device, const D3DPRESENT_PARAMETERS& parameters) override;
	void OnDeviceLost() override;
	void OnPresent(IDirect3DDevice9* device) override;
	Profiler::Section ProfileSection() const override { return Profiler::Section_PresentOverlay; }

	void Shutdown();

	bool IsInitialized() const { return m_initialized; }
	bool IsInteractive() const { return m_interactive; }
	bool WantsInputCapture() const;

	WindowContainer* GetContainer() { return m_container.get(); }

	void OpenUpdateNotifier();

	LRESULT HandleWindowMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam, bool& outHandled);

private:
	WindowManager() = default;
	~WindowManager() = default;
	WindowManager(const WindowManager&) = delete;
	WindowManager& operator=(const WindowManager&) = delete;

	bool Initialize(HWND window, IDirect3DDevice9* device);
	void Render();
	void PollInput();
	void ScaleToBackBuffer();
	bool GetBackBufferScale(float& outX, float& outY) const;
	LPARAM ScaleMousePosition(LPARAM lParam) const;
	void ApplyScale(float scale);
	void ObserveFocus(UINT message, WPARAM wParam);
	void InstallWindowProc(HWND window);
	void RemoveWindowProc();
	void AnnounceUpdate();

	bool m_initialized = false;
	bool m_updateAnnounced = false;
	bool m_deviceObjectsValid = false;
	bool m_overlayActive = false;
	bool m_interactive = false;
	bool m_hasFocus = true;
	float m_fontScale = 1.0f;
	float m_appliedScale = 0.0f;
	float m_appliedFontSize = 0.0f;
	ImGuiStyle m_baseStyle;
	HWND m_window = nullptr;
	IDirect3DDevice9* m_device = nullptr;
	WNDPROC m_originalWndProc = nullptr;

	std::unique_ptr<WindowContainer> m_container;
};
