#pragma once
#include "Archipelago.h"

#include <sys/stat.h>

#include <Basic.hpp>
#include <Engine_classes.hpp>
#include <PB_Chr_PlayerRoot_classes.hpp>
#include <ProjectBlood_classes.hpp>
#include <ProjectBlood_structs.hpp>
#include <UnrealContainers.hpp>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <vector>

#include "GameManager.h"
#include "Logger.h"
#include "apclient.hpp"
#include "apuuid.hpp"
#include "nlohmann/json_fwd.hpp"

std::unique_ptr<APClient> ap;
bool ap_sync_queued = false;
bool ap_connect_sent = false;
double deathtime = -1;
bool is_https = false;  // set to true if the context only supports wss://
bool is_wss = false;    // set to true if the connection specifically asked for wss://
bool is_ws = false;     // set to true if the connection specifically asked for ws://
unsigned connect_error_count = 0;
bool awaiting_password = false;

const std::string GAME_NAME = "Bloodstained";
std::string game_seed;
const int MAX_SEED_LENGTH = 32;

#define UUID_FILE "uuid"
#define CERT_STORE "cacert.pem"

using json = nlohmann::json;

Archipelago& Archipelago::Instance() {
    static Archipelago instance;
    return instance;
}

Archipelago::Archipelago()
    : state_(ArchipelagoConnectionState::Disconnected),
      retryTimer_(0.0),
      retryInterval_(10.0),
      maxRetries_(-1),
      retryCount_(0) {}

Archipelago::~Archipelago() {}

void Archipelago::ExecuteConsoleCommand(const char* command) { GameManager::Instance().GivePlayerItem(command); }

void Archipelago::ConnectSlot() {
    bool _connected = false;
    if (ap) {
        if (state_ == ArchipelagoConnectionState::Connected) {
            Logger::Log("[AP] Retrying connection (attempt", retryCount_, ")...");
            _connected = ap->ConnectSlot(slotName_, password_, itemsHandling_);
            if (_connected) {
                ap_sync_queued = true;
                Logger::Log("[AP] Connection from here was successful.");
                UpdateState(ArchipelagoConnectionState::SlotConnected);
                return;
            } else {
                Logger::Log("[AP] Max retries reached. Giving up.");
                currentUri_.clear();
            }
        }
    } else {
        Logger::Log(LogLevel::Debug, "[AP]", "Connection lost.");
    }
}

void Archipelago::SendLocationChecks(const std::string& locationId) {
    if (!ap || !IsConnected()) return;

    std::vector<int64_t> locations;

    // First check if chest is a single-item location (no .0 suffix)
    int64_t singleLocationId = ap->get_location_id(locationId);

    if (singleLocationId != APClient::INVALID_NAME_ID) {
        locations.push_back(singleLocationId);
        Logger::Log("[AP] Single item chest: ", locationId, " (ID: ", singleLocationId, ")");
    } else {
        // Multi-item chest - check for .0, .1, .2, .3
        for (int i = 0; i <= 3; i++) {
            std::string fullLocation = locationId + "." + std::to_string(i);
            int64_t locationId = ap->get_location_id(fullLocation);

            if (locationId != APClient::INVALID_NAME_ID) {
                locations.push_back(locationId);
                Logger::Log("[AP] Multi-item chest: ", fullLocation, " (ID: ", locationId, ")");
            } else {
                break;
            }
        }
    }

    // Send LocationScout and LocationCheck for all collected locations
    for (int64_t locationId : locations) {
        ap->LocationScouts({locationId}, 0);
        ap->LocationChecks({locationId});
        checkedLocations_.insert(locationId);
        missingLocations_.erase(locationId);
        Logger::Log("[AP] Sent scout & check for location: ", locationId);
    }
}

void Archipelago::Sync() {
    if (IsConnected()) {
        lastReceivedItemIndex_ = GetFileLastIndex();
        ap->Sync();
    }
}

int Archipelago::GetFileLastIndex() {
    indexFile.open("index.txt");
    std::string indexLine;

    // Only read first line
    if (indexFile.is_open()) {
        std::getline(indexFile, indexLine);
        indexFile.close();
    }

    int lastIndex = -1;
    try {
        if (!indexLine.empty()) lastIndex = std::stoi(indexLine);
    } catch (...) {
        Logger::Log("Invalid or empty index in index.txt; defaulting lastIndex to -1");
        lastIndex = -1;
    }
    return lastIndex;
}

void Archipelago::SetFileLastIndex() {
    int lastFileIndex = GetFileLastIndex();  // from file
    std::string fileName = "index.txt";
    std::ofstream indexFile(fileName, std::ios::out | std::ios::trunc);
    struct stat buf;
    bool fileExists = stat(fileName.c_str(), &buf) != -1;
    if (!fileExists) {
        if (!indexFile.is_open()) {
            Logger::Log("Index file was created");
        }
    }

    Logger::Log("Trying to save last index: ", lastFileIndex);

    if (lastReceivedItemIndex_ <= lastFileIndex) {
        Logger::Log("Bad index");
        return;
    } else {
        // Overwrite the contents of index.txt with the new index
        if (indexFile.is_open()) {
            Logger::Log("Overwriting last index");
            indexFile << lastReceivedItemIndex_ << '\n';
            indexFile.flush();
            indexFile.close();
        } else {
            Logger::Log("Failed to open index.txt for writing");
        }
    }
}

bool Archipelago::Connect(const std::string& slotName, const std::string& password, const std::string uri = "") {
    if (!GameManager::Instance().IsPlayerLoadedInGame()) {
        Logger::Log("Player is not loaded in game");
        return false;
    }

    slotName_ = slotName;
    password_ = password;
    currentUri_ = uri;

    retryTimer_ = 0.0;
    retryCount_ = 0;

    Logger::Log(LogLevel::Debug, "[AP]", "Connecting Player: ", slotName_, "with uri: ", uri);
    UpdateState(ArchipelagoConnectionState::Connecting);

    ap.reset();
    is_ws = uri.rfind("ws://", 0) == 0;
    is_wss = uri.rfind("wss://", 0) == 0;

    // remove scheme from URI for UUID generation and localhost-detection; UUID is per room this way
    std::string uri_without_scheme = uri.empty() ? APClient::DEFAULT_URI
                                     : is_ws     ? uri.substr(5)
                                     : is_wss    ? uri.substr(6)
                                                 : uri;

    std::string uuid = ap_get_uuid(UUID_FILE, uri_without_scheme);

    ap.reset(new APClient(uuid, GAME_NAME, uri.empty() ? APClient::DEFAULT_URI : uri, CERT_STORE));
    game_seed = ap->get_seed();

    ap_sync_queued = false;
    connect_error_count = 0;

    ap->set_socket_connected_handler([this]() {
        AbortPassword();
        UpdateState(ArchipelagoConnectionState::Connected);
        Logger::Log(LogLevel::Debug, "[AP]", "Player: ", slotName_, "connected to server successfully");
    });

    ap->set_room_info_handler([this]() {
        if (game_seed.empty()) {
            Logger::Log("[AP] Waiting for game...");
        } else if (strncmp(game_seed.c_str(), ap->get_seed().c_str(), MAX_SEED_LENGTH) != 0) {
            Logger::Log("Bad seed connection");
        }
        Logger::Log(LogLevel::Debug, "[AP]", "Room information retrieved successfully");
    });

    ap->set_print_handler([this](const std::string& msg) { Logger::Log("[AP] Raw print: ", msg); });

    ap->set_location_info_handler([this](const std::list<APClient::NetworkItem>& items) {
        Logger::Log("[AP] LocationInfo handler triggered!");
        for (const auto& item : items) {
            if (item.player == ap->get_player_number()) {
                Logger::Log("[AP] Item - player:", item.player, " location:", item.location, " item:", item.item,
                            "index:", item.index);

                std::string itemName = ap->get_item_name(item.item, ap->get_game());
                // Regardless of index give player these stats
                if (itemName.starts_with("Max")) {
                    GameManager::Instance().IncreasePlayerMaxStats(itemName);
                }
                Logger::Log("[AP] Giving item to player: ", itemName);
                GameManager::Instance().GivePlayerItem(itemName.c_str());
            } else {
                Logger::Log("[AP] Item belongs to other player, skipping");
            }
        }
    });

    ap->set_items_received_handler([this](const std::list<APClient::NetworkItem>& items) {
        if (!ap->is_data_package_valid()) {
            if (!ap_sync_queued) ap->Sync();
            ap_sync_queued = true;
            return;
        }

        int lastIndex = GetFileLastIndex();

        for (const auto& item : items) {
            Logger::Log("[AP] Item - player:", item.player, " location:", item.location, " item:", item.item);

            std::string itemName = ap->get_item_name(item.item, ap->get_game());

            Logger::Log(item.index, lastIndex);

            if (item.index <= lastIndex) continue;
            if (item.index <= lastReceivedItemIndex_) continue;
            if (itemName.starts_with("Max")) {
                GameManager::Instance().IncreasePlayerMaxStats(itemName);
                continue;
            }
            GameManager::Instance().GivePlayerItem(itemName.c_str());
        }

        Logger::Log("Saving last item index");
        auto lastItem = items.back();
        lastReceivedItemIndex_ = lastItem.index;
    });

    ap->set_data_package_changed_handler(
        [this](const json& data) { Logger::Log("[AP] Data package loaded for game: ", GAME_NAME); });

    ap->set_slot_connected_handler([this](const json& data) {
        // Store missing locations
        if (data.contains("missing_locations")) {
            for (const auto& loc : data["missing_locations"]) {
                missingLocations_.insert(loc.get<int64_t>());
            }
        }
        // Store checked locations
        if (data.contains("checked_locations")) {
            for (const auto& loc : data["checked_locations"]) {
                checkedLocations_.insert(loc.get<int64_t>());
            }
        }
        Logger::Log("[AP] Loaded ", missingLocations_.size(), " missing and ", checkedLocations_.size(),
                    " checked locations");

        // GameManager::Instance().ResetPlayerMaxStats();
    });

    // Handlers for disconnection
    ap->set_socket_disconnected_handler([this]() {
        if (state_ == ArchipelagoConnectionState::Disconnected) {
            Logger::Log(LogLevel::Debug, "[AP]", "socket disconnected");
            AbortPassword();
        }
    });
    ap->set_slot_disconnected_handler([this]() {
        Logger::Log(LogLevel::Debug, "[AP]", "Player: ", slotName_, "disconnected");
        UpdateState(ArchipelagoConnectionState::Disconnected);
        ap_connect_sent = false;
    });

    ap->set_slot_refused_handler([this](const std::list<std::string>& reasons_list) {
        Logger::Log("Slot:", slotName_, "couldn't connect");
        UpdateState(ArchipelagoConnectionState::ConnectionRefusedError);
    });

    ap->set_socket_error_handler([this, slotName](const std::string& error) {
        connect_error_count++;
        Logger::Log(LogLevel::Debug, "[AP]", "Player: ", slotName, "disconnected. Error: ", error);
        UpdateState(ArchipelagoConnectionState::SocketError);
    });
    return true;
}

void Archipelago::AbortPassword() { awaiting_password = false; }

void Archipelago::Disconnect() { UpdateState(ArchipelagoConnectionState::Disconnected); }

void Archipelago::Poll() {
    if (ap) ap->poll();
    if (state_ == ArchipelagoConnectionState::Connected && !ap_sync_queued) {
        ConnectSlot();
    }
}

void Archipelago::UpdateState(ArchipelagoConnectionState newState) {
    if (state_ == newState) return;
    state_ = newState;
}
