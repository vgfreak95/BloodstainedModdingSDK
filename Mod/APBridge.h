#pragma once

#include <deque>
#include <mutex>
#include <string>

// Lightweight, UI-friendly bridge to request Archipelago actions without touching Archipelago internals
// Usage: UI code calls EnqueueConnect/EnqueueDisconnect/EnqueueSync, then periodically call ProcessPending()
// from the main/game loop (or wherever Archipelago is processed).

class APBridge {
   public:
    // Access the singleton instance
    static APBridge& Instance();

    // Enqueue actions from UI
    void EnqueueConnect(const std::string& slotName, const std::string& password, const std::string& uri = "");
    void EnqueueDisconnect();
    void EnqueueSync();

    // Drain and dispatch pending commands to Archipelago
    void ProcessPending();

   private:
    APBridge();
    // Non-copyable
    APBridge(const APBridge&) = delete;
    APBridge& operator=(const APBridge&) = delete;

    struct Command {
        enum Type { Connect = 0, Disconnect = 1, Sync = 2, Poll = 3 } type;
        std::string slotName;
        std::string password;
        std::string uri;
    };

    std::deque<Command> queue_;
    std::mutex mutex_;
};
