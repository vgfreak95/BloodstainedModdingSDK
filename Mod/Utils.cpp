#pragma once
#include "Utils.h"

#include <UnrealContainers.hpp>

#include "Windows.h"

UC::FString FStringFromString(std::string string) {
    static std::wstring wide;
    wide = std::wstring(string.begin(), string.end());
    UC::FString fstring(wide.c_str());
    return fstring;
}
