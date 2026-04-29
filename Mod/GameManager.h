#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef _WIN64
#define WINVER 0x0600
#define _WIN32_WINNT 0x0600
#endif

#include <Windows.h>

#include <ProjectBlood_structs.hpp>
#include <UnrealContainers.hpp>
#include <unordered_map>
#include <unordered_set>

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
    bool PopulateDisplayToItemIdTable();

    void GivePlayerItem(const std::string& name, bool shouldDisplay = true);
    void GivePlayerCoin(SDK::int32 amount, bool shouldDisplay = true);
    void GivePlayerMaxStatItem(std::string& maxStat, bool shouldDisplay = true);

    std::optional<SDK::FPBItemCatalogData> PlayerHasItem(const SDK::TArray<SDK::FPBItemCatalogData>& itemsArray,
                                                         const std::string& itemName);
    std::optional<SDK::FPBItemCatalogData> CheckAllInventories(const std::string& itemName);

    void SendInGameNotification(std::string notification, float iconId);
    bool IsPlayerDead() const { return isPlayerDead; };
    void PlayerDied() { isPlayerDead = true; };
    void PlayerAlive() { isPlayerDead = false; };
    void CheckBossSoftlock();
    bool RoomIsBossRoom(const std::string& roomId) const { return bossRooms.count(roomId); };
    std::optional<std::string> GetIdFromDisplayName(const std::string& itemId);
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
    const std::unordered_set<std::string> bossRooms = {
        "m01SIP_000", "m09TRN_002", "m07LIB_011", "m08TWR_019", "m05SAN_023", "m18ICE_018",
    };
    std::unordered_map<std::string, std::string> DisplayNameToItemId;
};
