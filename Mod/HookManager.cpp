#pragma once
#include "HookManager.h"

#include <Chr_P0000_classes.hpp>
#include <Engine_classes.hpp>
#include <PB_Chr_PlayerRoot_classes.hpp>
#include <PB_Chr_Root_classes.hpp>
#include <ProjectBlood_structs.hpp>
#include <Step_P0000_classes.hpp>
#include <UMG_classes.hpp>
#include <UnrealContainers.hpp>

#include "Basic.hpp"
#include "CoreUObject_classes.hpp"
#include "GameManager.h"
#include "ItemGetPopup_classes.hpp"
#include "Logger.h"
#include "Mod/Archipelago.h"
#include "ProjectBlood_classes.hpp"
#include "SDK.hpp"

std::set<std::string> HookManager::pendingWidgets;
std::set<std::string> HookManager::processedWidgets;

void (*HookManager::originalProcessEvent)(SDK::UObject*, SDK::UFunction*, void*) = nullptr;
void (*HookManager::originalProcessLocalScriptFunction)(SDK::UObject*, SDK::UFunction*, void*) = nullptr;
bool HookManager::playerDetected = false;
static int tickCount = 0;
std::unordered_set<std::string> sentLocationChecks;

bool HookManager::Init() {
    MH_Initialize();

    constexpr SDK::int32 ProcessLocalScriptFunction = 0x0674B520;
    void* processEventPtr = (void*)(SDK::InSDKUtils::GetImageBase() + SDK::Offsets::ProcessEvent);
    void* processLocalScriptFunctionPtr = (void*)(SDK::InSDKUtils::GetImageBase() + ProcessLocalScriptFunction);
    if (!processEventPtr) {
        Logger::Log(LogLevel::Error, "Failed to get ProcessEvent address");
        return false;
    }
    if (!processLocalScriptFunctionPtr) {
        Logger::Log(LogLevel::Error, "Failed to get processLocalScriptFunction address");
        return false;
    }

    MH_STATUS peStatus = MH_CreateHook(processEventPtr, &HOOKED_ProcessEvent, (void**)&originalProcessEvent);
    MH_STATUS plsfStatus = MH_CreateHook(processLocalScriptFunctionPtr, &HOOKED_ProcessLocalScriptFunction,
                                         (void**)&originalProcessLocalScriptFunction);
    if (peStatus != MH_OK) {
        Logger::Log(LogLevel::Error, "pe MH_CreateHook failed: ", (int)peStatus);
        return false;
    }
    if (plsfStatus != MH_OK) {
        Logger::Log(LogLevel::Error, "plsf MH_CreateHook failed: ", (int)plsfStatus);
        return false;
    }

    peStatus = MH_EnableHook(processEventPtr);
    if (peStatus != MH_OK) {
        Logger::Log(LogLevel::Error, "pe MH_EnableHook failed: ", (int)peStatus);
        return false;
    }
    plsfStatus = MH_EnableHook(processLocalScriptFunctionPtr);
    if (plsfStatus != MH_OK) {
        Logger::Log(LogLevel::Error, "plsf MH_EnableHook failed: ", (int)plsfStatus);
        return false;
    }

    // When game and player completely load in
    NotifyOnClassFunction("PBGameMode_Miriam_BP_C", "OnLoadGameCompletely", [](void* obj) {
        GameManager::Instance().PlayerAlive();
        APBridge::Instance().EnqueueSync();
        Logger::Log("Player respawned");
    });

    Logger::Log("HookManager initialized successfully");
    return true;
}

bool HookManager::PostInit() {
    // When player dies
    NotifyOnClassFunction("Chr_P0000_C", "Kill", [](void* obj) {
        GameManager::Instance().PlayerDied();
        sentLocationChecks.clear();
        Logger::Log("Player died");
    });

    // When player returns to title screen
    // IMPORTANT: Also when player dies and loading screen occurs, the title gets created for whatever reason
    NotifyOnClassFunction("PBTitlePlayerController_C", "ClientRestart", [](void* obj) {
        if (!GameManager::Instance().IsPlayerDead()) {
            Archipelago::ConnectedInstance()->ResetLocalIndex();
            APBridge::Instance().EnqueueDisconnect();
            sentLocationChecks.clear();
        }
        Logger::Log("Returned to title");
    });

    // When Bael is defeated
    NotifyOnClassFunction("Chr_N1013_Dominique_C", "BP_OnKilled",
                          [](void* obj) { Archipelago::Instance().BaelDefeated(); });

    // When player changes room
    NotifyOnClassFunction("PBRoomManager", "OnSerializeGame", [](void* obj) {
        std::string currentRoomId = GameManager::Instance().RoomManager()->GetCurrentRoomId().ToString();
        SDK::UPBGameInstance in;
        Logger::Log("Changed rooms:", currentRoomId);
    });

    // When player saves
    NotifyOnClassFunction("PBGameInstanceBP_C", "OnSaveStoryDataCompletedDelegates_Event_0", [](void* obj) {
        Logger::Log("Player saved game");
        Archipelago::ConnectedInstance()->UpdateServerLastIndex();
        sentLocationChecks.clear();
    });

    // When the shard that comes out of the enemy appears
    NotifyOnClassFunction("PurpleShard_C", "UserConstructionScript", [](void* obj) {
        SDK::TMap<SDK::FName, SDK::FPBShardCurrentNum> playerShards;

        auto shardBase = reinterpret_cast<SDK::AShardBase*>(obj);
        auto instance = GameManager::Instance;
        auto roomManager = instance().RoomManager();
        auto roomId = roomManager->GetCurrentRoomId().ToString();

        // Don't allow in tutorial room because softlock :/
        if (roomId != "m01SIP_000" && !instance().RoomIsBossRoom(roomId)) {
            instance().GivePlayerItem(shardBase->ShardId.ToString());
            ((SDK::AActor*)(shardBase))->K2_DestroyActor();
        }
    });

    // When the item popup in bottom right corner appears
    NotifyOnClassFunction("ItemGetPopup_C", "Tick", [](void* obj) {
        auto* popup = static_cast<SDK::UItemGetPopup_C*>(obj);
        std::string popupText = popup->MLTF_SIZE_23_ItemName->text.ToString();

        if (!popupText.starts_with("AP_")) return;

        std::string locationId = popupText.substr(3);
        if (sentLocationChecks.count(locationId)) return;

        sentLocationChecks.insert(locationId);
        Archipelago::ConnectedInstance()->SendLocationChecks(locationId);
        Logger::Log("[ItemGetPopup] Sending location check:", locationId);
    });

    Logger::Log("HookManager post initialized successfully");
    return true;
}
