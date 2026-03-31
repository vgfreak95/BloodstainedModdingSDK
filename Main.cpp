#include <Windows.h>
#include <minwindef.h>
#include <winuser.h>
#ifdef GetCurrentTime
#undef GetCurrentTime
#endif
#include <iostream>
#include <thread>
#include "version/version.h"

#include "Engine_classes.hpp"
#include "Mod/Logger.h"
#include "Mod/HookManager.h"
#include "Mod/GameManager.h"
#include "Mod/Gui.h"
#include "Mod/ModRunner.h"
#include "kiero.h"

long __stdcall HookedPresent(IDXGISwapChain* swapChain, unsigned int syncInterval, unsigned int flags);

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT __stdcall HookWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (Gui::Instance().IsOpen() && ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
	{
		return true;
	}
	return CallWindowProc((WNDPROC)Gui::Instance().GetOriginalWndProc(), hwnd, msg, wParam, lParam);
}

// Salvaged Code from Livestream
BOOL APIENTRY InitKieroAndHook() {
	HWND hwnd = Gui::Instance().GetGameWindow();
	if (hwnd) {
		WNDPROC originalWndProc = (WNDPROC)SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)HookWndProc);
		Gui::Instance().SetOriginalWndProc(originalWndProc);
		Logger::Log("WndProc Hooked");
	}

	kiero::init(kiero::RenderType::D3D11);

	if (kiero::bind(8, (void**)&Gui::originalPresent, (void*)HookedPresent) == kiero::Status::Success) {
		Logger::Log("Present hook installed");
		Gui::g_Hooked = true;
		return true;
	}
	else {
		Logger::Log("ERROR: Failed to hook Present");
		return false;
	}
}

void renderGui()
{
    if (!Gui::Instance().IsImGuiInit())
        return;

    Gui::Instance().Render();
}

long __stdcall HookedPresent(IDXGISwapChain* swapChain, unsigned int syncInterval, unsigned int flags)
{
    static bool init = false;
    if (!init)
    {
        init = true;
        Gui::Instance().InitImGui(swapChain);
        Logger::Log("Present hooked - GUI initialized");
    }

    renderGui();

    return Gui::originalPresent(swapChain, syncInterval, flags);
}

DWORD APIENTRY MainThread(HMODULE Module)
{
#ifdef _DEBUG
    char dllName[MAX_PATH];
    GetModuleFileNameA(Module, dllName, MAX_PATH);

    Logger::Init();
    Logger::Log("Starting Bloodstained Fun Mod");

    Gui::Instance().Init();

    if (!InitKieroAndHook())
        return 0;

    while (!GameManager::Instance().Init())
        Sleep(500);

    while (!HookManager::Instance().Init())
        Sleep(500);

    while (!GameManager::Instance().PostInit())
        Sleep(500);

    while (!HookManager::Instance().PostInit())
        Sleep(500);
    
    Logger::Log("Ready to Game!");

    // Gui::Instance().Shutdown();
    // kiero::shutdown();

#endif

    FreeLibraryAndExitThread(Module, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    std::thread* second;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        setupWrappers();
        second = new std::thread(MainThread, hModule);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
