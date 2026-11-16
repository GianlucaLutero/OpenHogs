// Training.cpp - see Training.h for an overview.

#include "openhogs/game/Training.h"

#include "openhogs/core/PathUtils.h"

#include <cstdio>

namespace openhogs::game {

namespace {

// The crate items (weapon number = text entry - 96) the Boot Camp uses.
constexpr int kBayonet = 3, kRifle = 7, kSniperRifle = 11, kGrenade = 19, kBazooka = 29, kTnt = 37;

} // namespace

bool isTrainingMap(std::string_view mapName) {
    return core::toUpper(mapName) == kTrainingMap;
}

TrainingTip trainingStartTip() {
    return {211, 2};
}

std::optional<TrainingTip> tipWhenCrateAppears(const Pickup& crate) {
    // FUN_00465ab0, by the crate's weapon (and how many grenades).
    if (crate.kind != PickupKind::Weapon) return std::nullopt;
    switch (crate.item) {
    case kRifle: return TrainingTip{215, 6};        // FOLLOW THE YELLOW PATH ...
    case kSniperRifle: return TrainingTip{219, 10};  // COLLECT THE CRATE.
    case kGrenade:
        if (crate.amount == 5) return TrainingTip{221, 12};   // FOLLOW THE RED PATH ...
        if (crate.amount == 10) return TrainingTip{224, 15};  // FOLLOW THE BLUE PATH ...
        return std::nullopt;
    case kBazooka: return TrainingTip{231, 22};  // ENTER THE BUILDING ...
    case kTnt: return TrainingTip{226, 17};      // FOLLOW THE PURPLE PATH ...
    default: return std::nullopt;
    }
}

std::optional<TrainingTip> tipWhenCratePicked(const Pickup& crate) {
    // FUN_00465cb0: health crates by amount, weapons by item.
    if (crate.kind == PickupKind::Health) {
        switch (crate.amount) {
        case 10: return TrainingTip{227, 18};  // USE SHIFT BUTTON TO JUMP THE GAP.
        case 15: return TrainingTip{228, 19};  // FOLLOW THE PATH THROUGH THE MINEFIELD ...
        case 20: return TrainingTip{232, 23};  // FOLLOW THE PATH THROUGH THE MINEFIELD ...
        case 25: return TrainingTip{233, 24};  // USE BACKSPACE BUTTON TO ENTER ...
        default: return std::nullopt;
        }
    }
    if (crate.kind != PickupKind::Weapon) return std::nullopt;
    switch (crate.item) {
    case kBayonet: return TrainingTip{212, 3};       // PRESS RETURN BUTTON FOR SKILL MENU.
    case kRifle: return TrainingTip{217, 8};         // ... AND SPACE TO SELECT YOUR WEAPON.
    case kSniperRifle: return TrainingTip{220, 11};  // HOLD CTRL BUTTON TO AIM ...
    case kGrenade:
        if (crate.amount == 5) return TrainingTip{222, 13};   // HOLD SPACE TO SET POWER ...
        if (crate.amount == 10) return TrainingTip{225, 16};  // ... SET A FLAT AIM ANGLE.
        return std::nullopt;
    case kBazooka: return TrainingTip{234, 25};  // HOLD SPACE TO SET POWER AND RELEASE TO FIRE.
    case kTnt: return TrainingTip{229, 20};      // PRESS SPACE TO SET CHARGE ...
    default: return std::nullopt;
    }
}

std::optional<TrainingTip> tipWhenWeaponTaken(WeaponId weapon) {
    // FUN_00493020 (when a weapon is chosen), by the weapon in hand.
    switch (weapon) {
    case WeaponId::Bayonet: return TrainingTip{214, 5};  // PRESS SPACE TO ATTACK THE DUMMY.
    case WeaponId::Rifle: return TrainingTip{218, 9};    // HOLD CTRL BUTTON TO AIM - PRESS SPACE TO FIRE.
    case WeaponId::Grenade: return TrainingTip{223, 14}; // HOLD SPACE BUTTON TO SET POWER ...
    default: return std::nullopt;
    }
}

std::string trainingSpeechPath(int line) {
    char file[48];
    std::snprintf(file, sizeof file, "Speech/Sku1/Train1/tr1_en%02d.wav", line);
    return file;
}

} // namespace openhogs::game
