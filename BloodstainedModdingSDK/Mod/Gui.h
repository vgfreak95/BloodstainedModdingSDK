#pragma once
#include <d3d11.h>
#include <Windows.h>
#include <dxgi.h>

class Gui
{
public:
	static Gui& Instance();

	bool Init();
	void Shutdown();
	void Render();

	void ToggleMenu() { m_Open = !m_Open; }
	bool IsOpen() const { return m_Open; }
	bool IsImGuiInit() const { return m_ImGuiInit; }

	bool InitImGui(IDXGISwapChain* swapChain);

	ID3D11Device* GetDevice() const { return m_Device; }
	ID3D11DeviceContext* GetDeviceContext() const { return m_DeviceContext; }
	ID3D11RenderTargetView* GetRenderTargetView() const { return m_RenderTargetView; }
	HWND GetGameWindow() const { return m_GameWindow; }
	WNDPROC GetOriginalWndProc() const { return m_OriginalWndProc; }
	void SetOriginalWndProc(WNDPROC proc) { m_OriginalWndProc = proc; }

	static long(__stdcall* originalPresent)(IDXGISwapChain*, unsigned int, unsigned int);
	static bool g_Hooked;

private:
	Gui() = default;
	~Gui() = default;
	Gui(const Gui&) = delete;
	Gui& operator=(const Gui&) = delete;

	ID3D11Device* m_Device = nullptr;
	ID3D11DeviceContext* m_DeviceContext = nullptr;
	ID3D11RenderTargetView* m_RenderTargetView = nullptr;

	bool m_Initialized = false;
	bool m_ImGuiInit = false;
	bool m_Open = false;

	WNDPROC m_OriginalWndProc = nullptr;
	HWND m_GameWindow = nullptr;
};
