#pragma once
#include <string.h>

#include "SDK.hpp"
#include "UnrealContainers.hpp"

UC::FString FStringFromString(std::string string);
std::wstring Utf8ToWide(const std::string& str);
SDK::FName FNameFromString(const std::string& str);
