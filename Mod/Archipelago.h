#pragma once
#include <fstream>
#include <set>
#include <string>

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

    Archipelago();
    ~Archipelago();

    // AP Actions
    bool Connect(const std::string& slotName, const std::string& password, std::string uri);
    void Disconnect();
    void Poll();
    void Sync();

    void SendLocationChecks(const std::string& locationId);

    bool IsConnected() const { return state_ == ArchipelagoConnectionState::SlotConnected; }

    ArchipelagoConnectionState GetConnectionState() const { return state_; }
    std::string GetStateAsString();
    std::string GetSlotName() const { return slotName_; }
    std::string GetLastError() const { return lastError_; }

    void SetLastError(const std::string error) const { lastError_ = error; };

    void ExecuteConsoleCommand(const char* command);

    void BaelDefeated();
    void ResetLocalIndex() { lastReceivedItemIndex_ = -1; };
    void SetFileLastIndex();

   private:
    void AbortPassword();
    void ConnectSlot();
    void UpdateState(ArchipelagoConnectionState newState);

    // General Management Actions
    void GivePlayerItem(std::string& itemName, bool shouldDisplay = true);
    void SendInGameNotification(std::string notification);

    int GetFileLastIndex();

    ArchipelagoConnectionState state_;
    std::string slotName_;
    int playerSlot_ = 0;
    std::string password_;
    int itemsHandling_ = 0b0001;  // Send items from other players
    mutable std::string lastError_;
    std::string currentUri_;
    double retryTimer_;
    double retryInterval_;
    int maxRetries_;
    int retryCount_;
    std::fstream indexFile;

    std::set<int64_t> missingLocations_;
    std::set<int64_t> checkedLocations_;
    int64_t lastReceivedItemIndex_ = -1;
};
