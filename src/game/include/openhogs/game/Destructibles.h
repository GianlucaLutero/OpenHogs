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
// Hogs, crates and animals are not handled here.

#pragma once

#include "openhogs/formats/MapObjects.h"

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

namespace openhogs::game {

enum class ObjectClass { Building, Vehicle, Scenery, Other };

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

    // Buildings and vehicles blow up when destroyed; scenery just goes.
    bool blowsUp() const { return objectClass != ObjectClass::Scenery; }
};

// The objects of a map that can be destroyed.
std::vector<Destructible> findDestructibles(const std::vector<formats::MapObject>& objects);

// Takes `amount` of health; returns true if that destroyed it. As in the
// original, an object is gone once it has less than one point left.
bool damageObject(Destructible& target, float amount);

} // namespace openhogs::game
