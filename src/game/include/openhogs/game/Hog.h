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
#include "openhogs/game/Ranks.h"
#include "openhogs/game/TerrainHeightField.h"
#include "openhogs/game/Weapons.h"
#include "openhogs/scene/CharacterLibrary.h"

#include <glm/glm.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace openhogs::game {

// Health of an unhurt hog. The original shows health as a number up to
// 100; whether every class starts with 100 is still open
// (docs/open-decisions.md).
inline constexpr float kMaxHealth = 100.0f;

// Aim limits, in degrees above (+) or below (-) the horizon.
inline constexpr float kLowestAimDegrees = -30.0f;
inline constexpr float kHighestAimDegrees = 80.0f;

// An animation the hog holds while it stands still instead of its idle
// behaviour: taking out its weapon, the aiming pose, the firing animation.
struct HeldAnimation {
    std::size_t animation = 0;
    AnimationPlayer::Repeat repeat = AnimationPlayer::Repeat::Loop;
    float frame = 0.0f;  // Repeat::Hold: the frame shown (the aiming angle)
};

struct Hog {
    const scene::CharacterClass* characterClass = nullptr;  // never null for spawned hogs
    int team = 0;                   // 0 to 7, from the object's team bit mask
    bool playerControlled = false;  // the map marks hogs meant for the player
    glm::vec3 position{0.0f};       // where its feet are, world units
    float yawRadians = 0.0f;        // turn around the vertical axis, as for map objects
    AnimationPlayer animation;      // what its body is doing
    IdleAnimator idle;              // chooses the animation while it waits
    HogMotion motion;               // walking, jumping, swimming...
    std::optional<HeldAnimation> held;  // overrides the idle behaviour when set

    float secondsInWater = 0.0f;    // swimming time not yet charged as damage

    float health = kMaxHealth;
    float maxHealth = kMaxHealth;   // unhurt health (campaign ranks differ)
    bool alive = true;

    // Campaign squad hogs only: the pig's name and rank (shown by the HUD).
    std::string name;
    std::optional<RankId> rank;
    std::optional<std::size_t> squadIndex;  // its place in the campaign squad

    // The weapon in its hand, shown with its model: taken out when the hog
    // stands still in its turn, put away when it moves (as the original).
    std::optional<WeaponId> drawnWeapon;

    std::vector<WeaponId> weapons;  // what it carries (from its class)
    std::size_t weaponSlot = 0;     // the selected one
    float aimDegrees = 15.0f;       // above the horizon
    float power = 0.0f;             // 0 to 1, while charging a throw
    bool charging = false;          // the fire key is held for a powered weapon

    WeaponId selectedWeapon() const {
        return weapons.empty() ? WeaponId::Trotter : weapons[weaponSlot % weapons.size()];
    }
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

// The yaw (Hog::yawRadians) that faces along `direction` (its height is
// ignored).
float yawFacing(const glm::vec3& direction);

// Swimming hurts every hog except the two officer ranks, Commando (the
// "ace" class) and Hero ("legend"), as the game's guides say: they swim
// without harm. The others lose kSwimDamagePerSecond health every second
// spent in the water ("even a full health pig drowns very quickly"; the
// exact rate is not known, see docs/open-decisions.md).
bool swimsSafely(const Hog& hog);
inline constexpr float kSwimDamagePerSecond = 10.0f;

// The centre of the hog's body (half way up), the point explosions and
// bullets aim at.
glm::vec3 hogCentre(const Hog& hog);

// Matrix that takes the hog's posed model (model units, Y down, origin at
// the hips) to its place in the world.
glm::mat4 hogModelToWorld(const Hog& hog);

} // namespace openhogs::game
