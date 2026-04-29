#pragma once
#include "GameManager.h"

#include <stringapiset.h>

#include <Basic.hpp>
#include <CoreUObject_structs.hpp>
#include <DataLoadList_classes.hpp>
#include <ItemGetPopup_classes.hpp>
#include <PBInterfaceHUDBP_classes.hpp>
#include <PB_Chr_Root_classes.hpp>
#include <ProjectBlood_classes.hpp>
#include <ProjectBlood_structs.hpp>
#include <UnrealContainers.hpp>
#include <algorithm>
#include <format>

#include "CoreUObject_classes.hpp"
#include "Engine_classes.hpp"
#include "Logger.h"
#include "ThreadQueue.h"
#include "Utils.h"

#define PLAYER_NAME "Chr_P0000_C_0"

GameManager& GameManager::Instance() {
    static GameManager instance;
    return instance;
}

bool GameManager::IsInstanceValid(SDK::UObject* object, const char* c_str) {
    if (!object) {
        std::string formatted = std::format("waiting for {} to initialize", c_str);
        Logger::Log(formatted);
        return false;
    }
    return true;
}

bool GameManager::IsPlayerLoadedInGame() {
    auto playerController = GameManager::Instance().PlayerController();
    if (playerController) {
        auto playerControllerName = playerController->GetName();
        if (playerControllerName == "PBTitlePlayerController_C_0")
            return false;
        else if (playerControllerName == "PBPlayerController_0")
            return true;
    }
}

bool GameManager::PopulateDisplayToItemIdTable() {
    auto itemTable = SDK::UPBDataTableManager::GetLoadedDataTable(SDK::EPBDataTables::ItemMaster);
    auto inventory = GameManager::Instance().Player()->CharacterInventory;

    for (auto& pair : itemTable->RowMap) {
        std::string itemId = pair.Key().ToString();
        auto itemName = FNameFromString(itemId);
        SDK::FPBItemCatalogData itemData = SDK::FPBItemCatalogData();
        inventory->GetItemDataById(itemName, &itemData);
        if (itemData.Name.ToString().empty()) continue;
        if (itemData.Name.ToString().starts_with("AP_")) continue;

        std::string displayName = itemData.Name.ToString();
        DisplayNameToItemId[displayName] = itemId;
    }
    return true;
}

std::optional<std::string> GameManager::GetIdFromDisplayName(const std::string& itemId) {
    auto it = DisplayNameToItemId.find(itemId);
    if (it == DisplayNameToItemId.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool GameManager::Init() {
    if (!GameManager::IsInstanceValid(GameManager::Engine(), "Engine")) return false;
    if (!GameManager::IsInstanceValid(GameManager::World(), "World")) return false;
    Logger::Log("Game Manager initialized successfully");
    initCompleted = true;
    return true;
}

bool GameManager::PostInit() {
    if (!GameManager::IsInstanceValid(GameManager::GameInstance(), "GameInstance")) return false;
    if (!GameManager::IsInstanceValid(GameManager::PlayerController(), "PlayerController")) return false;
    // Access ingame Table

    // Wait for the player to load in
    if (!GameManager::Instance().IsPlayerLoadedInGame()) return false;
    // ProcessNamePool();
    ThreadQueue::Instance().Enqueue([this] { GameManager::Instance().PopulateDisplayToItemIdTable(); });
    Logger::Log(DisplayNameToItemId["Knife"]);
    Logger::Log("Game Manager POST initialized successfully");
    postInitCompleted = true;
    return true;
}

bool GameManager::IsInitialized() { return initCompleted && postInitCompleted ? true : false; }

void GameManager::CheckBossSoftlock() {
    Logger::Log("Softlock fix triggered");
    auto instance = (SDK::UPBGameInstance*)GameManager::Instance().GameInstance();
    auto currentBoss = instance->CurrentBoss;

    if (currentBoss) {
        std::string boss = currentBoss->GetBossId().ToString();
        if ((boss == "N1003" || boss == "N2001" || boss == "N2013") && currentBoss->GetHitPoint() <= 0) {
            ThreadQueue::Instance().Enqueue([currentBoss]() { currentBoss->EndBossBattle(true); });
        }
        if (boss != "N2001") return;
        Sleep(2000);
        ThreadQueue::Instance().Enqueue([]() {
            auto instance = (SDK::UPBGameInstance*)GameManager::Instance().GameInstance();
            std::string none = "None";
            std::wstring wideName(none.begin(), none.end());
            auto noneName = SDK::UKismetStringLibrary::Conv_StringToName(wideName.c_str());

            std::string warpRoom = "m09TRN_003";
            std::wstring wideRoomName(warpRoom.begin(), warpRoom.end());
            auto warpName = SDK::UKismetStringLibrary::Conv_StringToName(wideName.c_str());
            Sleep(2000);
            Logger::Log("Warping player");
            instance->pRoomManager->Warp(warpName, false, false, noneName, {0, 0, 0, 0});
        });
    }
}

void GameManager::ProcessNamePool() {
    for (UC::uint32 i = 0; i < SDK::UObject::GObjects->Num(); i++) {
        SDK::UObject* obj = SDK::UObject::GObjects->GetByIndex(i);
        if (!obj) continue;

        std::string Name = obj->Name.ToString();
        UC::int32 Index = obj->Name.ComparisonIndex;

        NameLookup[Name] = Index;
    }
}

SDK::FName GameManager::FindName(std::string name) {
    auto it = NameLookup.find(name);
    if (it != NameLookup.end()) {
        ProcessNamePool();
        it = NameLookup.find(name);
    }

    if (it != NameLookup.end()) {
        SDK::FName foundName(it->second);
        return foundName;
    }

    Logger::Log("Item not found");
    return SDK::FName();
}

void GameManager::GivePlayerMaxStatItem(std::string& maxStat, bool shouldDisplay) {
    auto player = GameManager::Instance().Player();

    ThreadQueue::Instance().Enqueue([player, maxStat, shouldDisplay]() {
        std::wstring wideName = std::wstring(maxStat.begin(), maxStat.end());
        SDK::APBPlayerController* controller = (SDK::APBPlayerController*)GameManager::Instance().PlayerController();
        SDK::APBInterfaceHUDBP_C* hud = (SDK::APBInterfaceHUDBP_C*)controller->MyHUD;

        auto itemName = SDK::UKismetStringLibrary::Conv_StringToName(wideName.c_str());
        player->CharacterInventory->UseConsumable(itemName, false);
        if (shouldDisplay) {
            UC::FString displayString = FStringFromString(maxStat);
            hud->DisplayItemNameWindow(displayString, 1022);
        }
    });
}

void GameManager::GivePlayerCoin(SDK::int32 amount, bool shouldDisplay) {
    if (amount != 50 && amount != 100 && amount != 500 && amount != 1000 && amount != 2000) return;

    ThreadQueue::Instance().Enqueue([amount, shouldDisplay] {
        SDK::APBPlayerController* controller = (SDK::APBPlayerController*)GameManager::Instance().PlayerController();
        SDK::APBInterfaceHUDBP_C* hud = (SDK::APBInterfaceHUDBP_C*)controller->MyHUD;
        SDK::APB_Chr_Root_C* player = (SDK::APB_Chr_Root_C*)GameManager::Instance().Player();

        auto dropCoin = GameManager::Instance().CoinLookup[amount];
        auto coinIconId = player->CharacterInventory->GetCoinIcon(dropCoin);
        auto coinString = std::to_string(amount) + "G";

        if (shouldDisplay) {
            UC::FString displayString = FStringFromString(coinString);
            hud->DisplayItemNameWindow(displayString, coinIconId);
        }

        SDK::UPBGameInstance* instance = (SDK::UPBGameInstance*)GameManager::Instance().GameInstance();
        instance->AddTotalCoin(amount);
    });
}

void GameManager::SendInGameNotification(std::string notification, float iconId) {
    ThreadQueue::Instance().Enqueue([notification, iconId]() {
        SDK::APBPlayerController* controller = (SDK::APBPlayerController*)GameManager::Instance().PlayerController();
        SDK::APBInterfaceHUDBP_C* hud = (SDK::APBInterfaceHUDBP_C*)controller->MyHUD;
        SDK::APB_Chr_Root_C* player = (SDK::APB_Chr_Root_C*)GameManager::Instance().Player();

        UC::FString displayString = FStringFromString(notification);
        hud->DisplayItemNameWindow(displayString, iconId);
    });
}

std::optional<SDK::FPBItemCatalogData> GameManager::PlayerHasItem(
    const SDK::TArray<SDK::FPBItemCatalogData>& itemsArray, const std::string& itemName) {
    std::string lowerItemName = itemName;
    std::transform(lowerItemName.begin(), lowerItemName.end(), lowerItemName.begin(), ::tolower);

    for (auto& item : itemsArray) {
        std::string itemId = item.ID.ToString();
        std::transform(itemId.begin(), itemId.end(), itemId.begin(), ::tolower);
        if (itemId == lowerItemName) {
            Logger::Log("Player has item already");
            return item;
        }
    }
    return std::nullopt;
}

std::optional<SDK::FPBItemCatalogData> GameManager::CheckAllInventories(const std::string& itemName) {
    auto* player = static_cast<SDK::APB_Chr_Root_C*>(GameManager::Instance().Player());
    if (!player || !player->CharacterInventory) return std::nullopt;
    auto* inv = player->CharacterInventory;

    for (auto& arr : {inv->myWeapons, inv->myBullets, inv->myArmors, inv->myHeadGears, inv->myAccessories,
                      inv->myMufflers, inv->myConsumables, inv->myFoodstuffs, inv->myKeyItems, inv->myBooks,
                      inv->myTriggerShards, inv->myEffectiveShards, inv->myDirectionalShards, inv->myEnchantShards,
                      inv->myFamiliarShards, inv->mySkills}) {
        auto result = PlayerHasItem(arr, itemName);
        if (result.has_value()) return result;
    }

    return std::nullopt;
}

void GameManager::GivePlayerItem(const std::string& name, bool shouldDisplay) {
    auto player = GameManager::Instance().Player();
    auto inventory = player->CharacterInventory;
    SDK::UPBGameInstance* inst = (SDK::UPBGameInstance*)GameManager::Instance().GameInstance();

    auto itemName = FNameFromString(name);

    SDK::FPBItemCatalogData itemData = SDK::FPBItemCatalogData();

    auto itemInInventory = GameManager::Instance().CheckAllInventories(name);
    inventory->GetItemDataById(itemName, &itemData);

    if (itemInInventory.has_value()) {
        if (itemInInventory->Num == itemInInventory->MaxNum) {
            auto iconId = inventory->GetItemIcon(itemInInventory->ID);
            GameManager::Instance().SendInGameNotification("Limit reached: " + itemData.Name.ToString(), iconId);
        }
        Logger::Log("Player has", itemInInventory->Num, "of", name);
    }

    ThreadQueue::Instance().Enqueue([&itemData, itemName, inventory, shouldDisplay, name]() {
        auto instance = (SDK::UPBGameInstance*)GameManager::Instance().GameInstance();
        auto str = FStringFromString("ITEM_NAME_Headband");
        auto value = instance->pStringManager->GetStringFromKey(SDK::EPBStringTables::Master, str);

        Logger::Log(value.ToString());
        inventory->GetItemWithDisplay(itemName, 1, shouldDisplay);
    });
}
