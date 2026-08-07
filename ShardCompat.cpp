#pragma once
#include <Engine_classes.hpp>
#include <ProjectBlood_structs.hpp>
#include <UnrealContainers.hpp>
#include <optional>

#include "Basic.hpp"
#include "GameManager.h"
#include "Gui.h"
#include "Logger.h"
#include "Mod/Archipelago.h"
#include "ProjectBlood_classes.hpp"
#include "Utils.h"


struct ShardAppearance {
    SDK::EShardType type;
    SDK::EShardColor color;
};

static std::unordered_map<std::string, ShardAppearance> originalRandomizedShardAppearances;
static std::unordered_map<std::string, SDK::EDropSpecialFlag> originalRandomizedDropFlags;

namespace {

const std::unordered_map<std::string, const char*> knownVanillaShardIds = {
    // Generated from the unmodified base-game PB_DT_DropRateMaster in pakchunk0.
    // Runtime table access is insufficient because Randomizer.pak has already replaced
    // these rows with AP_* placeholder shards by the time this plugin loads.
    {"AP_AAAA_Shard", "Dummy"},
    {"AP_N1001_Shard", "SwingTentacle"},
    {"AP_N1002_Shard", "Bloodsteel"},
    {"AP_N1003_Shard", "Demoniccapture"},
    {"AP_N1006_Shard", "Reflectionray"},
    {"AP_N1008_Shard", "Dimensionshift"},
    {"AP_N2001_Shard", "Hammerknuckle"},
    {"AP_N2004_Shard", "Doublejump"},
    {"AP_N2006_Shard", "Invert"},
    {"AP_N2007_Shard", "Shadowtracer"},
    {"AP_N2012_Shard", "AccelWorld"},
    {"AP_N2013_Shard", "NeverSatisfied"},
    {"AP_N1004_Shard", "GoldBarrett"},
    {"AP_N1005_Shard", "InfernoBrace"},
    {"AP_N3006_Shard", "Ceruleansplash"},
    {"AP_N3006_OpeningDemo_Shard", "Ceruleansplash"},
    {"AP_N3005_Shard", "FireCannon"},
    {"AP_N3005_FireCannon_Shard", "FireCannon"},
    {"AP_N3007_Shard", "Raginggirl"},
    {"AP_N3013_Shard", "SummonAme"},
    {"AP_N3015_Shard", "Headfail"},
    {"AP_N3051_Shard", "SummonRat"},
    {"AP_N3004_Shard", "BoonsLore"},
    {"AP_N3029_Shard", "SummonButt"},
    {"AP_N3073_Shard", "SummonGilemund"},
    {"AP_N3105_Shard", "HelleTicalGrinder"},
    {"AP_N3080_Shard", "SwordMastery"},
    {"AP_N3066_Shard", "EntangleBind"},
    {"AP_N3042_Shard", "Releasetoade"},
    {"AP_N3065_Shard", "SummonYorkton"},
    {"AP_N3011_Shard", "StraightArrow"},
    {"AP_N3091_Shard", "FamiliaCarabos"},
    {"AP_N3034_Shard", "SummonApe"},
    {"AP_N3075_Shard", "CraftMastery"},
    {"AP_N3100_Shard", "GratefulAssist"},
    {"AP_N3104_Shard", "Acidgouache"},
    {"AP_N3053_Shard", "Petrey"},
    {"AP_N3079_Shard", "KnifeMastery"},
    {"AP_N3055_Shard", "INTEnhance"},
    {"AP_N3001_Shard", "SummonBuell"},
    {"AP_N3116_Shard", "LigaStreyma"},
    {"AP_N3082_Shard", "SummonBugs"},
    {"AP_N3090_Shard", "SummonDurahanMaHead"},
    {"AP_N3084_Shard", "SummonBuell"},
    {"AP_N2002_Shard", "Beastguard"},
    {"AP_N3064_Shard", "CircleRipper"},
    {"AP_N3010_Shard", "Shovelshooter"},
    {"AP_N3033_Shard", "ResistThunder"},
    {"AP_N3017_Shard", "GunMastery"},
    {"AP_N3063_Shard", "FamiliaDantalion"},
    {"AP_N3103_Shard", "Sacredshade"},
    {"AP_N3099_Shard", "ResistPetri"},
    {"AP_N3008_Shard", "TepesOsius"},
    {"AP_N3111_Shard", "Aimingshield"},
    {"AP_N3058_Shard", "ChangeBunny"},
    {"AP_N3019_Shard", "ShootingDagger"},
    {"AP_N2009_Shard", "DragonicRage"},
    {"AP_N3025_Shard", "Resistcurse"},
    {"AP_N3043_Shard", "ResistPoison"},
    {"AP_N3092_Shard", "Healing"},
    {"AP_N3024_Shard", "BaudRideBlast"},
    {"AP_N3070_Shard", "Moneyispower"},
    {"AP_N3114_Shard", "ResistBrow"},
    {"AP_N3072_Shard", "SummonChair"},
    {"AP_N3012_Shard", "ChaseArrow"},
    {"AP_N3052_Shard", "FamiliaBradBringer"},
    {"AP_N3085_Shard", "SpearMastery"},
    {"AP_N3109_Shard", "Upbeatheat"},
    {"AP_N3037_Shard", "FamiliaBuell"},
    {"AP_N3077_Shard", "KickMastery"},
    {"AP_N3093_Shard", "MNDEnhance"},
    {"AP_N3110_Shard", "ThrowAxe"},
    {"AP_N3121_Shard", "Zombie8Bit"},
    {"AP_N3122_Shard", "Ghost8Bit"},
    {"AP_N2015_Shard", "Nightmare8Bit"},
    {"AP_N3124_Shard", "EightBitFire"},
    {"AP_N3102_Shard", "ResistHorley"},
    {"AP_N3009_Shard", "optimizer"},
    {"AP_N3087_Shard", "Deadhowling"},
    {"AP_N3098_Shard", "Resistedge"},
    {"AP_N3078_Shard", "VaIsha"},
    {"AP_N3022_Shard", "Aquastream"},
    {"AP_N3044_Shard", "SamonSamHiggin"},
    {"AP_N3076_Shard", "Submariner"},
    {"AP_N3089_Shard", "Detectiveeye"},
    {"AP_N3056_Shard", "Wisdomwords"},
    {"AP_N3014_Shard", "CurseDray"},
    {"AP_N3002_Shard", "CONEnhance"},
    {"AP_N3112_Shard", "TissLeif"},
    {"AP_N3115_Shard", "ResistBrow"},
    {"AP_N2010_Shard", "Resistthrust"},
    {"AP_N3101_Shard", "Resistdark"},
    {"AP_N3059_Shard", "WhipMastery"},
    {"AP_N3113_Shard", "Toxicstorm"},
    {"AP_N3083_Shard", "RedDowther"},
    {"AP_N3032_Shard", "ResistFire"},
    {"AP_N3119_Shard", "FireThrower"},
    {"AP_N3123_Shard", "HellHound"},
    {"AP_N3086_Shard", "RapidSpear"},
    {"AP_N3071_Shard", "LUKEnhance"},
    {"AP_N3035_Shard", "LuckyDrop"},
    {"AP_N3054_Shard", "STREnhance"},
    {"AP_N3074_Shard", "PetraBless"},
    {"AP_N3020_Shard", "HiddenDart"},
    {"AP_N3117_Shard", "Tornadoslicer"},
    {"AP_N3067_Shard", "EntangleBind"},
    {"AP_N3030_Shard", "SummonButt"},
    {"AP_N3045_Shard", "Releasetoade"},
    {"AP_N3016_Shard", "FireCannon"},
    {"AP_N3028_Shard", "FamiliaBuell"},
    {"AP_N2003_Shard", "WildScratch"},
    {"AP_N2008_Shard", "Voidlay"},
    {"AP_N3081_Shard", "drain"},
    {"AP_N3021_Shard", "Accelerator"},
    {"AP_N3038_Shard", "GunMastery"},
    {"AP_N3031_Shard", "Resistice"},
    {"AP_N3088_Shard", "SummonTracer"},
    {"AP_N3018_Shard", "GunMastery"},
    {"AP_N3120_Shard", "AxStrike"},
    {"AP_N3057_Shard", "FoldShiu"},
    {"AP_N3106_Shard", "Chiselbalage"},
    {"AP_N3107_Shard", "RuinBeak"},
    {"AP_N3108_Shard", "Jackpot"},
    {"AP_Shortcut_Shard", "Shortcut"},
    {"AP_Deepsinker_Shard", "Deepsinker"},
    {"AP_FamiliaSilverKnight_Shard", "FamiliaSilverKnight"},
    {"AP_FamiliaArcher_Shard", "FamiliaArcher"},
};

std::optional<SDK::FName> ResolveVanillaShardId(const SDK::FName& randomizedShardId) {
    auto knownShard = knownVanillaShardIds.find(randomizedShardId.ToString());
    if (knownShard == knownVanillaShardIds.end()) return std::nullopt;
    return FNameFromString(knownShard->second);
}

ItemLookupResult GetVanillaShardItemSupport(Archipelago& archipelago, const SDK::FName& vanillaShardId) {
    SDK::FPBItemCatalogData itemData{};
    GameManager::Instance().Player()->CharacterInventory->GetItemDataById(vanillaShardId, &itemData);
    std::string displayName = itemData.Name.ToString();

    if (!displayName.empty()) {
        ItemLookupResult result = archipelago.GetItemLookupResult(displayName);
        if (result != ItemLookupResult::UnknownItem) return result;
    }
    return archipelago.GetItemLookupResult(vanillaShardId.ToString());
}

SDK::EShardColor DefaultShardColor(SDK::EShardType shardType) {
    switch (shardType) {
        case SDK::EShardType::Trigger:
            return SDK::EShardColor::Red;
        case SDK::EShardType::Effective:
            return SDK::EShardColor::Blue;
        case SDK::EShardType::Directional:
            return SDK::EShardColor::Purple;
        case SDK::EShardType::Enchant:
            return SDK::EShardColor::Yellow;
        case SDK::EShardType::Familia:
            return SDK::EShardColor::Green;
        case SDK::EShardType::Skill:
            return SDK::EShardColor::White;
        default:
            return SDK::EShardColor::None;
    }
}

SDK::FPBShardMasterData* FindShardMasterRow(SDK::UDataTable* shardMasterTable, const std::string& rowName) {
    if (!shardMasterTable) return nullptr;
    for (const auto& pair : shardMasterTable->RowMap) {
        if (pair.Key().ToString() == rowName) {
            return reinterpret_cast<SDK::FPBShardMasterData*>(pair.Value());
        }
    }
    return nullptr;
}

SDK::FPBDropRateMasterData* FindDropMasterRow(SDK::UDataTable* dropTable, const std::string& rowName) {
    if (!dropTable) return nullptr;
    for (const auto& pair : dropTable->RowMap) {
        if (pair.Key().ToString() == rowName) {
            return reinterpret_cast<SDK::FPBDropRateMasterData*>(pair.Value());
        }
    }
    return nullptr;
}

bool RestoreVanillaShardAppearance(SDK::AShardBase* shardBase, const SDK::FName& vanillaShardId) {
    auto* gameInstance = static_cast<SDK::UPBGameInstance*>(GameManager::Instance().GameInstance());
    if (!gameInstance) return false;

    auto* shardManager = gameInstance->GetShardManager();
    if (!shardManager || !shardManager->ShardMasterTable) return false;

    auto* shardData = FindShardMasterRow(shardManager->ShardMasterTable, vanillaShardId.ToString());
    if (!shardData) return false;

    SDK::EShardColor shardColor = shardData->ShardColorOverride;
    if (shardColor == SDK::EShardColor::None) shardColor = DefaultShardColor(shardData->ShardType);

    shardBase->ShardId = vanillaShardId;
    shardBase->ShardType = shardData->ShardType;
    shardBase->ShardColor = shardColor;
    shardBase->SetShardType(shardData->ShardType);
    return true;
}

}  // namespace

void HookManager::ResetCompatibilityShardMasterData() {
    auto* gameInstance = static_cast<SDK::UPBGameInstance*>(GameManager::Instance().GameInstance());
    if (gameInstance) {
        auto* shardManager = gameInstance->GetShardManager();
        if (shardManager && shardManager->ShardMasterTable) {
            for (const auto& [rowName, appearance] : originalRandomizedShardAppearances) {
                auto* row = FindShardMasterRow(shardManager->ShardMasterTable, rowName);
                if (!row) continue;
                row->ShardType = appearance.type;
                row->ShardColorOverride = appearance.color;
            }
            originalRandomizedShardAppearances.clear();
        }
    }

    auto* dropManager = SDK::UPBDropManager::GetDropManager();
    if (dropManager && dropManager->DropTable) {
        for (const auto& [rowName, dropFlag] : originalRandomizedDropFlags) {
            auto* row = FindDropMasterRow(dropManager->DropTable, rowName);
            if (row) row->DropSpecialFlags = dropFlag;
        }
        originalRandomizedDropFlags.clear();
    }
}

void HookManager::ApplyCompatibilityShardMasterData() {
    auto* archipelago = Archipelago::ConnectedInstance();
    if (!archipelago) return;

    ResetCompatibilityShardMasterData();

    auto* gameInstance = static_cast<SDK::UPBGameInstance*>(GameManager::Instance().GameInstance());
    if (!gameInstance) return;
    auto* shardManager = gameInstance->GetShardManager();
    if (!shardManager || !shardManager->ShardMasterTable) return;
    auto* dropManager = SDK::UPBDropManager::GetDropManager();

    size_t patchedRows = 0;
    for (const auto& [randomizedName, vanillaName] : knownVanillaShardIds) {
        SDK::FName vanillaShardId = FNameFromString(vanillaName);
        ItemLookupResult itemResult = GetVanillaShardItemSupport(*archipelago, vanillaShardId);
        if (itemResult == ItemLookupResult::NotReady) {
            ResetCompatibilityShardMasterData();
            return;
        }
        if (itemResult == ItemLookupResult::KnownItem) continue;

        auto* randomizedRow = FindShardMasterRow(shardManager->ShardMasterTable, randomizedName);
        auto* vanillaRow = FindShardMasterRow(shardManager->ShardMasterTable, vanillaName);
        if (!randomizedRow || !vanillaRow) continue;

        originalRandomizedShardAppearances[randomizedName] =
            ShardAppearance{randomizedRow->ShardType, randomizedRow->ShardColorOverride};
        randomizedRow->ShardType = vanillaRow->ShardType;
        randomizedRow->ShardColorOverride = vanillaRow->ShardColorOverride == SDK::EShardColor::None
                                                  ? DefaultShardColor(vanillaRow->ShardType)
                                                  : vanillaRow->ShardColorOverride;

        if (dropManager && dropManager->DropTable && randomizedName.starts_with("AP_")) {
            std::string dropRowName = randomizedName.substr(3);
            auto* dropRow = FindDropMasterRow(dropManager->DropTable, dropRowName);
            if (dropRow) {
                originalRandomizedDropFlags[dropRowName] = dropRow->DropSpecialFlags;
                dropRow->DropSpecialFlags = SDK::EDropSpecialFlag::None;
            }
        }
        patchedRows++;
    }

    Logger::Log("[Shard] Applied old-world compatibility to ", patchedRows, " shard rows");
}
