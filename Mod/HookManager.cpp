#pragma once
#include "HookManager.h"

#include <Chr_P0000_classes.hpp>
#include <Engine_classes.hpp>
#include <PB_Chr_Root_classes.hpp>
#include <ProjectBlood_structs.hpp>
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

    Logger::Log("HookManager initialized successfully");
    return true;
}

bool HookManager::PostInit() {
    // When player dies
    NotifyOnClassFunction("PBGameMode_BP_C", "OnGameOver", [](void* obj) {
        GameManager::Instance().PlayerDied();
        Logger::Log("Player respawned");
    });

    // When player returns to title screen
    NotifyOnClassFunction("PBTitlePlayerController_C", "ClientRestart", [](void* obj) {
        if (!GameManager::Instance().IsPlayerDead()) {
            Archipelago::Instance().ResetLocalIndex();
            APBridge::Instance().EnqueueDisconnect();
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

    // When player dies and respawns
    NotifyOnClassFunction("PBPlayerController", "ClientRestart", [](void* obj) {
        GameManager::Instance().PlayerAlive();
        APBridge::Instance().EnqueueSync();
        Logger::Log("Player respawned");
    });

    // When player saves
    NotifyOnClassFunction("PBGameInstanceBP_C", "OnSaveStoryDataCompletedDelegates_Event_0", [](void* obj) {
        Logger::Log("Player saved game");
        Archipelago::Instance().SetFileLastIndex();
    });

    // When the shard that comes out of the enemy appears
    NotifyOnClassFunction("PurpleShard_C", "UserConstructionScript", [](void* obj) {
        SDK::TMap<SDK::FName, SDK::FPBShardCurrentNum> playerShards;

        auto shardBase = reinterpret_cast<SDK::AShardBase*>(obj);
        auto instance = GameManager::Instance;
        auto roomManager = instance().RoomManager();
        auto roomId = roomManager->GetCurrentRoomId().ToString();

        // Don't allow in tutorial room because softlock :/
        if (roomId != "m01SIP_000" && instance().RoomIsBossRoom(roomId)) {
            instance().GivePlayerItem(shardBase->ShardId.ToString());
            ((SDK::AActor*)(shardBase))->K2_DestroyActor();
        }

        // Check if player is softlocked
        // GameManager::Instance().CheckBossSoftlock();
    });

    // Code below I may need in the future :/
    // Future here, yeah I needed it

    // NotifyOnClassFunction("Chr_P0000_C", "GetAdditionalCameraTargetLocations", [](void* obj) {
    //     Logger::Log("Tick from player");
    //     Logger::Log(tickCount);
    //     if (++tickCount % 30 != 0) return;
    //     tickCount = 0;
    //
    //     auto* player = static_cast<SDK::AChr_P0000_C*>(obj);
    //     if (!player || !player->CharacterInventory) return;
    //
    //     for (SDK::FPBItemCatalogData& item : player->CharacterInventory->myKeyItems) {
    //         std::string itemId = item.ID.ToString();
    //         if (!itemId.starts_with("AP_")) continue;
    //         Archipelago::Instance().SendLocationChecks(itemId.substr(3));
    //     }
    //     // auto* uobj = static_cast<SDK::UObject*>(obj);
    //     // auto name = uobj->GetName();  // Use GetName to match what Tick uses
    //     // Logger::Log("[ItemGetPopup] Construct: ", name);
    //     // pendingWidgets.insert(name);
    //     // auto player = (SDK::AChr_P0000_C*)GameManager::Instance().Player();
    //     // SDK::UPBCharacterInventoryComponent* comp = player->CharacterInventory;
    // });

    NotifyOnClassFunction("ItemGetPopup_C", "Tick", [](void* obj) {
        auto* popup = static_cast<SDK::UItemGetPopup_C*>(obj);
        std::string popupText = popup->MLTF_SIZE_23_ItemName->text.ToString();

        if (!popupText.starts_with("AP_")) return;

        std::string locationId = popupText.substr(3);
        if (sentLocationChecks.count(locationId)) return;

        sentLocationChecks.insert(locationId);
        Archipelago::Instance().SendLocationChecks(locationId);
        Logger::Log("[ItemGetPopup] Sending location check:", locationId);
    });

    // When item display in bottom left corner is constructed
    // NotifyOnClassFunction("ItemGetPopup_C", "Construct", [](void* obj) {
    //     auto* uobj = static_cast<SDK::UObject*>(obj);
    //     auto name = uobj->GetName();  // Use GetName to match what Tick uses
    //     Logger::Log("[ItemGetPopup] Construct: ", name);
    //     pendingWidgets.insert(name);
    // });
    //
    // // When item display in bottom left corner is being shown to player
    // NotifyOnClassFunction("ItemGetPopup_C", "Tick", [](void* obj) {
    //     auto* uobj = static_cast<SDK::UObject*>(obj);
    //     auto name = uobj->GetName();
    //
    //     if (pendingWidgets.find(name) == pendingWidgets.end()) return;
    //
    //     // get text of popup
    //     auto* popup = static_cast<SDK::UItemGetPopup_C*>(obj);
    //     std::string popupText = popup->MLTF_SIZE_23_ItemName->text.ToString();
    //
    //     // Check if AP Item
    //     if (!popupText.starts_with("AP_")) {
    //         pendingWidgets.erase(name);
    //         processedWidgets.insert(name);
    //         return;
    //     };
    //
    //     if (popup->MLTF_SIZE_23_ItemName && popup->MLTF_SIZE_23_ItemName->text.TextData) {
    //         // DropItemID Name
    //         std::string dropItemId = popupText.substr(3);
    //         pendingWidgets.erase(name);
    //         processedWidgets.insert(name);
    //         Logger::Log("[ItemGetPopup] Item received: ", popupText);
    //         Logger::Log("Processing dropItemId:", dropItemId);
    //         Archipelago::Instance().SendLocationChecks(dropItemId);
    //     }
    //     Sleep(500);
    // });

    // When player opens treasure box (unused)
    // NotifyOnClassFunction(
    //     "PBEasyTreasureBox_BP_C",
    //     "BndEvt__PBET_OnInteractBox_K2Node_ComponentBoundEvent_4_PBETDelegate_OnInteract__DelegateSignature",
    //     [](void* obj) {
    //         SDK::FName* dropItemId = reinterpret_cast<SDK::FName*>(static_cast<uint8_t*>(obj) + 0x6F8);
    //         std::string dropItemIdStr = dropItemId->ToString();
    //
    //         Logger::Log("Opening chest: ", dropItemIdStr);
    //         // Archipelago::Instance().SendLocationChecksForChest(dropItemIdStr);
    //     });

    // When level loads and chest is detected (unused)
    // NotifyOnClassFunction("PBEasyTreasureBox_BP_C", "OnLevelLoaded", [](void* obj) {
    //     auto treasure = reinterpret_cast<SDK::APBBronzeTreasureBox_BP_C*>(obj);
    //     auto droppedItems = (SDK::TArray<SDK::FPBDroppedItem>)(treasure->DroppedItems);
    //
    //     Logger::Log("Level loaded for treasurechest:", treasure->DropItemID.ToString());
    //     for (auto item : droppedItems) {
    //         Logger::Log(item.ItemID.ToString());
    //     }
    // });

    // When player collects MaxHP (unused)
    // NotifyOnClassFunction(
    //     "HPMaxUp_C",
    //     "BndEvt__PBET_OnOverlapPCBox_K2Node_ComponentBoundEvent_0_PBETDelegate_OnOverlapPC__DelegateSignature",
    //     [](void* obj) {
    //         SDK::UObject* uobj = (SDK::UObject*)obj;
    //
    //         auto hasCollected = reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(obj) + 0x7C4);
    //         Logger::Log("Player has collected:", hasCollected);
    //         Logger::Log("Player has collected:", *hasCollected);
    //
    //         auto roomManager = GameManager::Instance().RoomManager();
    //         std::string formatted = uobj->GetName() + "." + roomManager->GetCurrentRoomId().ToString();
    //         Logger::Log("Picked up:", formatted);
    //     });

    // When player collects MaxMP (unused)
    // NotifyOnClassFunction(
    //       "MPMaxUp_C",
    //       "BndEvt__PBET_OnOverlapPCBox_K2Node_ComponentBoundEvent_0_PBETDelegate_OnOverlapPC__DelegateSignature",
    //       [](void* obj) {
    //           SDK::UObject* uobj = (SDK::UObject*)obj;
    //           auto hasCollected = reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(obj) + 0x7C4);
    //           Logger::Log("Player has collected:", hasCollected);
    //           Logger::Log("Player has collected:", *hasCollected);
    //
    //           auto roomManager = GameManager::Instance().RoomManager();
    //           std::string formatted = uobj->GetName() + "." + roomManager->GetCurrentRoomId().ToString();
    //           Logger::Log("Picked up:", formatted);
    //       });

    // When player collects MaxCapacity (unused)
    // NotifyOnClassFunction(
    //       "BulletMaxUp_C",
    //       "BndEvt__PBET_OnOverlapPCBox_K2Node_ComponentBoundEvent_0_PBETDelegate_OnOverlapPC__DelegateSignature",
    //       [](void* obj) {
    //           SDK::UObject* uobj = (SDK::UObject*)obj;
    //           auto hasCollected = reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(obj) + 0x7C4);
    //           Logger::Log("Player has collected:", hasCollected);
    //           Logger::Log("Player has collected:", *hasCollected);
    //
    //           auto roomManager = GameManager::Instance().RoomManager();
    //           std::string formatted = uobj->GetName() + "." + roomManager->GetCurrentRoomId().ToString();
    //           Logger::Log("Picked up:", formatted);
    //       });

    Logger::Log("HookManager post initialized successfully");
    return true;
}
