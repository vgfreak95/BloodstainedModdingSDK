#pragma once
#include "GameManager.h"

#include <Basic.hpp>
#include <ItemGetPopup_classes.hpp>
#include <PBInterfaceHUDBP_classes.hpp>
#include <PB_Chr_Root_classes.hpp>
#include <ProjectBlood_classes.hpp>
#include <ProjectBlood_structs.hpp>
#include <UnrealContainers.hpp>
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
    ProcessNamePool();
    Logger::Log("Game Manager POST initialized successfully");
    postInitCompleted = true;
    return true;
}

bool GameManager::IsInitialized() { return initCompleted && postInitCompleted ? true : false; }

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

void GameManager::GivePlayerItem(std::string name, bool shouldDisplay) {
    auto player = GameManager::Instance().Player();
    auto inventory = player->CharacterInventory;
    SDK::UPBGameInstance* inst = (SDK::UPBGameInstance*)GameManager::Instance().GameInstance();

    std::wstring wideName(name.begin(), name.end());
    auto itemName = SDK::UKismetStringLibrary::Conv_StringToName(wideName.c_str());
    ThreadQueue::Instance().Enqueue([itemName, inventory, shouldDisplay]() {
        SDK::FPBItemCatalogData itemData = SDK::FPBItemCatalogData();
        inventory->GetItemDataById(itemName, &itemData);
        inventory->GetItemWithDisplay(itemName, 1, shouldDisplay);

        // auto weaponsOffset = 0x960;
        // auto* manual = reinterpret_cast<uint8_t*>(inventory) + weaponsOffset;
        // auto* sdkDirect = reinterpret_cast<uint8_t*>(&inventory->myWeapons);
        //
        // std::cout << "manual:    " << (void*)manual << std::endl;
        // std::cout << "sdkDirect: " << (void*)sdkDirect << std::endl;
        //
        //
        // // Check sizeof your target type
        // std::cout << "FPBItemCatalogData size: " << sizeof(SDK::FPBItemCatalogData) << std::endl;
        //
        // if (itemData.itemCategory == SDK::ECarriedCatalog::Weapon) {
        //   // append to the weapons tarray
        //   Logger::Log("Added item to weapons:", inventory);
        //   auto weaponsOffset = 0x960;
        //
        //   auto** GMalloc = reinterpret_cast<void***>(0x7ff757480f38);
        //   auto* allocator = *GMalloc;
        //   auto UE_Malloc = reinterpret_cast<void*(*)(void*, size_t,
        //   uint32_t)>((*reinterpret_cast<void***>(allocator))[2]); auto UE_Free   = reinterpret_cast<void(*)(void*,
        //   void*)>((*reinterpret_cast<void***>(allocator))[4]);
        //
        //   auto count = inventory->myWeapons.Num();
        //   auto* myWeapons = reinterpret_cast<SDK::TArray<SDK::FPBItemCatalogData>*>(
        //       reinterpret_cast<uint8_t*>(inventory) + weaponsOffset
        //   );
        //
        //   // Allocate new buffer
        //   auto* buffer = static_cast<SDK::FPBItemCatalogData*>(
        //       UE_Malloc(allocator, sizeof(SDK::FPBItemCatalogData) * (count + 1), alignof(SDK::FPBItemCatalogData))
        //   );
        //
        //   // Copy existing weapons
        //   if (count > 0 && myWeapons->Data)
        //       memcpy(buffer, myWeapons->Data, sizeof(SDK::FPBItemCatalogData) * count);
        //
        //   // Append new item
        //   buffer[count] = itemData;
        //
        //   // Free old buffer
        //   if (myWeapons->Data && count > 0)
        //       UE_Free(allocator, myWeapons->Data);
        //
        //   // Point TArray at new buffer
        //   myWeapons->Data = buffer;
        //   myWeapons->NumElements = count + 1;
        //   myWeapons->MaxElements = count + 1;
        //
        //   std::cout << "weapons count now: " << myWeapons->Num() << std::endl;
        //
        //   inventory->GetItemWithDisplay(itemName, 1, true);
        //   Logger::Log("Hello");
    });
}
