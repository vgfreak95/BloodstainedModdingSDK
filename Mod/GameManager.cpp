#pragma once
#include "GameManager.h"

#include <Basic.hpp>
#include <PBGameInstanceBP_classes.hpp>
#include <ProjectBlood_classes.hpp>
#include <ProjectBlood_structs.hpp>
#include <UnrealContainers.hpp>
#include <format>
#include <fstream>

#include "CommonMenuStatus_classes.hpp"
#include "CoreUObject_classes.hpp"
#include "Engine_classes.hpp"
#include "ItemGetPopup_classes.hpp"
#include "Logger.h"
#include "SDK.hpp"
#include "ThreadQueue.h"

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
    auto playerControllerName = playerController->GetName();
    if (playerControllerName == "PBTitlePlayerController_C_0")
        return false;
    else if (playerControllerName == "PBPlayerController_0")
        return true;
}

bool GameManager::Init() {
    if (!GameManager::IsInstanceValid(GameManager::Engine(), "Engine")) return false;
    if (!GameManager::IsInstanceValid(GameManager::World(), "World")) return false;
    Logger::Log("Game Manager initialized successfully");
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

void GameManager::IncreasePlayerMaxStats(std::string& maxStat) {
    auto player = GameManager::Instance().Player();

    ThreadQueue::Instance().Enqueue([player, maxStat]() {
        std::wstring wideName;
        if (maxStat == "Max_HP") {
            std::string name = "MaxHPUP";
            wideName = std::wstring(name.begin(), name.end());
        } else if (maxStat == "Max_MP") {
            std::string name = "MaxMPUP";
            wideName = std::wstring(name.begin(), name.end());
        } else if (maxStat == "Max_Capacity") {
            std::string name = "MaxBulletUp";
            wideName = std::wstring(name.begin(), name.end());
        }
        auto itemName = SDK::UKismetStringLibrary::Conv_StringToName(wideName.c_str());
        player->CharacterInventory->UseConsumable(itemName, false);
    });
}

void GameManager::GivePlayerItem(std::string name) {
    auto player = GameManager::Instance().Player();
    auto inventory = player->CharacterInventory;

    std::wstring wideName(name.begin(), name.end());
    auto itemName = SDK::UKismetStringLibrary::Conv_StringToName(wideName.c_str());
    ThreadQueue::Instance().Enqueue([itemName, inventory]() {
        SDK::FPBItemCatalogData itemData = SDK::FPBItemCatalogData();
        inventory->GetItemDataById(itemName, &itemData);
        inventory->GetItemWithDisplay(itemName, 1, true);

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
