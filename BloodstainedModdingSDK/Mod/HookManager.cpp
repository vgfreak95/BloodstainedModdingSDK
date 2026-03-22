#include "HookManager.h"
#include "Logger.h"

#define PLAYER_NAME "Chr_P0000_C_0"

// Static member
std::unordered_map<void*, detour_ctx_t> HookManager::ctxs;

HookManager& HookManager::Instance()
{
	static HookManager instance;
	return instance;
}

bool HookManager::Init()
{
	// Can't find Game Information without the Engine and the World
	SDK::UEngine* engine = SDK::UEngine::GetEngine();
	if (!engine)
	{
		Logger::Log(LogLevel::Warning, this, "waiting for UEngine");
		return false;
	}

	auto world = SDK::UWorld::GetWorld();
	if (!world || !world->OwningGameInstance || !world->OwningGameInstance->LocalPlayers.Num() || !world->OwningGameInstance->LocalPlayers[0]->PlayerController)
	{
		Logger::Log(LogLevel::Warning, this, "waiting for UWorld");
		return false;
	}

	SDK::UGameInstance* pbGameInstance = SDK::UGameplayStatics::GetGameInstance(world);

	auto playerController = (SDK::APBPlayerController*)(pbGameInstance->LocalPlayers[0]->PlayerController);
	auto pawn = (SDK::APBCPlayerBase*)(playerController->Pawn);

	// Wait for the player to load in
	std::string playerName = pawn->GetName(); // Initially TitlePawn
	if (playerName != PLAYER_NAME) {
		playerName = pawn->GetName();
		Logger::Log(LogLevel::Warning, this, "waiting for player to start a file / game...");
		return false;
	}

	// PB_Chr_PlayerRoot_C
	auto character = (SDK::APB_Chr_PlayerRoot_C*)(pawn);
	auto characterInventory = character->CharacterInventory;
	auto charParams = character->CharacterParamaters;

	// Modifying Character Parameters (STR, LUC, CON etc)
	charParams.STR = 10000.0f;
	charParams.LUC = 10000.0f;

	// Setting Movement Speed and Back Step Speed, Check EPBEquipSpecialAttribute for more information
	characterInventory->SetSpecialAttribute(SDK::EPBEquipSpecialAttribute::NormalMoveUpRate, 1000.0f);
	characterInventory->SetSpecialAttribute(SDK::EPBEquipSpecialAttribute::BackStepRate, 1000.0f);

	auto playerState = (SDK::APBPlayerState*)character->PlayerState;
	
	// Fun God mode
	Logger::Log("Turning on God Mode...");
	playerState->bGodMode = true;



	// SetLaunchGameIntent
	// if (!HookNativeFunction(SDK::UGameInstance::StaticClass()))
	//if (!HookNativeFunction(SDK::UGameInstanceZion::StaticClass(), "GameInstanceZion", "SetLaunchGameIntent", &HookManager::SetLaunchGameIntent_Hook))
		//Logger::Log(LogLevel::Error, this, "Failed to hook SetLaunchIntent");

	// tick
//	if (!HookNativeFunction(SDK::UActorComponent::StaticClass(), "ActorComponent", "ReceiveTick", &HookManager::DEBUG_Hook))
//		Logger::Log(LogLevel::Error, this, "Failed to hook tick");

	/*s
	for (int i = 0; i < SDK::UObject::GObjects->Num(); ++i)
	{
		SDK::UObject* Object = SDK::UObject::GObjects->GetByIndex(i);

		if (!Object)
			continue;

		if (Object->HasTypeFlag(SDK::EClassCastFlags::Function) && Object->GetName() == "ReceiveTick")
		{
			Logger::Log(LogLevel::Error, this, "FUNC: ", Object->GetFullName());
		}
	}
	for (const SDK::UStruct* Clss = SDK::APlayerController::StaticClass(); Clss; Clss = Clss->Super)
	{
		for (SDK::UField* Field = Clss->Children; Field; Field = Field->Next)
		{
			if (Field->HasTypeFlag(SDK::EClassCastFlags::Function))
				Logger::Log(LogLevel::Error, this, Clss->GetName(), "." , Field->GetName());
		}
	}*/

	// Enable Hooks
	Logger::Log(this, "Init ok");
	return true;
}

//bool HookManager::HookNativeFunction(const SDK::UClass *defaultClass, const std::string className, const std::string funcName, FNativeFuncPtr detour)
//{
//	if (!defaultClass)
//	{
//		Logger::Log(LogLevel::Error, this, "no default class");
//		return false;
//	}
//	auto Func = defaultClass->GetFunction(className, funcName);
//	if (!Func || !Func->ExecFunction)
//	{
//		Logger::Log(LogLevel::Error, this, "no function", className, ".", funcName);
//		return false;
//	}
//	this->ctxs[detour] = detour_ctx_t();
//	detour_init(&this->ctxs[detour], Func->ExecFunction, detour);
//	return detour_enable(&this->ctxs[detour]);
//}
//
//bool HookManager::HookProcessEvent(FProcessEventFuncPtr detour)
//{
//	void* origPtr = reinterpret_cast<void*>(SDK::InSDKUtils::GetImageBase() + SDK::Offsets::ProcessEvent);
//	if (!origPtr)
//	{
//		Logger::Log(LogLevel::Error, this, "Failed to get original ProcessEvent function");
//		return false;
//	}
//	this->ctxs[detour] = detour_ctx_t();
//	detour_init(&this->ctxs[detour], origPtr, detour);
//	return detour_enable(&this->ctxs[detour]);
//}
//
//void HookManager::ProcessEvent(const SDK::UObject* obj, SDK::UFunction* func, void* params)
//{
//	static Subscriber PlayerCameraManager_ReceiveTick("CameraAnimationCameraModifier", "BlueprintModifyCamera");
//	if (PlayerCameraManager_ReceiveTick.Matches(obj, func))
//		GameManager::Instance().OnReceiveTick();
//}
//
//void HookManager::SetLaunchGameIntent(SDK::UObject* Context, void* TheStack, void* Result)
//{
//	GameManager::Instance().OnGameStarted();
//}

inline bool HookManager::Subscriber::Matches(const SDK::UObject* obj, const SDK::UFunction* func)
{
	if (!obj || !func) return false;
	if (_objFName.ComparisonIndex != 0 && _funcFName.ComparisonIndex != 0)
	{
		return obj->Class->Name == _objFName && func->Name == _funcFName;
	}
	auto match = objName == obj->Class->Name.ToString() && funcName == func->Name.GetRawString();
	if (match)
	{
		_objFName = obj->Class->Name;
		_funcFName = func->Name;
	}
	return match;
}
