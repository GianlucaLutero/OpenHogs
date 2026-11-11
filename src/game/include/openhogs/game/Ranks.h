// Ranks.h
//
// The ranks of the campaign and how a hog climbs them.
//
// Every hog of the player's squad starts as a Grunt. Its first promotion
// chooses a career, which it can never leave; each career has three
// ranks, after which every career leads to Commando and finally Hero:
//
//                 ┌─ Heavy weapons: Gunner → Bombardier → Pyrotechnic ─┐
//     Grunt ──────┼─ Medic:         Orderly → Medic → Surgeon         ─┼── Commando → Hero
//                 ├─ Espionage:     Scout → Sniper → Spy              ─┤
//                 └─ Engineer:      Sapper → Engineer → Saboteur      ─┘
//
// Health and weapons follow the game's manual and the player guides; only
// the weapons OpenHogs implements are given (the others, such as the
// flame thrower or healing hands, come later: docs/open-decisions.md).
// The names are entries of the game text table gtext (63 GRUNT ... 79
// LEGEND), so language packs translate them.

#pragma once

#include "openhogs/game/Weapons.h"

#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace openhogs::game {

enum class RankId {
    Grunt,
    Gunner, Bombardier, Pyrotechnic,  // heavy weapons
    Orderly, Medic, Surgeon,          // medic
    Scout, Sniper, Spy,               // espionage
    Sapper, Engineer, Saboteur,       // engineer
    Commando, Hero,                   // officers
};

enum class Career { None, HeavyWeapons, Medic, Espionage, Engineer, Officer };

struct RankSpec {
    RankId id = RankId::Grunt;
    std::string_view key;           // stable name for save files: "grunt"
    std::size_t textEntry = 0;      // gtext entry with the rank's name
    std::string_view fallbackName;  // English, if the text table is missing
    Career career = Career::None;
    int level = 0;                  // promotions needed to get here (Grunt 0 ... Hero 5)
    float maxHealth = 50.0f;
    std::string_view classId;       // body model (scene::characterClasses id)
    std::vector<WeaponId> weapons;
};

const RankSpec& rankSpec(RankId rank);
std::span<const RankSpec> allRanks();

// The rank with this save-file key; empty if there is none.
std::optional<RankId> findRank(std::string_view key);

// The ranks a hog can be promoted to from `rank`: the four first career
// ranks from Grunt, then the next rank of its career, Commando after the
// third career rank, Hero after Commando; none from Hero.
std::vector<RankId> promotionsFrom(RankId rank);

// Promotion points a promotion to `rank` costs: its level (1 for a first
// career rank ... 5 for Hero), so higher ranks are worth saving for.
int promotionCost(RankId rank);

} // namespace openhogs::game
