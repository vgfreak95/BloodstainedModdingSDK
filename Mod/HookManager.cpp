#include "HookManager.h"
#include "Basic.hpp"
#include "CoreUObject_classes.hpp"
#include "ItemGetPopup_classes.hpp"
#include "Logger.h"
#include "GameManager.h"
#include "ProjectBlood_classes.hpp"

std::set<std::string> HookManager::pendingWidgets;
std::set<std::string> HookManager::processedWidgets;
void(*HookManager::originalProcessEvent)(SDK::UObject*, SDK::UFunction*, void*) = nullptr;
bool HookManager::playerDetected = false;

bool HookManager::Init()
{
    SDK::UPBGameInstance instance;
    SDK::UItemGetPopup_C popup;
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

    // NotifyOnClassFunction("ItemGetPopup_C", "Construct", [](void* obj) {
    //     auto* uobj = static_cast<SDK::UObject*>(obj);
    //     auto name = uobj->GetName();
    //     Logger::Log(LogLevel::Debug, "Constructed: ", name);
    //     pendingWidgets.insert(name);
    // });

    NotifyOnClassFunction("PBPlayerController", "ClientRestart", [](void* obj) {
      Logger::Log("Found player!");
      GameManager::Instance().ClientRestart();
    });
  Logger::Log("HookManager initialized successfully");
  return true;
}

bool HookManager::PostInit()
{
  NotifyOnClassFunction("PBEasyTreasureBox_BP_C", "BndEvt__PBET_OnInteractBox_K2Node_ComponentBoundEvent_4_PBETDelegate_OnInteract__DelegateSignature", [](void* obj) {

    SDK::FName* dropItemId = reinterpret_cast<SDK::FName*>(static_cast<uint8_t*>(obj) + 0x6F8);
    auto roomManager = GameManager::Instance().RoomManager();
    std::string formatted = dropItemId->ToString() + "." + roomManager->GetCurrentRoomId().ToString();
    Logger::Log("Pickup up: ", formatted);
  });

  NotifyOnClassFunction("HPMaxUp_C", "BndEvt__PBET_OnOverlapPCBox_K2Node_ComponentBoundEvent_0_PBETDelegate_OnOverlapPC__DelegateSignature", [](void* obj) {
    SDK::UObject* uobj = (SDK::UObject*)obj;
    auto roomManager = GameManager::Instance().RoomManager();
    std::string formatted = uobj->GetName() + "." + roomManager->GetCurrentRoomId().ToString();
    Logger::Log("Picked up:", formatted);

  });

  NotifyOnClassFunction("MPMaxUp_C", "BndEvt__PBET_OnOverlapPCBox_K2Node_ComponentBoundEvent_0_PBETDelegate_OnOverlapPC__DelegateSignature", [](void* obj) {
    SDK::UObject* uobj = (SDK::UObject*)obj;
    auto roomManager = GameManager::Instance().RoomManager();
    std::string formatted = uobj->GetName() + "." + roomManager->GetCurrentRoomId().ToString();
    Logger::Log("Picked up:", formatted);
  });

  NotifyOnClassFunction("BulletMaxUp_C", "BndEvt__PBET_OnOverlapPCBox_K2Node_ComponentBoundEvent_0_PBETDelegate_OnOverlapPC__DelegateSignature", [](void* obj) {
    SDK::UObject* uobj = (SDK::UObject*)obj;
    auto roomManager = GameManager::Instance().RoomManager();
    std::string formatted = uobj->GetName() + "." + roomManager->GetCurrentRoomId().ToString();
    Logger::Log("Picked up:", formatted);
  });

  NotifyOnClassFunction("PurpleShard_C", "ReceiveEndPlay", [](void* obj){
    auto shardBase = reinterpret_cast<SDK::AShardBase*>(obj);
    std::string shardNameLookup = "SHARD_NAME_" + shardBase->ShardId.ToString();
    auto shardName = GameManager::Instance().GetLocalizedString(shardNameLookup);
    Logger::Log("Picked up:", shardName);
  });

  // Code below I may need in the future :/

  // NotifyOnClassFunction("ItemGetPopup_C", "Contruct", [](void* obj) {
  //     Logger::Log("Called OnInitialized");
  //       auto* uobj = static_cast<SDK::UObject*>(obj);
  //       auto name = uobj->GetFullName();
  //       pendingWidgets.insert(name);
  // });
  //
  //
  //   NotifyOnClassFunction("ItemGetPopup_C", "Tick", [](void* obj) {
  //       auto* uobj = static_cast<SDK::UObject*>(obj);
  //       auto name = uobj->GetName();
  //       //Logger::Log("From: ", name);
  //
  //       if (pendingWidgets.find(name) == pendingWidgets.end())
  //           return;
  //
  //       auto* popup = static_cast<SDK::UItemGetPopup_C*>(obj);
  //       if (popup->MLTF_SIZE_23_ItemName && popup->MLTF_SIZE_23_ItemName->text.TextData)
  //       {
  //           pendingWidgets.erase(name);
  //           processedWidgets.insert(name);
  //           Logger::Log("[ItemGetPopup] Item: ", popup->MLTF_SIZE_23_ItemName->text.ToString());
  //       }
  //   });

    Logger::Log("HookManager post initialized successfully");
    return true;
}
