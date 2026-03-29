#include "HookManager.h"
#include "CoreUObject_classes.hpp"
#include "Logger.h"
#include "GameManager.h"

std::set<std::string> HookManager::pendingWidgets;
std::set<std::string> HookManager::processedWidgets;
void(*HookManager::originalProcessEvent)(SDK::UObject*, SDK::UFunction*, void*) = nullptr;
bool HookManager::playerDetected = false;

bool HookManager::Init()
{
    MH_Initialize();
    
    void* processEventPtr = (void*)(SDK::InSDKUtils::GetImageBase() + SDK::Offsets::ProcessEvent);
    if (!processEventPtr)
    {
        Logger::Log(LogLevel::Error, "Failed to get ProcessEvent address");
        return false;
    }

    MH_STATUS status = MH_CreateHook(processEventPtr, &HOOKED_ProcessEvent, (void**)&originalProcessEvent);
    if (status != MH_OK)
    {
        Logger::Log(LogLevel::Error, "MH_CreateHook failed: ", (int)status);
        return false;
    }

    status = MH_EnableHook(processEventPtr);
    if (status != MH_OK)
    {
        Logger::Log(LogLevel::Error, "MH_EnableHook failed: ", (int)status);
        return false;
    }

    NotifyOnClassFunction("PBPlayerController", "ClientRestart", [](void* obj) {
      Logger::Log("Found player!");
      GameManager::Instance().ClientRestart();
    });

    Logger::Log("HookManager initialized successfully");
    return true;
}

bool HookManager::PostInit()
{
    NotifyOnClassFunction("ItemGetPopup_C", "Construct", [](void* obj) {
        auto* uobj = static_cast<SDK::UObject*>(obj);
        auto name = uobj->GetName();
        pendingWidgets.insert(name);
    });

    NotifyOnClassFunction("ItemGetPopup_C", "Tick", [](void* obj) {
        auto* uobj = static_cast<SDK::UObject*>(obj);
        auto name = uobj->GetName();

        if (pendingWidgets.find(name) == pendingWidgets.end())
            return;

        auto* popup = static_cast<SDK::UItemGetPopup_C*>(obj);
        if (popup->MLTF_SIZE_23_ItemName && popup->MLTF_SIZE_23_ItemName->text.TextData)
        {
            pendingWidgets.erase(name);
            processedWidgets.insert(name);
            Logger::Log("[ItemGetPopup] Item: ", popup->MLTF_SIZE_23_ItemName->text.ToString());
        }
    });

    // NotifyOnClassFunction("APBBronzeTreasureBox_BP_C", "OnChestOpened", [](void* obj) {
    //     auto* chest = static_cast<SDK::APBBronzeTreasureBox_BP_C*>(obj);
    //     Logger::Log("[ChestOpened] DropItemID: ", chest->DropItemID.ToString());
    // });
    //
    // NotifyOnClassFunction("APBGoldenTreasureBox_BP_C", "OnChestOpened", [](void* obj) {
    //     auto* chest = static_cast<SDK::APBGoldenTreasureBox_BP_C*>(obj);
    //     Logger::Log("[ChestOpened] DropItemID: ", chest->DropItemID.ToString());
    // });
    //

    Logger::Log("HookManager post initialized successfully");
    return true;
}
