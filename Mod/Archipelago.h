#pragma once
#include <apclient.hpp>
#include <fstream>
#include <set>
#include <string>

using json = nlohmann::json;

enum class ArchipelagoConnectionState {
    Disconnected = 0,
    Connecting = 1,
    Connected = 2,
    SlotConnected = 3,
    SocketDisconnectedError = 4,
    ConnectionRefusedError = 5,
    InvalidSlotError = 6
};

class Archipelago {
   public:
    static Archipelago& Instance();
    static Archipelago* ConnectedInstance();

    Archipelago();
    ~Archipelago();

    bool IsConnected() const { return state_ == ArchipelagoConnectionState::SlotConnected; }

    ArchipelagoConnectionState GetConnectionState() const { return state_; }
    std::string GetStateAsString();
    std::string GetSlotName() const { return slotName_; }
    std::string GetLastError() const { return lastError_; }

    void SetLastError(const std::string error) const { lastError_ = error; };
    bool IsPendingDeathlink() const { return pendingDeathlink_; };
    void ResetDeathLink() const { pendingDeathlink_ = false; };

    // AP Actions
    bool Connect(const std::string& slotName, const std::string& password, std::string uri, const bool& wantsDeathlink);
    void Disconnect();
    void Poll();
    void Sync();

    void SendLocationChecks(const std::string& locationId);

    void ExecuteConsoleCommand(const char* command);

    void BaelDefeated();
    void ResetLocalIndex() { lastReceivedItemIndex_ = -1; };
    void UpdateServerLastIndex();
    void InvokeDeathLink();
    void LockStartingInventory();
    void ResetRecievedStartingInventory() const { recievedStartingInventory_ = false; };

   private:
    void AbortPassword();
    void ConnectSlot();
    void UpdateState(ArchipelagoConnectionState newState);

    // General Management Actions
    void GivePlayerItem(std::string& itemName, bool shouldDisplay = true);

    ArchipelagoConnectionState state_;
    std::string currentUri_;
    std::string slotName_;
    int playerSlot_ = 0;
    std::string password_;
    int itemsHandling_ = 0b0001;  // Send items from other players
    std::set<int64_t> missingLocations_;
    std::set<int64_t> checkedLocations_;

    mutable std::string lastError_;
    mutable int64_t lastReceivedItemIndex_ = -1;
    mutable bool queueLastIndexReset_ = false;
    mutable bool wantsStartingInventory_ = false;
    mutable bool recievedStartingInventory_ = false;
    mutable bool startingInventoryLocked = false;
    mutable bool pendingDeathlink_ = false;
    mutable bool wantsDeathlink_ = false;
    mutable bool checkingIndex_ = false;
    mutable bool checkingStartInventory_ = false;

    std::list<APClient::NetworkItem> items_;
    json slotData_;

    int64_t shardDropInitialGrade_;

    // Holds Data stored on server
    // slot:1:lock_starting_inventory
    // slot:1:index
    struct {
        std::string startingInventoryLockedSlot;
        std::string lastIndexSlot;
    } SlotDataStore_;

    std::list<std::string> deathReasons_ = {"Dominiques elbow was too strong for Miriam.",
                                            "Johannes failed at at making his latest potion",
                                            "The Galleon Minerva crashed into an Iceberg",
                                            "Miriam thought of the wrong place when using a waystone",
                                            "Miriam starved because Susie ate all the food",
                                            "Miriam forgot to return a book...",
                                            "Miriams invert didn't auto cancel...",
                                            "Miriam tripped and fell while backstepping",
                                            "Miriam saw a large shard of stained glass flying at her and didn't "
                                            "realize it was from a building, not a monster.",
                                            "Zangetsu missed the train..."};
};
