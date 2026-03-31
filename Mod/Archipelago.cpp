#include "Archipelago.h"
#include "Logger.h"
#include "apclient.hpp"
#include "apuuid.hpp"
#include "nlohmann/json_fwd.hpp"
#include <memory>
#include <nlohmann/json.hpp>


std::unique_ptr<APClient> ap;
bool ap_sync_queued = false;
bool ap_connect_sent = false; // TODO: move to APClient::State ?
double deathtime = -1;
bool is_https = false; // set to true if the context only supports wss://
bool is_wss = false; // set to true if the connection specifically asked for wss://
bool is_ws = false; // set to true if the connection specifically asked for ws://
unsigned connect_error_count = 0;
bool awaiting_password = false;

#ifdef _DEBUG
const std::string game_name = "APQuest";
const std::string game_seed = "45680924940695334086";
        // if the socket (re)connects we actually don't know the server's state. clear game's cache to not desync
#else
const std::string game_name = "Bloodstained: Ritual of the Night";
const std::string game_seed = "00000000000000000000";
#endif

/*
These steps should be followed in order to establish a gameplay connection with an Archipelago session.

    Client establishes WebSocket connection to Archipelago server.
    Server accepts connection and responds with a RoomInfo packet.
    Client may send a GetDataPackage packet.
    Server sends a DataPackage packet in return. (If the client sent GetDataPackage.)
    Client sends Connect packet in order to authenticate with the server.
    Server validates the client's packet and responds with Connected or ConnectionRefused.
    Server may send ReceivedItems to the client, in the case that the client is missing items that are queued up for it.
    Server sends PrintJSON to all players to notify them of the new client connection.

In the case that the client does not authenticate properly and receives a ConnectionRefused then the server will maintain the connection and allow for follow-up Connect packet.

There are also a number of community-supported libraries available that implement this network protocol to make integrating with Archipelago easier.

*/



#define UUID_FILE "uuid"
#define CERT_STORE "cacert.pem"

using nlohmann::json;


// APClient integration requires: apclientpp, websocketpp, asio, nlohmann/json, valijson
// TODO: Add apclientpp dependencies and uncomment this code

Archipelago::Archipelago()
    : state_(ArchipelagoConnectionState::Disconnected)
    , retryTimer_(0.0)
    , retryInterval_(10.0)
    , maxRetries_(-1)
    , retryCount_(0)
{
}

Archipelago::~Archipelago()
{
}

void Archipelago::ConnectSlot() {
    bool _connected = false;
    if (ap) {
        // Only try to connect to slot if they are connected to server
        if (state_ == ArchipelagoConnectionState::Connected)
        {
            retryTimer_ += 1.0 / 60.0; // Assuming 60 FPS

            if (retryTimer_ >= retryInterval_)
            {
                if (maxRetries_ == -1 || retryCount_ < maxRetries_)
                {
                    retryTimer_ = 0.0;
                    retryCount_++;
                    Logger::Log("[AP] Retrying connection (attempt", retryCount_, ")...");
                    if (_connected) {
                        ap_sync_queued = true;
                        Logger::Log("[AP] Connection successful.");
                        return;
                    }
                }
                else
                {
                    Logger::Log("[AP] Max retries reached. Giving up.");
                    currentUri_.clear();
                }
            }
        }
    } 
    else 
    {
		Logger::Log(LogLevel::Debug, "[AP]", "Connection lost.");
    }
}

void Archipelago::Connect(const std::string& slotName, const std::string& password, const std::string uri="")
{
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
    std::string uri_without_scheme =
            uri.empty() ? APClient::DEFAULT_URI :
            is_ws ? uri.substr(5) :
            is_wss ? uri.substr(6) :
            uri;

    std::string uuid = ap_get_uuid(UUID_FILE, uri_without_scheme);

    ap.reset(new APClient(uuid, game_name, uri.empty() ? APClient::DEFAULT_URI : uri, CERT_STORE));


    ap_sync_queued = false;
    connect_error_count = 0;

    // Handlers for connection

    ap->set_socket_connected_handler([this](){
        // When websocket successfully connects to server
        AbortPassword();
        UpdateState(ArchipelagoConnectionState::Connected);
		Logger::Log(LogLevel::Debug, "[AP]", "Player: ", slotName_, "connected to server successfully");
    });


    ap->set_room_info_handler([this](){
        // Data recieved from Server about Room information 
		Logger::Log(LogLevel::Debug, "[AP]", "Connecting to slot");
		Logger::Log(LogLevel::Debug, "[AP]", "room information retrieved successfully");
    });

    ap->set_slot_connected_handler([](const json&) {
        // Do things with the data recieved
		Logger::Log(LogLevel::Debug, "[AP]", "Setting up slot information");
    });

    // Handlers for disconnection
    ap->set_socket_disconnected_handler([this](){
        if (state_ == ArchipelagoConnectionState::Disconnected) {
			Logger::Log(LogLevel::Debug, "[AP]", "socket disconnected");
			AbortPassword();
        }
    });
    ap->set_slot_disconnected_handler([this, slotName](){
		Logger::Log(LogLevel::Debug, "[AP]", "Player: ", slotName, "disconnected");
        ap_connect_sent = false;
    });
    ap->set_socket_error_handler([this, slotName](const std::string& error) {
        connect_error_count++;
		Logger::Log(LogLevel::Debug, "[AP]", "Player: ", slotName, "disconnected. Error: ", error);
    });


}

void Archipelago::AbortPassword() {
  awaiting_password = false;
}

void Archipelago::Disconnect()
{
    UpdateState(ArchipelagoConnectionState::Disconnected);
}

void Archipelago::Poll()
{
  if (ap) ap->poll();

  if (state_ == ArchipelagoConnectionState::Connected && !ap_sync_queued) {
      ConnectSlot();
  }
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
    
    // Logger::Log("[Archipelago] State changed: ", (int)oldState, " -> ", (int)newState);
    
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
