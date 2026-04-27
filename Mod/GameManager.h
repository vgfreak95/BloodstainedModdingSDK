#pragma once

#include <ProjectBlood_structs.hpp>
#include <UnrealContainers.hpp>
#include <unordered_map>

#include "CoreUObject_classes.hpp"
#include "Engine_classes.hpp"
#include "PB_Chr_Root_classes.hpp"
#include "ProjectBlood_classes.hpp"

class GameManager {
   public:
    static GameManager& Instance();

    static constexpr float HP_INCREASE = 20.0f;
    static constexpr float MP_INCREASE = 10.0f;
    static constexpr int32_t CAPACITY_INCREASE = 5.0;

    static constexpr float DEFAULT_HP = 120.0f;
    static constexpr float DEFAULT_MP = 80.f;
    static constexpr int32_t DEFAULT_CAPACITY = 0;

    // Instances
    SDK::UWorld* World() const { return SDK::UWorld::GetWorld(); };
    SDK::UEngine* Engine() const { return SDK::UEngine::GetEngine(); };
    SDK::UGameInstance* GameInstance() const {
        return SDK::UGameplayStatics::GetGameInstance(GameManager::Instance().World());
    };
    SDK::APBPlayerController* PlayerController() const {
        return (SDK::APBPlayerController*)(GameManager::Instance().GameInstance()->LocalPlayers[0]->PlayerController);
    };
    SDK::APB_Chr_Root_C* Player() const { return (SDK::APB_Chr_Root_C*)PlayerController()->Pawn; };
    SDK::UPBRoomManager* RoomManager() const {
        return ((SDK::UPBGameInstance*)(GameManager::Instance().GameInstance()))->GetRoomManager();
    };

    bool Init();
    bool PostInit();
    bool IsInitialized();
    bool IsInstanceValid(SDK::UObject* object, const char* str);

    void GivePlayerItem(std::string name, bool shouldDisplay = true);
    void GivePlayerCoin(SDK::int32 amount, bool shouldDisplay = true);
    void GivePlayerMaxStatItem(std::string& maxStat, bool shouldDisplay = true);
    void SendInGameNotification(std::string notification, float iconId);
    bool IsPlayerDead() const { return isPlayerDead; };
    void PlayerDied() { isPlayerDead = true; };
    void PlayerAlive() { isPlayerDead = false; };


    void NotifyOnNewObject(const SDK::UObject* obj, SDK::UFunction* func, void* params);
    void ProcessEvent(const SDK::UObject* obj, SDK::UFunction* func, void* params);
    SDK::FName FindName(std::string name);

    bool IsPlayerLoadedInGame();
    SDK::FPBItemCatalogData* PlayerHasShard(SDK::FName shardId);
    std::unordered_map<std::string, UC::int32> NameLookup;
    std::unordered_map<UC::int32, SDK::EDropCoin> CoinLookup = {{50, SDK::EDropCoin::D50},
                                                                {100, SDK::EDropCoin::D100},
                                                                {500, SDK::EDropCoin::D500},
                                                                {1000, SDK::EDropCoin::D1000},
                                                                {2000, SDK::EDropCoin::D2000}};

   private:
    GameManager() = default;
    ~GameManager() = default;
    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;

    void ProcessNamePool();
    bool playerLoaded;
    bool initCompleted;
    bool postInitCompleted;
    bool isPlayerDead = false;
};
