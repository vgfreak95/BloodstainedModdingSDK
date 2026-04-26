#pragma once
#include <chrono>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "Logger.h"

class NotifyObject {
   public:
    using Callback = std::function<void(void* obj)>;
    using CallbackWithFunc = std::function<void(void* obj, const std::string& funcName)>;
    using CallBackWithFuncAndParams = std::function<void(void* obj, const std::string& funcName, void* params)>;
    using IsValidFunc = std::function<bool(void* obj)>;

    NotifyObject() = default;
    ~NotifyObject() = default;

    int GetCallbackCount() const { return (int)callbacks.size() + (int)classCallbacks.size(); }
    int GetProcessedCount() const { return processedCount; }

    void Register(const std::string& className, const std::string& funcName, Callback callback) {
        std::string key = className + "|" + funcName;
        callbacks[key] = callback;
        Logger::Log("[NotifyObject] Registered class=", className, " func=", funcName);
    }

    void Register(const std::string& className, Callback callback) {
        classCallbacks[className] = callback;
        Logger::Log("[NotifyObject] Registered class=", className, " (all funcs)");
    }

    void RegisterWithFuncName(const std::string& className, CallbackWithFunc callback) {
        classCallbacksWithFunc[className] = callback;
        Logger::Log("[NotifyObject] Registered class=", className, " (with func names)");
    }

    void RegisterPartial(const std::string& partialName, Callback callback) {
        std::string lower = partialName;
        for (auto& c : lower) c = std::tolower(c);
        partialCallbacks[lower] = callback;
        Logger::Log("[NotifyObject] Registered partial=", partialName, " (case-insensitive)");
    }

    void RegisterPartial(const std::string& partialName, CallbackWithFunc callback) {
        std::string lower = partialName;
        for (auto& c : lower) c = std::tolower(c);
        partialCallbacksWithFunc[lower] = callback;
        Logger::Log("[NotifyObject] Registered partial=", partialName, " with func (case-insensitive)");
    }

    void RegisterWithFuncNameAndParams(const std::string& className, const std::string& funcName,
                                       CallBackWithFuncAndParams callback) {
        classCallbacksWithFuncAndParams[className] = callback;
        Logger::Log("[NotifyObject] Registered class=", className, " (with func names and params)");
    }

    void SetValidityChecker(IsValidFunc checker) { isValidFunc = checker; }

    void OnProcessEvent(void* obj, const std::string& className, const std::string& funcName, void* params) {
        if (!obj) return;

        CleanupIfNeeded();

        std::string key = className + "|" + funcName;
        auto it = callbacks.find(key);
        if (it != callbacks.end()) {
            // Logger::Log("[NotifyObject] MATCH class=", className, " func=", funcName);
            if (trackedObjects.find(obj) == trackedObjects.end()) {
                trackedObjects.insert(obj);
                processedCount++;
                // Logger::Log("[NotifyObject] Callback fired! (processed: ", processedCount, ")");
                it->second(obj);
                trackedObjects.erase(obj);
            }
            return;
        }

        auto classIt = classCallbacks.find(className);
        if (classIt != classCallbacks.end()) {
            // Logger::Log("[NotifyObject] MATCH class=", className, " (class callback)");
            if (trackedObjects.find(obj) == trackedObjects.end()) {
                trackedObjects.insert(obj);
                processedCount++;
                // Logger::Log("[NotifyObject] Callback fired! (processed: ", processedCount, ")");
                classIt->second(obj);
                trackedObjects.erase(obj);
            }
        }

        auto classWithFuncIt = classCallbacksWithFunc.find(className);
        if (classWithFuncIt != classCallbacksWithFunc.end()) {
            // Logger::Log("[NotifyObject] MATCH class=", className, " func=", funcName, " (with func name)");
            classWithFuncIt->second(obj, funcName);
        }

        // Check partial matches (case-insensitive)
        std::string lowerClassName = className;
        for (auto& c : lowerClassName) c = std::tolower(c);

        for (const auto& [partial, callback] : partialCallbacks) {
            if (lowerClassName.find(partial) != std::string::npos) {
                callback(obj);
                break;
            }
        }

        for (const auto& [partial, callback] : partialCallbacksWithFunc) {
            if (lowerClassName.find(partial) != std::string::npos) {
                callback(obj, funcName);
                break;
            }
        }
        auto classWithFuncAndParamsIt = classCallbacksWithFuncAndParams.find(className);
        if (classWithFuncAndParamsIt != classCallbacksWithFuncAndParams.end()) {
            classWithFuncAndParamsIt->second(obj, funcName, params);
        }
    }

    void Clear() {
        callbacks.clear();
        classCallbacks.clear();
        classCallbacksWithFunc.clear();
        trackedObjects.clear();
    }

   private:
    void CleanupIfNeeded() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration<double, std::milli>(now - lastCleanup).count();

        if (elapsed < 500.0) return;

        for (auto it = trackedObjects.begin(); it != trackedObjects.end();) {
            if (!isValidFunc || !isValidFunc(*it)) {
                Logger::Log("[NotifyObject] Removing invalid obj=", (void*)*it);
                it = trackedObjects.erase(it);
            } else {
                ++it;
            }
        }
        lastCleanup = now;
    }

    std::unordered_map<std::string, Callback> callbacks;
    std::unordered_map<std::string, Callback> classCallbacks;
    std::unordered_map<std::string, CallbackWithFunc> classCallbacksWithFunc;
    std::unordered_map<std::string, CallBackWithFuncAndParams> classCallbacksWithFuncAndParams;
    std::unordered_map<std::string, Callback> partialCallbacks;
    std::unordered_map<std::string, CallbackWithFunc> partialCallbacksWithFunc;
    std::unordered_set<void*> trackedObjects;
    IsValidFunc isValidFunc;
    std::chrono::steady_clock::time_point lastCleanup = std::chrono::steady_clock::now();
    int processedCount = 0;
};
