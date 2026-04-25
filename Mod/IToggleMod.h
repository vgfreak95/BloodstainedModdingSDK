#pragma once
#include "imgui.h"

class IToggleMod {
   public:
    IToggleMod() = default;
    virtual ~IToggleMod() = default;

    void Init(const char* label);
    void Toggle();

    virtual void OnActivated() {}
    virtual void OnDeactivated() {}

   private:
    bool m_Toggled = false;
};
