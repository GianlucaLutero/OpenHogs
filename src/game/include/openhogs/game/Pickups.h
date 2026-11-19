// Pickups.h
//
// The crates and other things a hog can pick up on a map. In the map's
// objects (.POG) they have script type 19; their parameters say what they
// hold (docs/formats/maps.md, "Crates"):
//
//   - parameter 0 = 255: a health crate, parameter 1 = how much health;
//   - the PROPOINT model: promotion points for the campaign;
//   - otherwise a weapon crate: parameter 0 is an item number that the
//     executable turns into a weapon through its table at 0x4D9DC8 (the
//     weapon is then text entry 96 + that number), parameter 1 how many.
//
// A hog picks a crate up by walking into it; the crate disappears.

#pragma once

#include "openhogs/formats/MapObjects.h"
#include "openhogs/game/Weapons.h"

#include <glm/glm.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace openhogs::game {

enum class PickupKind { Weapon, Health, PromotionPoints };

struct Pickup {
    std::size_t object = 0;  // index of its map object (to hide it)
    glm::vec3 centre{0.0f};
    glm::vec3 halfSize{0.0f};  // the crate's box (unturned), for touching it
    PickupKind kind = PickupKind::Weapon;
    int item = 0;  // weapons: the text entry minus 96 (WeaponId - 96)
    int amount = 0;
    bool taken = false;
    // The model drawn for it: "CRATE1" (weapons), "CRATE2" (health) or
    // "PROPOINT" (promotion points).
    std::string model;
    // A reward dropped by an objective during the match (Objectives.h)
    // rather than one of the map's crates; `object` is then a number
    // after the map's objects, so it can be shown and hidden the same way.
    bool dropped = false;
    // Hidden until revealed (a Boot Camp crate, see Training.h): it cannot
    // be picked up or seen.
    bool hidden = false;
    // How far above its place it still is while it comes down by
    // parachute (dropped rewards and revealed crates); it can only be
    // picked up once it has landed.
    float dropHeight = 0.0f;
    bool falling() const { return dropHeight > 0.0f; }

    // The weapon a weapon crate holds, if OpenHogs has it.
    std::optional<WeaponId> weapon() const;
};

// Crates that appear during a match come down by parachute from 0xC00
// above their place (FUN_004aa170, decision 111), at kCrateFallSpeed
// (the speed is OpenHogs' choice, decision 157).
inline constexpr float kCrateDropHeight = 3072.0f;
inline constexpr float kCrateFallSpeed = 600.0f;

// The map script type of a pickup.
inline constexpr int kPickupScriptType = 19;

// The pickups among a map's objects.
std::vector<Pickup> findPickups(const std::vector<formats::MapObject>& objects);

// The weapon number (text entry - 96) a crate's item number stands for
// (the executable's table at 0x4D9DC8), or -1.
int crateItemWeapon(int item);

// Whether a hog with its feet at `feet` touches the pickup.
bool touches(const Pickup& pickup, const glm::vec3& feet);

} // namespace openhogs::game
