#pragma once

#include <nlohmann/json.hpp>
#include <unordered_map>

#include "CoreUObject_classes.hpp"
#include "Engine_classes.hpp"
#include "PB_Chr_Root_classes.hpp"
#include "ProjectBlood_classes.hpp"
#include "StringTable.h"

class GameManager {
public:
  static GameManager& Instance();
  bool Init();
  bool PostInit();
  bool IsInstanceValid(SDK::UObject* object, const char* str);

  // Instances
  SDK::UWorld* World() const { return SDK::UWorld::GetWorld(); };
  SDK::UEngine* Engine() const { return SDK::UEngine::GetEngine(); };
  SDK::UGameInstance* GameInstance() const { return SDK::UGameplayStatics::GetGameInstance(GameManager::Instance().World()); };
  SDK::APBPlayerController* PlayerController() const { return (SDK::APBPlayerController*)(GameManager::Instance().GameInstance()->LocalPlayers[0]->PlayerController); };
  SDK::APB_Chr_Root_C* Player() const { return (SDK::APB_Chr_Root_C*)PlayerController()->Pawn; };
  SDK::UPBRoomManager* RoomManager() const { return ((SDK::UPBGameInstance*)(GameManager::Instance().GameInstance()))->GetRoomManager(); };

  std::string GetLocalizedString(const std::string& key) const
  {
      auto it = PBMasterStringTable.find(key);
      if (it != PBMasterStringTable.end())
          return it->second;
      return key; // fallback to key if not found
  }

  void NotifyOnNewObject(const SDK::UObject* obj, SDK::UFunction* func, void* params);
  void ProcessEvent(const SDK::UObject* obj, SDK::UFunction* func, void* params);
  void ClientRestart();

  bool IsPawnControlledByPlayer();

private:
  GameManager() = default;
  ~GameManager() = default;
  GameManager(const GameManager&) = delete;
  GameManager& operator=(const GameManager&) = delete;
  bool playerLoaded;
};

