#include "CoreUObject_classes.hpp"
#include "Engine_classes.hpp"
#include "ItemGetPopup_classes.hpp"
#include "SDK.hpp"
#include "Logger.h"
#include "GameManager.h"

#include <format>

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

void GameManager::ClientRestart() {
  if (playerLoaded) {
    playerLoaded = false;
  } else {
    playerLoaded = true;
  }
}

bool GameManager::IsPawnControlledByPlayer() {
  if (playerLoaded) return true;
  return false;
}

bool GameManager::Init()
{
  if(!GameManager::IsInstanceValid(GameManager::Engine(), "Engine")) return false;
  if(!GameManager::IsInstanceValid(GameManager::World(), "World")) return false;

  Logger::Log("Game Manager initialized successfully");
  return true;
}

bool GameManager::PostInit() {
  if(!GameManager::IsInstanceValid(GameManager::GameInstance(), "GameInstance")) return false;
  if(!GameManager::IsInstanceValid(GameManager::PlayerController(), "PlayerController")) return false;
  // Access ingame Table

	// Wait for the player to load in
  if(!playerLoaded) return false;

  Logger::Log("Game Manager POST initialized successfully");
}
