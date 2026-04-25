#pragma once
#include "ModRunner.h"

#include <format>

#include "Bullet_PlayerRoot_classes.hpp"
#include "Engine_classes.hpp"
#include "GameManager.h"
#include "ItemGetPopup_classes.hpp"
#include "Logger.h"
#include "PB_Chr_Root_classes.hpp"
#include "ProjectBlood_classes.hpp"
#include "SDK.hpp"
#include "imgui.h"

ModRunner& ModRunner::Instance() {
    static ModRunner instance;
    return instance;
}

bool ModRunner::ModButton() {
    if (ImGui::Button("Modding button")) {
        Logger::Log("MOdding button pressed");
        return true;
    }
}

bool ModRunner::Start() {
    Logger::Log("Attempting to launch modrunner");

    auto pawn = GameManager::Instance().PlayerController()->Pawn;

    if (!GameManager::Instance().IsPlayerLoadedInGame()) return false;

    auto character = (SDK::APB_Chr_PlayerRoot_C*)(pawn);
    auto characterInventory = character->CharacterInventory;

    if (!character->CanPause()) return false;
    Sleep(2000);

    SDK::APBControllerBase base;

    character->CharacterParamaters.STR = 999.0f;

    // Set STR and LUC Buff here, don't use charParams
    characterInventory->SetSpecialAttribute(SDK::EPBEquipSpecialAttribute::BuffSTR, 1000.0f);
    characterInventory->SetSpecialAttribute(SDK::EPBEquipSpecialAttribute::BuffLUC, 1000.0f);
    characterInventory->SetSpecialAttribute(SDK::EPBEquipSpecialAttribute::BuffCON, 1000.0f);
    characterInventory->SetSpecialAttribute(SDK::EPBEquipSpecialAttribute::BuffMND, 1000.0f);
    characterInventory->SetSpecialAttribute(SDK::EPBEquipSpecialAttribute::BuffINT, 1000.0f);
    // characterInventory->SetSpecialAttribute(SDK::EPBEquipSpecialAttribute::, 1000.0f);

    // Setting Movement Speed and Back Step Speed, Check EPBEquipSpecialAttribute for more information
    characterInventory->SetSpecialAttribute(SDK::EPBEquipSpecialAttribute::NormalMoveUpRate, 1000.0f);
    characterInventory->SetSpecialAttribute(SDK::EPBEquipSpecialAttribute::BackStepRate, 1000.0f);

    auto playerState = (SDK::APBPlayerState*)character->PlayerState;

    // Fun God mode
    Logger::Log("Turning on God Mode...");
    playerState->bGodMode = true;

    // character->GetPBPlayerState();

    return true;
}
