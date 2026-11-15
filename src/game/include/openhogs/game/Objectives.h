// Objectives.h
//
// Mission objectives: the jobs a map gives through the script type of its
// objects (.POG, docs/formats/maps.md "Objectives"). Doing one drops a
// reward crate or, for a few, costs promotion points:
//
//   - reach objectives (7, 8, 10) sit on one of the player's hogs: when
//     that hog, in its own turn, walks into the target cell (the object's
//     script position), the reward appears at its feet;
//   - destroy objectives (1, 2, 4) sit on something to destroy, often an
//     enemy hog: when it goes, the reward appears at the target position;
//   - group objectives (13 - 18) belong to a group of objects (the script
//     group): only when the last of them goes is the reward given.
//
// The rewards follow warhogs_.exe (FUN_004a7730 for reaching,
// FUN_004aa170 for destroying):
//
//   type | done by     | reward
//   -----+-------------+-------------------------------------------------
//    1   | destroying  | crate: weapon (item, amount) or health (item 255)
//    2   | destroying  | promotion point
//    4   | destroying  | the player loses `item` promotion points
//    7   | reaching    | crate, as type 1
//    8   | reaching    | promotion point
//   10   | reaching    | the player loses `item` promotion points
//   13   | group gone  | crate, as type 1
//   14   | group gone  | promotion point
//   16   | group gone  | the player loses `item` promotion points
//
// Other script types (9, 15, 17, 18) count in their group but give
// nothing. Types 20 to 23 belong to the Boot Camp and are not handled here.
//
// This module only decides; World applies the result (adds the crate,
// reports the event).

#pragma once

#include "openhogs/formats/MapObjects.h"
#include "openhogs/game/Pickups.h"

#include <glm/glm.hpp>

#include <cstddef>
#include <optional>
#include <vector>

namespace openhogs::game {

struct Objective {
    std::size_t object = 0;  // the map object it sits on (index in the .POG)
    int type = 0;            // the object's script type
    int group = 0;           // the script group (group objectives)
    // Script parameter 0: for crates the item, already turned into a
    // weapon number (text entry - 96) or 255 for health; for the types that
    // cost points, how many.
    int item = 0;
    int amount = 0;          // script parameter 1: how many (crates)
    glm::vec3 target{0.0f};  // the script position: a point on the ground
    bool done = false;       // done or gone: it counts no more
};

// What doing an objective brings.
struct ObjectiveResult {
    std::optional<Pickup> drop;  // a crate to add to the map
    int pointsLost = 0;          // promotion points taken from the player
    int sergeantSection = -1;    // the sergeant's comment (sargeSpeechPath)
};

// The objectives among a map's objects (script types 1 to 18, except the
// crates themselves).
std::vector<Objective> findObjectives(const std::vector<formats::MapObject>& objects);

// The cell of the 32 x 32 grid the original compares positions in: a
// cell is 512 units wide and the map runs from -16384 to 16383.
int objectiveCell(float coordinate);

// Whether the hog carrying `objective`, standing at `feet`, has reached its
// target. Reaching it marks it done and returns what it brings. Only
// reach objectives (7 - 10) can be reached.
std::optional<ObjectiveResult> reachObjective(Objective& objective, const glm::vec3& feet);

// Map object `object` was destroyed (or the hog it stands for died): marks
// its objective done and returns what that brings, if anything. For group
// objectives the reward comes only with the last of the group. A reach
// objective whose hog dies is simply lost.
std::optional<ObjectiveResult> destroyObjective(std::vector<Objective>& objectives, std::size_t object);

} // namespace openhogs::game
