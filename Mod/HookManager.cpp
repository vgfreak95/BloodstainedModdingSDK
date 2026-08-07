#pragma once
#include "HookManager.h"

#include <Engine_classes.hpp>
#include <ProjectBlood_parameters.hpp>
#include <UMG_classes.hpp>
#include <UnrealContainers.hpp>

#include "Basic.hpp"
#include "GameManager.h"
#include "Gui.h"
#include "Logger.h"
#include "ShardCompat.h"
#include "Mod/Archipelago.h"
#include "ItemGetPopup_classes.hpp"
#include "ProjectBlood_classes.hpp"
#include "CoreUObject_classes.hpp"

std::set<std::string> HookManager::pendingWidgets;
std::set<std::string> HookManager::processedWidgets;

void (*HookManager::originalProcessEvent)(SDK::UObject*, SDK::UFunction*, void*) = nullptr;
void (*HookManager::originalProcessLocalScriptFunction)(SDK::UObject*, SDK::UFunction*, void*) = nullptr;
bool HookManager::playerDetected = false;


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
        Archipelago::Instance().ResetLocalLocationCache();
        Gui::Instance().TryAutoConnect();
        APBridge::Instance().EnqueueSync();
        Logger::Log("Player respawned");
    });

    Logger::Log("HookManager initialized successfully");
    return true;
}

bool HookManager::PostInit() {
    // Whens constantly when the player is alive
    NotifyOnClassFunction("Chr_P0000_C", "GetAdditionalCameraTargetLocations", [](void* obj) {
        if (!Archipelago::ConnectedInstance()) return;
        if (!Archipelago::ConnectedInstance()->IsPendingDeathlink()) return;

        Logger::Log("Pending death link is true");
        // is pending death link is true
        if (GameManager::Instance().CanKillPlayer() && !GameManager::Instance().IsPlayerDead()) {
            Logger::Log("Killing player from deathlink");
            GameManager::Instance().KillPlayer();
            Logger::Log("Killed player");
        }
    });

    // When player dies
    NotifyOnClassFunction("Chr_P0000_C", "Kill", [](void* obj) {
        Logger::Log("Player died");
        GameManager::Instance().PlayerDied();
        Logger::Log("Pending death link", Archipelago::ConnectedInstance()->IsPendingDeathlink());
        // pending death is true
        if (!Archipelago::ConnectedInstance()->IsPendingDeathlink()) {
            Logger::Log("Sent out death link");
            Archipelago::ConnectedInstance()->InvokeDeathLink();  // send out death link
        } else {
            Archipelago::ConnectedInstance()->ResetDeathLink();
            Logger::Log("Resetting death link");
        }
        Logger::Log("End of player died");
    });

    // When player returns to title screen
    // IMPORTANT: Also when player dies and loading screen occurs, the title gets created for whatever reason
    NotifyOnClassFunction("PBTitlePlayerController_C", "ClientRestart", [](void* obj) {
        if (!GameManager::Instance().IsPlayerDead()) {
            if (Archipelago::Instance().IsConnected()) APBridge::Instance().EnqueueDisconnect();
            Gui::Instance().ResetAutoConnect();
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
    });

    // When the shard that comes out of the enemy appears
    NotifyOnClassFunction("PurpleShard_C", "UserConstructionScript", [](void* obj) {
        auto shardBase = reinterpret_cast<SDK::AShardBase*>(obj);
        auto instance = GameManager::Instance;
        auto roomManager = instance().RoomManager();
        auto roomId = roomManager->GetCurrentRoomId().ToString();

        auto shardId = shardBase->ShardId;
        std::string shardName = shardId.ToString();
        if (!shardName.starts_with("AP_")) return;
        auto* archipelago = Archipelago::ConnectedInstance();
        if (!archipelago) return;

        auto vanillaShardId = ResolveVanillaShardId(shardId);
        if (!vanillaShardId) {
            Logger::Log("[Shard] Could not resolve randomized shard actor to a vanilla shard:", shardName);
            return;
        }

        ItemLookupResult itemResult = GetVanillaShardItemSupport(*archipelago, *vanillaShardId);
        if (itemResult == ItemLookupResult::NotReady) {
            Logger::Log("[Shard] Item data is not ready; leaving shard actor intact:", shardName);
            return;
        }
        if (itemResult == ItemLookupResult::UnknownItem) {
            RestoreVanillaShardAppearance(shardBase, *vanillaShardId);
            return;
        }

        LocationCheckResult checkResult = archipelago->SendLocationChecks(shardName);
        if (checkResult == LocationCheckResult::NotReady) {
            Logger::Log("[Shard] World data is not ready; leaving vanilla shard actor intact:", shardName);
            return;
        }

        if (checkResult == LocationCheckResult::UnknownLocation) {
            Logger::Log("[Shard] Server supports shard items but this shard has no location; suppressing actor:",
                        shardName);
        } else if (checkResult == LocationCheckResult::Sent) {
            instance().GivePlayerItem(shardName, false, 0);
            Logger::Log("[Shard] Sent location check:", shardName);
        }

        ((SDK::AActor*)(shardBase))->K2_DestroyActor();
        if (instance().RoomIsBossRoom(roomId)) instance().CheckBossSoftlock();
    });

    // When the item popup in bottom right corner appears
    NotifyOnClassFunction("ItemGetPopup_C", "Tick", [](void* obj) {
        auto* popup = static_cast<SDK::UItemGetPopup_C*>(obj);
        std::string popupText = popup->MLTF_SIZE_23_ItemName->text.ToString();

        if (!popupText.starts_with("AP_")) return;

        if (Archipelago::Instance().SendLocationChecks(popupText) == LocationCheckResult::Sent) {
            Logger::Log("[ItemGetPopup] Sent location check:", popupText);
        }
    });

    Logger::Log("HookManager post initialized successfully");
    return true;
}

void HookManager::ProcessEventBefore(SDK::UObject* obj, SDK::UFunction* func, void* params) {
    if (!obj || !func) return;
    std::string functionName = func->Name.GetRawString();

    // UserConstructionScript selects the shard actor's material. Restore the vanilla
    // ID before that script runs so compatibility shards use their real category color.
    if (functionName == "UserConstructionScript" && obj->Class->Name.ToString() == "PurpleShard_C") {
        auto* shardBase = static_cast<SDK::AShardBase*>(obj);
        std::string shardName = shardBase->ShardId.ToString();
        auto* archipelago = Archipelago::ConnectedInstance();
        if (!archipelago || !shardName.starts_with("AP_")) return;

        auto vanillaShardId = ResolveVanillaShardId(shardBase->ShardId);
        if (!vanillaShardId) return;

        ItemLookupResult itemResult = GetVanillaShardItemSupport(*archipelago, *vanillaShardId);
        if (itemResult != ItemLookupResult::UnknownItem) return;

        RestoreVanillaShardAppearance(shardBase, *vanillaShardId);
        return;
    }

    if (!params || functionName != "CrystallizeDead") return;

    auto* crystallizeParams = static_cast<SDK::Params::PBBaseCharacter_CrystallizeDead*>(params);
    std::string shardName = crystallizeParams->DropShardId.ToString();
    auto* archipelago = Archipelago::ConnectedInstance();
    if (!archipelago || !shardName.starts_with("AP_")) return;

    auto vanillaShardId = ResolveVanillaShardId(crystallizeParams->DropShardId);
    if (!vanillaShardId) {
        Logger::Log("[Shard] Could not resolve randomized shard before crystallization:", shardName);
        return;
    }

    ItemLookupResult itemResult = GetVanillaShardItemSupport(*archipelago, *vanillaShardId);
    if (itemResult != ItemLookupResult::UnknownItem) return;

    crystallizeParams->DropShardId = *vanillaShardId;
}
