#include "Overlay/WindowManager.h"

#include "Core/KeyState.h"
#include "Core/KeyboardCapture.h"
#include "Core/ProcessTuning.h"
#include "Core/info.h"
#include "Core/interfaces.h"
#include "Core/logger.h"
#include "D3D9/DeviceHooks.h"
#include "Overlay/HotkeyActions.h"
#include "Overlay/NotificationBar.h"
#include "Overlay/OverlayFont.h"
#include "Web/UpdateCheck.h"

#include <backends/imgui_impl_dx9.h>
#include <backends/imgui_impl_win32.h>
#include <imgui_internal.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace {

constexpr float kReferenceFontSize = 13.0f;
constexpr int kCursorSoftware = 1;
constexpr int kCursorHardware = 2;

WNDPROC g_originalWndProc = nullptr;
bool g_unicodeWindow = false;

bool WantsTextInputThisFrame()
{
	const ImGuiContext* const context = ImGui::GetCurrentContext();
	if (context != nullptr && context->WantTextInputNextFrame == 1)
		return true;

	return ImGui::GetIO().WantTextInput;
}

bool HasClientMousePosition(UINT message)
{
	switch (message)
	{
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_MBUTTONDBLCLK:
	case WM_XBUTTONDOWN: case WM_XBUTTONUP: case WM_XBUTTONDBLCLK:
		return true;
	default:
		return false;
	}
}

bool IsMouseMessage(UINT message)
{
	return HasClientMousePosition(message) || message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL;
}

bool IsKeyboardMessage(UINT message)
{
	switch (message)
	{
	case WM_KEYDOWN: case WM_KEYUP:
	case WM_SYSKEYDOWN: case WM_SYSKEYUP:
	case WM_CHAR: case WM_SYSCHAR:
	case WM_DEADCHAR: case WM_SYSDEADCHAR:
	case WM_UNICHAR:
	case WM_IME_CHAR:
	case WM_IME_STARTCOMPOSITION: case WM_IME_COMPOSITION: case WM_IME_ENDCOMPOSITION:
		return true;
	default:
		return false;
	}
}

LRESULT CallOriginal(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (g_originalWndProc == nullptr)
		return g_unicodeWindow ? DefWindowProcW(window, message, wParam, lParam)
			: DefWindowProcA(window, message, wParam, lParam);

	return g_unicodeWindow ? CallWindowProcW(g_originalWndProc, window, message, wParam, lParam)
		: CallWindowProcA(g_originalWndProc, window, message, wParam, lParam);
}

LRESULT CALLBACK ModWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	bool handled = false;
	const LRESULT result = WindowManager::GetInstance().HandleWindowMessage(window, message, wParam, lParam, handled);

	return handled ? result : CallOriginal(window, message, wParam, lParam);
}

bool WantsSoftwareCursor()
{
	if (g_settings.overlayCursor == kCursorSoftware)
		return true;

	if (g_settings.overlayCursor == kCursorHardware)
		return false;

	return DeviceHooks::GetPresentParameters().Windowed == FALSE;
}

}

WindowManager& WindowManager::GetInstance()
{
	static WindowManager instance;
	return instance;
}

void WindowManager::OnDeviceReady(IDirect3DDevice9* device, const D3DPRESENT_PARAMETERS&)
{
	if (!m_initialized)
	{
		Initialize(DeviceHooks::Window(), device);
		return;
	}

	if (device != m_device)
	{
		ImGui_ImplDX9_Shutdown();
		m_device = device;
		m_deviceObjectsValid = ImGui_ImplDX9_Init(device);
		LOG("WindowManager moved to device 0x%p", static_cast<void*>(device));
		return;
	}

	m_deviceObjectsValid = ImGui_ImplDX9_CreateDeviceObjects();

	if (!m_deviceObjectsValid)
		LOG("ImGui_ImplDX9_CreateDeviceObjects failed after reset");
}

void WindowManager::OnDeviceLost()
{
	if (!m_initialized)
		return;

	ImGui_ImplDX9_InvalidateDeviceObjects();
	m_deviceObjectsValid = false;
}

void WindowManager::OnPresent(IDirect3DDevice9*)
{
	Render();
}

bool WindowManager::Initialize(HWND window, IDirect3DDevice9* device)
{
	if (window == nullptr || device == nullptr)
	{
		LOG("WindowManager cannot start: window=0x%p device=0x%p", static_cast<void*>(window), static_cast<void*>(device));
		return false;
	}

	m_window = window;
	m_device = device;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	OverlayFont::Load();

	ImGui::StyleColorsDark();
	m_fontScale = ImGui::GetStyle().FontScaleMain;
	m_baseStyle = ImGui::GetStyle();
	m_appliedScale = 0.0f;

	if (!ImGui_ImplWin32_Init(window))
	{
		LOG("ImGui_ImplWin32_Init failed");
		ImGui::DestroyContext();
		return false;
	}

	if (!ImGui_ImplDX9_Init(device))
	{
		LOG("ImGui_ImplDX9_Init failed");
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
		return false;
	}

	m_deviceObjectsValid = true;
	m_container = std::make_unique<WindowContainer>();

	InstallWindowProc(window);
	ProcessTuning::SetWindow(window);

	m_hasFocus = GetForegroundWindow() == window;
	m_initialized = true;

	NotificationBar::Add("%s %s loaded. Press %s to open the main window.", MBTL_IM_NAME, MBTL_IM_VERSION,
		KeyBinds::Format(g_settings.toggleOverlayKey).c_str());

	LOG("WindowManager initialized (ImGui %s), window 0x%p", IMGUI_VERSION, static_cast<void*>(window));
	return true;
}

void WindowManager::Shutdown()
{
	if (!m_initialized)
		return;

	ProcessTuning::Shutdown();
	RemoveWindowProc();
	m_container.reset();

	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	m_initialized = false;
	m_deviceObjectsValid = false;
	m_device = nullptr;
	m_window = nullptr;
}

void WindowManager::InstallWindowProc(HWND window)
{
	g_unicodeWindow = IsWindowUnicode(window) != FALSE;

	const LONG_PTR detour = reinterpret_cast<LONG_PTR>(&ModWindowProc);
	g_originalWndProc = reinterpret_cast<WNDPROC>(g_unicodeWindow ? SetWindowLongPtrW(window, GWLP_WNDPROC, detour)
		: SetWindowLongPtrA(window, GWLP_WNDPROC, detour));

	m_originalWndProc = g_originalWndProc;
	LOG("Window procedure installed (original 0x%p, %s)", static_cast<void*>(g_originalWndProc),
		g_unicodeWindow ? "unicode" : "ansi");
}

void WindowManager::RemoveWindowProc()
{
	if (m_window == nullptr || m_originalWndProc == nullptr)
		return;

	const LONG_PTR original = reinterpret_cast<LONG_PTR>(m_originalWndProc);

	if (g_unicodeWindow)
		SetWindowLongPtrW(m_window, GWLP_WNDPROC, original);
	else
		SetWindowLongPtrA(m_window, GWLP_WNDPROC, original);

	g_originalWndProc = nullptr;
	m_originalWndProc = nullptr;
}

LRESULT WindowManager::HandleWindowMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam, bool& outHandled)
{
	outHandled = false;
	ObserveFocus(message, wParam);

	if (!m_initialized)
		return 0;

	if (m_interactive)
		ImGui_ImplWin32_WndProcHandler(window, message, wParam,
			HasClientMousePosition(message) ? ScaleMousePosition(lParam) : lParam);

	if (KeyboardCapture::OwnsKeyboard() && IsKeyboardMessage(message))
	{
		outHandled = true;
		return 1;
	}

	if (!m_interactive)
		return 0;

	const ImGuiIO& io = ImGui::GetIO();

	if (io.WantCaptureMouse && (message == WM_SETCURSOR || IsMouseMessage(message)))
	{
		outHandled = true;
		return 1;
	}

	return 0;
}

bool WindowManager::WantsInputCapture() const
{
	if (!m_initialized || !m_interactive)
		return false;

	const ImGuiIO& io = ImGui::GetIO();
	return io.WantCaptureKeyboard || io.WantCaptureMouse;
}

void WindowManager::ObserveFocus(UINT message, WPARAM wParam)
{
	if (message == WM_ACTIVATEAPP)
	{
		m_hasFocus = wParam != 0;

		if (m_hasFocus)
			ProcessTuning::Reassert();

		return;
	}

	if (message == WM_SETFOCUS)
		ProcessTuning::Reassert();
}

void WindowManager::PollInput()
{
	if (!m_hasFocus && GetForegroundWindow() != m_window)
	{
		KeyState::Clear();
		return;
	}

	m_hasFocus = true;
	KeyState::Poll();

	if (KeyboardCapture::OwnsKeyboard())
		return;

	HotkeyActions::Run(*m_container);
}

bool WindowManager::GetBackBufferScale(float& outX, float& outY) const
{
	outX = 1.0f;
	outY = 1.0f;

	const D3DPRESENT_PARAMETERS& present = DeviceHooks::GetPresentParameters();
	if (present.BackBufferWidth == 0 || present.BackBufferHeight == 0 || m_window == nullptr)
		return false;

	RECT client = {};
	if (!GetClientRect(m_window, &client) || client.right <= 0 || client.bottom <= 0)
		return false;

	outX = static_cast<float>(present.BackBufferWidth) / static_cast<float>(client.right);
	outY = static_cast<float>(present.BackBufferHeight) / static_cast<float>(client.bottom);

	return outX != 1.0f || outY != 1.0f;
}

LPARAM WindowManager::ScaleMousePosition(LPARAM lParam) const
{
	float scaleX = 1.0f;
	float scaleY = 1.0f;

	if (!GetBackBufferScale(scaleX, scaleY))
		return lParam;

	const int x = static_cast<int>(static_cast<short>(LOWORD(lParam)) * scaleX);
	const int y = static_cast<int>(static_cast<short>(HIWORD(lParam)) * scaleY);

	return MAKELPARAM(static_cast<short>(x), static_cast<short>(y));
}

void WindowManager::ScaleToBackBuffer()
{
	float scaleX = 1.0f;
	float scaleY = 1.0f;

	if (!GetBackBufferScale(scaleX, scaleY))
	{
		ApplyScale(g_settings.uiScale);
		return;
	}

	ImGuiIO& io = ImGui::GetIO();
	const D3DPRESENT_PARAMETERS& present = DeviceHooks::GetPresentParameters();

	RECT client = {};
	POINT cursor = {};

	if (GetClientRect(m_window, &client) && GetCursorPos(&cursor) && ScreenToClient(m_window, &cursor) &&
		cursor.x >= 0 && cursor.y >= 0 && cursor.x < client.right && cursor.y < client.bottom)
	{
		io.AddMousePosEvent(cursor.x * scaleX, cursor.y * scaleY);
	}

	io.DisplaySize.x = static_cast<float>(present.BackBufferWidth);
	io.DisplaySize.y = static_cast<float>(present.BackBufferHeight);

	ApplyScale(g_settings.uiScale * scaleX);
}

void WindowManager::ApplyScale(float scale)
{
	if (scale <= 0.0f || (scale == m_appliedScale && g_settings.fontSize == m_appliedFontSize))
		return;

	m_appliedFontSize = g_settings.fontSize;

	ImGuiStyle& style = ImGui::GetStyle();
	const float text = g_settings.fontSize / kReferenceFontSize;

	style = m_baseStyle;
	style.ScaleAllSizes(scale * text);
	style.FontScaleMain = m_fontScale * scale;
	style.FontSizeBase = g_settings.fontSize;

	m_appliedScale = scale;
}

void WindowManager::Render()
{
	if (!m_initialized || !m_deviceObjectsValid || IsIconic(m_window))
	{
		KeyboardCapture::ReleaseAll();
		return;
	}

	PollInput();
	AnnounceUpdate();

	m_overlayActive = m_container->AnyWindowOpen();
	m_interactive = m_container->AnyInteractiveWindowOpen();

	if (!m_interactive)
		KeyboardCapture::ReleaseAll();

	if (!m_overlayActive && !NotificationBar::HasPending())
		return;

	ImGui::GetIO().MouseDrawCursor = m_interactive && WantsSoftwareCursor();

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ScaleToBackBuffer();
	ImGui::NewFrame();

	KeyboardCapture::DecayKeyCapture();

	m_container->UpdateAll();
	NotificationBar::Draw();

	ImGui::EndFrame();

	KeyboardCapture::SetTextInputActive(m_interactive && WantsTextInputThisFrame());

	ImGui::Render();

	if (FAILED(m_device->BeginScene()))
		return;

	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
	m_device->EndScene();
}

void WindowManager::OpenUpdateNotifier()
{
	if (m_container == nullptr)
		return;

	IWindow* const window = m_container->GetWindow(WindowType_UpdateNotifier);

	if (window != nullptr)
		window->Open();
}

void WindowManager::AnnounceUpdate()
{
	if (m_updateAnnounced || !UpdateCheck::HasNewer())
		return;

	m_updateAnnounced = true;
	OpenUpdateNotifier();
}
