// Hog.h
//
// One hog of a match, and how hogs are created from a map.
//
// Maps place the hogs with objects named after their class: GR_ME is a
// grunt, HV_ME a heavy weapons hog, SN_ME a sniper... (the two letters are
// the prefix of the class's model, see scene::characterClasses). The
// object also says which team the hog belongs to and where it faces.
// See docs/formats/maps.md.

#pragma once

#include "openhogs/formats/MapObjects.h"
#include "openhogs/game/AnimationPlayer.h"
#include "openhogs/game/HogMotion.h"
#include "openhogs/game/IdleAnimator.h"
#include "openhogs/game/TerrainHeightField.h"
#include "openhogs/scene/CharacterLibrary.h"

#include <glm/glm.hpp>

#include <string_view>
#include <vector>

namespace openhogs::game {

struct Hog {
    const scene::CharacterClass* characterClass = nullptr;  // never null for spawned hogs
    int team = 0;                   // 0 to 7, from the object's team bit mask
    bool playerControlled = false;  // the map marks hogs meant for the player
    glm::vec3 position{0.0f};       // where its feet are, world units
    float yawRadians = 0.0f;        // turn around the vertical axis, as for map objects
    AnimationPlayer animation;      // what its body is doing
    IdleAnimator idle;              // chooses the animation while it waits
    HogMotion motion;               // walking, jumping, swimming...
};

// The class a map object stands for ("GR_ME" -> grunt), or nullptr if the
// object is not a hog. Case-insensitive.
const scene::CharacterClass* hogClassForObject(std::string_view modelName);

// Creates the hogs of a map, in the order of its objects. Hogs stand on
// the terrain; a hog the map places well above the ground (on a bridge, a
// roof, a tower) keeps that height.
std::vector<Hog> spawnHogs(const std::vector<formats::MapObject>& objects, const TerrainHeightField& terrain);

// The nation whose uniform a team wears (a name of scene::characterTeams).
// In the original the campaign and the players decide it; until those
// exist, teams simply take the nations in order.
std::string_view teamNation(int team);

// Unit vector of the direction the hog faces, parallel to the ground.
glm::vec3 hogFacing(const Hog& hog);

// Matrix that takes the hog's posed model (model units, Y down, origin at
// the hips) to its place in the world.
glm::mat4 hogModelToWorld(const Hog& hog);

} // namespace openhogs::game
