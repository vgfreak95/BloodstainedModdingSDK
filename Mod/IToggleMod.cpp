#pragma once
#include "IToggleMod.h"

#include "Logger.h"

void IToggleMod::Init(const char* label) {
    if (ImGui::Checkbox(label, &m_Toggled)) {
        Toggle();
    }
}

void IToggleMod::Toggle() {
    if (m_Toggled) {
        OnActivated();
    } else {
        OnDeactivated();
    }
}
