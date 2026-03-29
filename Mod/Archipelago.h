#pragma once
#include <string>
#include <functional>
#include <list>

enum class ArchipelagoConnectionState
{
    Disconnected,
    Connecting,
    Connected,
    SlotConnected,
    Error
};

struct NetworkItem
{
    int64_t item;
    int64_t location;
    int64_t player;
    bool flags;
};

class Archipelago
{
public:
    using ConnectedCallback = std::function<void()>;
    using DisconnectedCallback = std::function<void()>;
    using ItemsReceivedCallback = std::function<void(const std::list<NetworkItem>&)>;
    using MessageCallback = std::function<void(const std::string&)>;
    using ErrorCallback = std::function<void(const std::string&)>;

    Archipelago();
    ~Archipelago();

    void Connect(const std::string& host, int port, const std::string& slotName, const std::string& password);
    void Disconnect();
    void Poll();
    
    void SendLocationCheck(int64_t locationId);
    void SendItem(int64_t itemId, int64_t locationId, int64_t playerId);

    ArchipelagoConnectionState GetState() const { return state_; }
    std::string GetSlotName() const { return slotName_; }
    std::string GetLastError() const { return lastError_; }

    void SetConnectedCallback(ConnectedCallback callback) { onConnected_ = callback; }
    void SetDisconnectedCallback(DisconnectedCallback callback) { onDisconnected_ = callback; }
    void SetItemsReceivedCallback(ItemsReceivedCallback callback) { onItemsReceived_ = callback; }
    void SetMessageCallback(MessageCallback callback) { onMessage_ = callback; }
    void SetErrorCallback(ErrorCallback callback) { onError_ = callback; }

private:
    void UpdateState(ArchipelagoConnectionState newState);

    ArchipelagoConnectionState state_;
    std::string slotName_;
    std::string lastError_;

    ConnectedCallback onConnected_;
    DisconnectedCallback onDisconnected_;
    ItemsReceivedCallback onItemsReceived_;
    MessageCallback onMessage_;
    ErrorCallback onError_;
};
