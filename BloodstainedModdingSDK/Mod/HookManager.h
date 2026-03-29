#pragma once
#include "SDK.hpp"
#include "Logger.h"
#include "NotifyObject.h"
#include "PBBronzeTreasureBox_BP_classes.hpp"
#include "PBGoldenTreasureBox_BP_classes.hpp"
#include "ItemGetPopup_classes.hpp"
#include <unordered_map>
#include <set>
#include <functional>

extern "C" {
#include "MinHook.h"
}

class HookManager
{
public:
    static HookManager& Instance()
    {
        static HookManager instance;
        return instance;
    }

    bool Init();
    bool PostInit();

    void NotifyOnClassFunction(const std::string& className, const std::string& funcName, NotifyObject::Callback callback)
    {
        notifyObject.Register(className, funcName, callback);
    }

    void NotifyOnClass(const std::string& className, NotifyObject::Callback callback)
    {
        notifyObject.Register(className, callback);
    }

    void NotifyOnMatchingClass(const std::string& partialName, NotifyObject::Callback callback)
    {
        notifyObject.RegisterPartial(partialName, callback);
    }

    void NotifyOnMatchingClass(const std::string& partialName, NotifyObject::CallbackWithFunc callback)
    {
        notifyObject.RegisterPartial(partialName, callback);
    }

    static bool playerDetected;

private:
    HookManager() = default;
    ~HookManager() = default;
    HookManager(const HookManager&) = delete;
    HookManager& operator=(const HookManager&) = delete;

    void ProcessEvent(void* obj, SDK::UFunction* func)
    {
        if (!obj || !func)
            return;

        auto* uobj = static_cast<SDK::UObject*>(obj);
        std::string className = uobj->Class->Name.ToString();
        std::string funcName = func->Name.GetRawString();

        notifyObject.OnProcessEvent(obj, className, funcName);
    }

    static void HOOKED_ProcessEvent(SDK::UObject* obj, SDK::UFunction* func, void* params)
    {
        if (obj && func)
        {
            HookManager::Instance().ProcessEvent(obj, func);
        }
        
        originalProcessEvent(obj, func, params);
    }

    NotifyObject notifyObject;
    static std::set<std::string> pendingWidgets;
    static std::set<std::string> processedWidgets;
    static void(*originalProcessEvent)(SDK::UObject*, SDK::UFunction*, void*);
};
