#include <Windows.h>
#include <iostream>
#include <thread>
#include "version/version.h"

#include "Engine_classes.hpp"
#include "Mod/Logger.h"
#include "Mod/HookManager.h"

DWORD APIENTRY MainThread(HMODULE Module)
{
#ifdef _DEBUG
	char  dllName[MAX_PATH];
	GetModuleFileNameA(Module, dllName, MAX_PATH);

	Logger::Init();
	Logger::Log("Starting Bloodstained Fun Mod");

	while (!HookManager::Instance().Init())
		Sleep(500);

	Logger::Log("Ready to Game!");


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