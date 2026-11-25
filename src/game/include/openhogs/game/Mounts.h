// Mounts.h
//
// The buildings and vehicles a hog can get into: the artillery gun
// (BIG_GUN), pillboxes, shelters and tents, tanks and troop carriers.
//
// A hog inside is safe: the object takes the damage instead, and when it
// is destroyed the hog comes out hurt (20 points from a building, 40 from
// a vehicle). Inside, the hog uses the object's own weapons, each with a
// number of shots that stays with the object; tanks and troop carriers
// can be driven; tents heal the hog inside when its turn starts.
//
// Which objects these are, their weapons and their rules follow the Hogs
// of War Wiki's pages on the vehicles and buildings (see the README's
// credits); the objects' health comes from the executable (Destructibles).

#pragma once

#include "openhogs/formats/MapObjects.h"
#include "openhogs/game/Weapons.h"

#include <glm/glm.hpp>

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace openhogs::game {

enum class MountKind {
    Artillery,      // BIG_GUN: six kinds of shell, aimed 0-45 degrees up
    Pillbox,        // heavy machine gun and flame thrower
    Shelter,        // SHELTER and TENT_S: protection only
    MedicTent,      // M_TENT1: +50 health when the hog's turn starts
    MashTent,       // M_TENT2: full health when the hog's turn starts
    Tank,           // TANK, AM_TANK: drivable, bazooka and one airburst
    TroopCarrier,   // CARRY: drivable, no weapons
};

struct MountSpec {
    MountKind kind = MountKind::Shelter;
    bool vehicle = false;           // VEHICLE INOUT (else BUILDING INOUT)
    bool drivable = false;
    float damageWhenDestroyed = 20.0f;  // to the hog inside
    float lowestAimDegrees = -30.0f;
    float highestAimDegrees = 60.0f;
    // Weapons, with the number of uses each (0: never runs out).
    std::vector<std::pair<WeaponId, int>> arsenal;
};

// What a map object can be got into as, from its model name; empty for
// anything else.
std::optional<MountKind> mountKind(std::string_view modelName);

const MountSpec& mountSpec(MountKind kind);

// One building or vehicle of a match.
struct Mount {
    std::size_t object = 0;  // index in the .POG
    MountKind kind = MountKind::Shelter;
    std::string model;       // its model's name (BIG_GUN, TANK...): what its label calls it
    std::optional<std::size_t> occupant;  // the hog inside
    bool destroyed = false;
    // Uses left of its weapons that run out (MountSpec::arsenal).
    std::map<WeaponId, int> ammo;
    // How far it was driven from where the map put it, and how much it
    // turned (radians, round its starting centre `origin`).
    glm::vec3 origin{0.0f};
    glm::vec3 moved{0.0f};
    float turned = 0.0f;
    // The occupant's own weapons, given back when it gets out.
    std::vector<WeaponId> savedWeapons;
    std::size_t savedOwnWeapons = 0;
    std::map<WeaponId, int> savedAmmo;
    std::size_t savedSlot = 0;
};

// An object of a map a hog can get into.
struct FoundMount {
    std::size_t object = 0;  // index in the .POG
    MountKind kind = MountKind::Shelter;
    std::string model;       // upper case
};
std::vector<FoundMount> findMounts(const std::vector<formats::MapObject>& objects);

} // namespace openhogs::game
