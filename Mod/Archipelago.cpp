#include "Archipelago.h"
#include "Logger.h"

// APClient integration requires: apclientpp, websocketpp, asio, nlohmann/json, valijson
// TODO: Add apclientpp dependencies and uncomment this code

Archipelago::Archipelago()
    : state_(ArchipelagoConnectionState::Disconnected)
{
}

Archipelago::~Archipelago()
{
}

void Archipelago::Connect(const std::string& host, int port, const std::string& slotName, const std::string& password)
{
    Logger::Log("[Archipelago] Not implemented - apclientpp not available");
}

void Archipelago::Disconnect()
{
}

void Archipelago::Poll()
{
}

void Archipelago::SendLocationCheck(int64_t locationId)
{
}

void Archipelago::SendItem(int64_t itemId, int64_t locationId, int64_t playerId)
{
}

void Archipelago::UpdateState(ArchipelagoConnectionState newState)
{
    if (state_ == newState)
        return;
    
    ArchipelagoConnectionState oldState = state_;
    state_ = newState;
    
    Logger::Log("[Archipelago] State changed: ", (int)oldState, " -> ", (int)newState);
    
    if (newState == ArchipelagoConnectionState::SlotConnected && onConnected_)
    {
        onConnected_();
    }
    else if (newState == ArchipelagoConnectionState::Disconnected && oldState != ArchipelagoConnectionState::Disconnected)
    {
        if (onDisconnected_)
            onDisconnected_();
    }
    else if (newState == ArchipelagoConnectionState::Error && onError_)
    {
        onError_(lastError_);
    }
}
