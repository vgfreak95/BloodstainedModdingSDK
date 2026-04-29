#pragma once
#include "Utils.h"

#include <UnrealContainers.hpp>

#include "SDK.hpp"
#include "Windows.h"

UC::FString FStringFromString(std::string string) {
    static std::wstring wide;
    wide = std::wstring(string.begin(), string.end());
    UC::FString fstring(wide.c_str());
    return fstring;
}

std::wstring Utf8ToWide(const std::string& str) {
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring wide(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wide[0], size);
    return wide;
}

SDK::FName FNameFromString(const std::string& str) {
    std::wstring wideName = Utf8ToWide(str);
    return SDK::UKismetStringLibrary::Conv_StringToName(wideName.c_str());
}
