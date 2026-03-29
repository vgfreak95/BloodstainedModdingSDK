#include "ToggleMods.h"
#include "Logger.h"
#include "GameManager.h"
#include "ProjectBlood_structs.hpp"


// UnlimitedStrength
void UnlimitedStrengthMod::OnActivated() {
  auto player = GameManager::Instance().Player();
  originalValue = player->CharacterParamaters.STR;
  player->CharacterParamaters.STR = 999.0f;
  Logger::Log("UnlimitedStrengthMod Activated!");
}

void UnlimitedStrengthMod::OnDeactivated() {
  auto player = GameManager::Instance().Player();
  player->CharacterParamaters.STR = originalValue;
  Logger::Log("UnlimitedStrengthMod Deactivated!");
}


// UnlimitedLuck
void UnlimitedLuckMod::OnActivated() {
  auto player = GameManager::Instance().Player();
  originalValue = player->CharacterParamaters.LUC;
  player->CharacterParamaters.LUC = 999.0f;
  Logger::Log("UnlimitedLuckMod Activated");
}

void UnlimitedLuckMod::OnDeactivated() {
  auto player = GameManager::Instance().Player();
  player->CharacterParamaters.LUC = originalValue;
  Logger::Log("UnlimitedLuckMod Deactivated!");
}

// UnlimitedInt
void UnlimitedIntMod::OnActivated() {
  auto player = GameManager::Instance().Player();
  originalValue = player->CharacterParamaters.INT;
  player->CharacterParamaters.INT = 999.0f;
  Logger::Log("UnlimitedIntMod Activated");
}

void UnlimitedIntMod::OnDeactivated() {
  auto player = GameManager::Instance().Player();
  player->CharacterParamaters.INT = originalValue;
  Logger::Log("UnlimitedIntMod Deactivated!");
}


// UnlimitedCon
void UnlimitedConMod::OnActivated() {
  auto player = GameManager::Instance().Player();
  originalValue = player->CharacterParamaters.CON;
  player->CharacterParamaters.CON = 999.0f;
  Logger::Log("UnlimitedConMod Activated");
}

void UnlimitedConMod::OnDeactivated() {
  auto player = GameManager::Instance().Player();
  player->CharacterParamaters.CON = originalValue;
  Logger::Log("UnlimitedConMod Deactivated!");
}


// UnlimitedMind
void UnlimitedMindMod::OnActivated() {
  auto player = GameManager::Instance().Player();
  originalValue = player->CharacterParamaters.MND;
  player->CharacterParamaters.MND = 999.0f;
  Logger::Log("UnlimitedMindMod Activated");
}

void UnlimitedMindMod::OnDeactivated() {
  auto player = GameManager::Instance().Player();
  player->CharacterParamaters.MND = originalValue;
  Logger::Log("UnlimitedMindMod Deactivated!");
}


// UnlimitedSpeed
void UnlimitedSpeedMod::OnActivated() {
  auto player = GameManager::Instance().Player();
  originalValue = player->CharacterInventory->GetSpecialAttribute(SDK::EPBEquipSpecialAttribute::NormalMoveUpRate);
  player->CharacterInventory->SetSpecialAttribute(SDK::EPBEquipSpecialAttribute::NormalMoveUpRate, 999.0f);
  Logger::Log("UnlimitedSpeedMod Activated");
}

void UnlimitedSpeedMod::OnDeactivated() {
  auto player = GameManager::Instance().Player();
  player->CharacterInventory->SetSpecialAttribute(SDK::EPBEquipSpecialAttribute::NormalMoveUpRate, originalValue);
  Logger::Log("UnlimitedSpeedMod Deactivated!");
}


