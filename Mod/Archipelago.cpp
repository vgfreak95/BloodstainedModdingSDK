#pragma once
#include "Archipelago.h"

#include <Basic.hpp>
#include <Engine_classes.hpp>
#include <PB_Chr_Root_classes.hpp>
#include <ProjectBlood_classes.hpp>
#include <ProjectBlood_structs.hpp>
#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>
#include <vector>

#include "GameManager.h"
#include "Logger.h"
#include "Utils.h"
#include "apclient.hpp"
#include "apuuid.hpp"
#include "nlohmann/detail/value_t.hpp"

std::unique_ptr<APClient> ap;
bool ap_sync_queued = false;
bool ap_connect_sent = false;
double deathtime = -1;
bool is_https = false;  // set to true if the context only supports wss://
bool is_wss = false;    // set to true if the connection specifically asked for wss://
bool is_ws = false;     // set to true if the connection specifically asked for ws://
unsigned connect_error_count = 0;
bool awaiting_password = false;

const std::string GAME_NAME = "Bloodstained: Ritual of the Night";
std::string game_seed;
const int MAX_SEED_LENGTH = 32;

#define UUID_FILE "uuid"
#define CERT_STORE "cacert.pem"

using json = nlohmann::json;

Archipelago& Archipelago::Instance() {
    static Archipelago instance;
    return instance;
}

Archipelago* Archipelago::ConnectedInstance() {
    if (!ap || !Instance().IsConnected()) {
        return nullptr;
    }
    return &Instance();
}

Archipelago::Archipelago() : state_(ArchipelagoConnectionState::Disconnected) {}

Archipelago::~Archipelago() {}

void Archipelago::ExecuteConsoleCommand(const char* command) { GameManager::Instance().GivePlayerItem(command); }

void Archipelago::ConnectSlot() {
    bool _connected = false;
    if (ap) {
        if (state_ == ArchipelagoConnectionState::Connected) {
            std::list<std::string> tags;
            if (wantsDeathlink_) {
                tags.push_back("DeathLink");
            }
            _connected = ap->ConnectSlot(slotName_, password_, itemsHandling_, tags);
            if (_connected) {
                SlotDataStore_.startingInventoryLockedSlot =
                    "slot:" + std::to_string(playerSlot_) + ":lock_starting_inventory";
                SlotDataStore_.lastIndexSlot = "slot:" + std::to_string(playerSlot_) + ":index";

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
    if (!ap || !IsConnected() || !ap->is_data_package_valid()) return;

    std::vector<int64_t> locations;


    if (!locationId.starts_with("AP_")) return;
    std::string locationWithoutPrefix = locationId.substr(3);

    // Is it a single-item location (no .0 suffix)
    int64_t apLocationId = ap->get_location_id(locationWithoutPrefix);

    if (apLocationId != APClient::INVALID_NAME_ID) {
        locations.push_back(apLocationId);
        Logger::Log("[AP] Single item: ", locationWithoutPrefix, " (ID: ", apLocationId, ")");
    } else {
        // Multi-item chest - check for .0, .1, .2, .3
        for (int i = 0; i <= 3; i++) {
            // Treasurebox.0
            std::string fullLocation = locationWithoutPrefix + "." + std::to_string(i);
            int64_t apLocationId = ap->get_location_id(fullLocation);

            if (apLocationId != APClient::INVALID_NAME_ID) {
                locations.push_back(apLocationId);
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
        Logger::Log("[AP] Sent scout & check for location: ", locationId);
    }
}

void Archipelago::Sync() {
    if (IsConnected() && ap) {
        queueLastIndexReset_ = true;
        ap->Sync();
    }
}

void Archipelago::UpdateServerLastIndex() {
    if (!ap) return;
    std::string key = SlotDataStore_.lastIndexSlot;
    json defaultValue = {{"value", -1}};
    std::list<APClient::DataStorageOperation> replaceOpts = {
        {.operation = "replace", .value = {{"value", lastReceivedItemIndex_}}}};
    ap->Set(key, defaultValue, false, replaceOpts);
}

void Archipelago::LockStartingInventory() {
    if (!ap) return;
    if (!wantsStartingInventory_) return;
    Logger::Log("Locking starting inventory on server");
    std::string key = SlotDataStore_.startingInventoryLockedSlot;
    json defaultValue = {{"value", -1}};
    std::list<APClient::DataStorageOperation> replaceOpts = {{.operation = "replace", .value = {{"value", 1}}}};
    ap->Set(key, defaultValue, false, replaceOpts);
}

std::string Archipelago::GetStateAsString() {
    auto state = Archipelago::Instance().GetConnectionState();
    switch (state) {
        case ArchipelagoConnectionState::Disconnected:
            return "Disconnected";
        case ArchipelagoConnectionState::Connecting:
            return "Connecting";
        case ArchipelagoConnectionState::Connected:
            return "Connected";
        case ArchipelagoConnectionState::SlotConnected:
            return "Slot Connected";
        case ArchipelagoConnectionState::SocketDisconnectedError:
            return "Socket Disconnected";
        case ArchipelagoConnectionState::ConnectionRefusedError:
            return "Connection Refused";
        case ArchipelagoConnectionState::InvalidSlotError:
            return "Invalid Slot";
        default:
            return "Unknown";
    }
}

void Archipelago::BaelDefeated() {
    auto goalStatus = APClient::ClientStatus::GOAL;
    if (ap) {
        ap->StatusUpdate(goalStatus);
    }
}

void Archipelago::InvokeDeathLink() {
    if (!ap) return;
    if (!wantsDeathlink_) return;
    deathtime = ap->get_server_time();
    int causeOfDeathNum = std::rand() % deathReasons_.size();
    std::list<std::string>::iterator it = deathReasons_.begin();
    advance(it, causeOfDeathNum);
    json data{
        {"time", deathtime},
        {"cause", *it},
        {"source", ap->get_slot()},
    };
    ap->Bounce(data, {}, {}, {"DeathLink"});
    ap->poll();
}

void Archipelago::GivePlayerItem(std::string& itemName, bool shouldDisplay) {
    auto instance = GameManager::Instance;
    Logger::Log("Giving player item:", itemName);
    if (itemName.starts_with("Max")) {
        instance().GivePlayerMaxStatItem(itemName, shouldDisplay);
        return;
        // Check for the bit coins (8 bit coin etc)
    } else if (std::isdigit(itemName[0]) && !itemName.starts_with("8") && !itemName.starts_with("16") &&
               !itemName.starts_with("32")) {
        int amount = std::stoi(itemName.substr(0, itemName.size() - 1));
        instance().GivePlayerCoin(amount, shouldDisplay);
        return;
    }

    auto itemId = GameManager::Instance().GetIdFromDisplayName(itemName);
    if (itemId.has_value()) {
        if (instance().ItemHasItemCategories(
                itemId.value(), {SDK::ECarriedCatalog::AllShard, SDK::ECarriedCatalog::TriggerShard,
                                 SDK::ECarriedCatalog::DirectionalShard, SDK::ECarriedCatalog::EffectiveShard,
                                 SDK::ECarriedCatalog::EnchantShard, SDK::ECarriedCatalog::FamiliarShard})) {
            // Shards don't normally show in chat, here we override
            instance().GivePlayerItem(itemId.value(), true, shardDropInitialGrade_);
            return;
        }
    }

    if (itemId.has_value()) {
        instance().GivePlayerItem(itemId.value(), shouldDisplay);
    } else {
        instance().GivePlayerItem(itemName.c_str(), shouldDisplay);
    }
}

bool Archipelago::Connect(const std::string& slotName, const std::string& password, const std::string uri = "",
                          const bool& wantsDeathlink = false) {
    if (!GameManager::Instance().IsPlayerLoadedInGame()) {
        Logger::Log("Player is not loaded in game");
        return false;
    }

    slotName_ = slotName;
    password_ = password;
    currentUri_ = uri;
    wantsDeathlink_ = wantsDeathlink;
    queueLastIndexReset_ = false;

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
        for (const auto& item : items) {
            if (item.player == ap->get_player_number()) {
                std::string itemName = ap->get_item_name(item.item, ap->get_game());
                Logger::Log("[AP] Item - player:", item.player, " location:", item.location, " item:", item.item,
                            "index:", item.index);
                Archipelago::GivePlayerItem(itemName, true);
            } else {
                std::string otherPlayer = ap->get_player_alias(item.player);
                std::string itemName = ap->get_item_name(item.item, ap->get_player_game(item.player));
                std::string otherPlayerItemNotification = "Sent: " + itemName + ", To: " + otherPlayer;
                GameManager::Instance().SendInGameNotification(otherPlayerItemNotification, 1023);
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
        items_ = items;
        if (ap->Get({SlotDataStore_.lastIndexSlot})) {
            checkingIndex_ = true;
            Logger::Log("successfully invoked get");
        }
    });

    ap->set_retrieved_handler([this](const std::map<std::string, json>& data) {
        if (checkingIndex_) {
            auto indexData = data.at(SlotDataStore_.lastIndexSlot);
            if (indexData == nlohmann::detail::value_t::null) {
                Logger::Log("[ReceivedHandler] index was null, setting index on server to -1");
                UpdateServerLastIndex();
                ap->Get({SlotDataStore_.lastIndexSlot});
            } else {
                auto indexValue = indexData.at("value");

                if (queueLastIndexReset_) {
                    lastReceivedItemIndex_ = indexValue;
                    queueLastIndexReset_ = false;
                }

                for (const auto& item : items_) {
                    Logger::Log("[AP] Item - player:", item.player, " location:", item.location, " item:", item.item);

                    std::string itemId = ap->get_item_name(item.item, ap->get_game());

                    Logger::Log("Item Index:", item.index, "File Index:", indexValue, "RAM Index",
                                lastReceivedItemIndex_);

                    if (item.index <= indexValue) continue;
                    if (item.index <= lastReceivedItemIndex_) continue;

                    auto player = (SDK::APB_Chr_Root_C*)GameManager::Instance().Player();
                    SDK::FPBItemCatalogData itemData = SDK::FPBItemCatalogData();

                    std::wstring wideName(itemId.begin(), itemId.end());
                    auto itemName = SDK::UKismetStringLibrary::Conv_StringToName(wideName.c_str());
                    player->CharacterInventory->GetItemDataById(itemName, &itemData);

                    std::string receivedItemsNotification =
                        "Item: " + itemData.Name.ToString() + ", From: " + ap->get_player_alias(item.player);

                    if (itemData.Name.ToString().empty()) {
                        receivedItemsNotification =
                            "Item: " + itemName.ToString() + ", From: " + ap->get_player_alias(item.player);
                    }

                    // send notification and don't display the item (implied by notification)
                    GameManager::Instance().SendInGameNotification(receivedItemsNotification, 1023);
                    Archipelago::GivePlayerItem(itemId, false);
                }
                Logger::Log("Saving last item index");
                auto lastItem = items_.back();
                lastReceivedItemIndex_ = lastItem.index;
            }
            checkingIndex_ = false;
        }
        Logger::Log("Ending retrieved handler");

        if (checkingStartInventory_) {
            if (data.contains(SlotDataStore_.startingInventoryLockedSlot) &&
                !data.at(SlotDataStore_.startingInventoryLockedSlot).is_null())
                return;
            if (!recievedStartingInventory_ && wantsStartingInventory_) {
                Logger::Log("Giving player start inventory");
                auto startItems = slotData_.at("start_inventory");
                for (auto& [item, amount] : startItems.items()) {
                    std::string itemName = item;
                    // TODO: Make GivePlayerItem have a number of items in future
                    for (int _ : amount) {
                        GivePlayerItem(itemName, true);
                    }
                }
                recievedStartingInventory_ = true;
            }
            checkingStartInventory_ = false;
        }
    });

    ap->set_data_package_changed_handler(
        [this](const json& data) { Logger::Log("[AP] Data package loaded for game: ", GAME_NAME); });

    ap->set_slot_connected_handler([this](const json& slotData) {
        Logger::Log("SLOT CONNECTED");
        Logger::Log(slotData.dump());

        slotData_ = slotData;

        if (slotData.contains("drop_experience_multiplier") && !slotData.at("drop_experience_multiplier").is_null()) {
            auto value = slotData.at("drop_experience_multiplier").get<float>();
            GameManager::Instance().GivePlayerStatusMultiplier(SDK::EPBEquipSpecialAttribute::GainExpRate, value);
        }

        if (slotData.contains("drop_item_multiplier") && !slotData.at("drop_item_multiplier").is_null()) {
            auto value = slotData.at("drop_item_multiplier").get<float>();
            GameManager::Instance().GivePlayerStatusMultiplier(SDK::EPBEquipSpecialAttribute::DropItemRate, value);
        }

        if (slotData.contains("drop_money_multiplier") && !slotData.at("drop_money_multiplier").is_null()) {
            auto value = slotData.at("drop_money_multiplier").get<float>();
            GameManager::Instance().GivePlayerStatusMultiplier(SDK::EPBEquipSpecialAttribute::DropMoneyRate, value);
        }

        if (slotData.contains("drop_shard_multiplier") && !slotData["drop_shard_multiplier"].is_null()) {
            auto value = slotData.at("drop_shard_multiplier").get<float>();
            GameManager::Instance().GivePlayerStatusMultiplier(SDK::EPBEquipSpecialAttribute::DropShardRate, value);
        }

        if (slotData.contains("shard_drop_initial_grade") && !slotData["shard_drop_initial_grade"].is_null()) {
            shardDropInitialGrade_ = slotData.at("shard_drop_initial_grade").get<int64_t>();
        }

        if (slotData.contains("start_inventory") && !slotData.at("start_inventory").is_null()) {
            wantsStartingInventory_ = true;
            if (ap->Get({SlotDataStore_.startingInventoryLockedSlot})) {
                checkingStartInventory_ = true;
                Logger::Log("Getting player starting inventory");
            }
        }

        // Store missing locations
        if (slotData.contains("missing_locations")) {
            for (const auto& loc : slotData["missing_locations"]) {
                missingLocations_.insert(loc.get<int64_t>());
            }
        }
        // Store checked locations
        if (slotData.contains("checked_locations")) {
            for (const auto& loc : slotData["checked_locations"]) {
                checkedLocations_.insert(loc.get<int64_t>());
            }
        }
        Logger::Log("[AP] Loaded ", missingLocations_.size(), " missing and ", checkedLocations_.size(),
                    " checked locations");
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
        UpdateState(ArchipelagoConnectionState::InvalidSlotError);
    });

    ap->set_socket_error_handler([this, slotName](const std::string& error) {
        connect_error_count++;
        Logger::Log(LogLevel::Debug, "[AP]", "Player: ", slotName, "disconnected. Error: ", error);
        UpdateState(ArchipelagoConnectionState::SocketDisconnectedError);
    });

    ap->set_bounced_handler([this, slotName](const json& cmd) {
        if (wantsDeathlink_) {
            Logger::Log("Recieved DeathLink");
            auto tagsIt = cmd.find("tags");
            auto dataIt = cmd.find("data");
            if (tagsIt != cmd.end() && tagsIt->is_array() &&
                std::find(tagsIt->begin(), tagsIt->end(), "DeathLink") != tagsIt->end()) {
                if (dataIt != cmd.end() && dataIt->is_object()) {
                    json data = *dataIt;
                    if (data["source"].get<std::string>() != slotName) {
                        std::string source =
                            data["source"].is_string() ? data["source"].get<std::string>().c_str() : "???";
                        std::string cause =
                            data["cause"].is_string() ? data["cause"].get<std::string>().c_str() : "???";
                        GameManager::Instance().SendInGameNotification(source + ": " + cause, 346);  // 346 for skull id
                        Logger::Log("Died by the hands of " + source + " : " + cause);
                        pendingDeathlink_ = true;
                    }
                } else {
                    Logger::Log("Bad deathlink packet!");
                }
            }
        }
    });

    return true;
}

void Archipelago::AbortPassword() { awaiting_password = false; }

void Archipelago::Disconnect() {
    UpdateState(ArchipelagoConnectionState::Disconnected);
    ap.reset();
}

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
