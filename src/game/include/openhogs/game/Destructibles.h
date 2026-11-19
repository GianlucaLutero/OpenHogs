// Destructibles.h
//
// The map objects weapons can destroy: buildings (pillboxes, tents, big
// guns), vehicles (tanks, landing craft) and scenery (walls, houses,
// bridges, trees, fences...). Each has health, in the same points as a
// hog's; explosions and bullets take it away, and at 0 the object is gone.
//
// What counts as what, and how much health it has, follows warhogs_.exe:
//
//   - the executable's object table (0x4D9680) sorts model names into
//     buildings, scenery, vehicles, pickups, animals and hogs; the map
//     loader (FUN_004a5250) creates each kind with its own class;
//   - buildings take their health from 0x4C2E08, scenery from 0x4D6D18,
//     vehicles from 0x4D94A0 (all in 1/128 points);
//   - a map object can replace it (offset 56, MapObject::health);
//   - scenery marked indestructible (offset 58, bit 6) takes no damage
//     (FUN_0048d990 returns at once).
//
// The owner also wants crates, explosive drums and animals to be
// destroyable (decision 115; in the PC executable a crate's damage method
// does nothing): crates and animals simply go, a drum catches fire and
// blows up two seconds later as the wiki describes (World).
//
// Scenery that drops below two thirds of its health for the first time
// gets a quarter of its health back (FUN_0048d990, where the same moment
// also stops windmills and other moving parts); trees, bushes, flowers,
// grass and the moving machines do not.
//
// Hogs are not handled here.

#pragma once

#include "openhogs/formats/MapObjects.h"

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

namespace openhogs::game {

enum class ObjectClass { Building, Vehicle, Scenery, Crate, Drum, Animal, Other };

// The class of a map object, from its model name.
ObjectClass objectClass(std::string_view modelName);

// The health a map object starts with, or nothing if it cannot be
// destroyed (not a building, vehicle or scenery, or marked indestructible).
std::optional<float> objectHealth(const formats::MapObject& object);

struct Destructible {
    std::size_t object = 0;  // index in the .POG
    ObjectClass objectClass = ObjectClass::Scenery;
    float health = 0.0f;
    bool destroyed = false;
    float startingHealth = 0.0f;  // for the two-thirds rule (0: none)
    bool toughens = false;        // the two-thirds rule applies to it
    bool toughened = false;       // and has already been used
    bool poison = false;          // a poison drum (DRUM2)

    // Buildings and vehicles blow up when destroyed; scenery, crates and
    // animals just go (drums blow up too, after their fuse: World).
    bool blowsUp() const { return objectClass == ObjectClass::Building || objectClass == ObjectClass::Vehicle; }
};

// The objects of a map that can be destroyed.
std::vector<Destructible> findDestructibles(const std::vector<formats::MapObject>& objects);

// Health of the objects the owner added (decision 115, figures from the
// wiki where it has them): a drum catches fire after 20 points (Explosive
// barrel page); crates take 40 like the scenery CRATE4; animals one hit.
inline constexpr float kCrateHealth = 40.0f;
inline constexpr float kDrumHealth = 20.0f;
inline constexpr float kAnimalHealth = 1.0f;

// A drum that has caught fire blows up this long after (the wiki), up to
// 75 points (an explosive drum) or 15 and poison (a poison drum, smaller).
inline constexpr float kDrumFuseSeconds = 2.0f;
inline constexpr float kDrumBlastDamage = 75.0f;
inline constexpr float kDrumBlastRadius = 600.0f;
inline constexpr float kDrumKnockback = 1200.0f;
inline constexpr float kPoisonDrumDamage = 15.0f;
inline constexpr float kPoisonDrumRadius = 250.0f;
bool isPoisonDrum(std::string_view modelName);

// Takes `amount` of health; returns true if that destroyed it. As in the
// original, an object is gone once it has less than one point left.
bool damageObject(Destructible& target, float amount);

} // namespace openhogs::game
