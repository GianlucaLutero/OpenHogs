// Training.h
//
// The Boot Camp, the campaign's training mission on the CAMP map. The
// executable plays that map in a training mode (DAT_005207a8, set for
// level 10 only) with rules of its own:
//
//   - the hog starts without weapons (the class loadout is skipped) and
//     gets them from crates, which then never run out; a health crate
//     gives 50 points (FUN_00465420);
//   - crates with a group start hidden and are revealed one group after
//     another as the dummies, trees and the wall of the chain go
//     (Objectives.h, types 20-23);
//   - the instructions (gtext 211-234) appear when the first turn starts,
//     when a crate appears (FUN_00465ab0) and when it is picked up
//     (FUN_00465cb0), each with a line of the instructor
//     (Speech/Sku1/Train1/tr1_enNN.wav, FUN_0043b530).
//
// OpenHogs ends the training when every crate of the chain has been
// picked up (decision 118).

#pragma once

#include "openhogs/game/Pickups.h"
#include "openhogs/game/Weapons.h"

#include <optional>
#include <string>
#include <string_view>

namespace openhogs::game {

// The map played in training mode.
inline constexpr std::string_view kTrainingMap = "CAMP";
bool isTrainingMap(std::string_view mapName);

// What a training crate's health crate gives (FUN_00465420).
inline constexpr int kTrainingHealthCrate = 50;

struct TrainingTip {
    int text = 0;  // gtext entry
    int line = 0;  // the instructor's line (trainingSpeechPath)
};

// "FOLLOW THE GREEN PATH AND COLLECT THE CRATE." when the training starts.
TrainingTip trainingStartTip();

// The tip when a crate of the chain appears, if it has one.
std::optional<TrainingTip> tipWhenCrateAppears(const Pickup& crate);

// The tip when a crate is picked up, if it has one.
std::optional<TrainingTip> tipWhenCratePicked(const Pickup& crate);

// The tip on how to use a weapon from a crate, the first time the hog
// takes it in hand (bayonet, rifle, grenade).
std::optional<TrainingTip> tipWhenWeaponTaken(WeaponId weapon);

// The file of instructor line `line` (1-28), e.g. "Speech/Sku1/Train1/tr1_en02.wav".
std::string trainingSpeechPath(int line);

} // namespace openhogs::game
