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

#include <algorithm>
#include <cstddef>
#include <map>
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

// What a hog did in the match, for the battle results (BattleAwards.h).
// As in the executable, everything that happens in a hog's turn is put
// down to that hog (hog + 0x1C6 ... 0x1D1, counted by FUN_00467ac0 and
// FUN_00467db0).
struct HogRecord {
    int enemiesKilled = 0;
    int friendsKilled = 0;       // its own team's hogs, itself included
    float enemyDamage = 0.0f;    // health taken from enemies
    float friendDamage = 0.0f;
    int killsThisTurn = 0;
    int mostKillsInATurn = 0;
};

struct Hog {
    const scene::CharacterClass* characterClass = nullptr;  // never null for spawned hogs
    int team = 0;                   // 0 to 7, from the object's team bit mask
    bool playerControlled = false;  // the map marks hogs meant for the player
    std::size_t object = 0;         // its map object (index in the .POG): objectives sit on it
    HogRecord record;               // kills and damage it dealt
    glm::vec3 position{0.0f};       // where its feet are, world units
    float yawRadians = 0.0f;        // turn around the vertical axis, as for map objects
    AnimationPlayer animation;      // what its body is doing
    IdleAnimator idle;              // chooses the animation while it waits
    HogMotion motion;               // walking, jumping, swimming...
    std::optional<HeldAnimation> held;  // overrides the idle behaviour when set

    float waterDamageOwed = 0.0f;   // swimming damage not yet charged (it is charged in whole points)
    float secondsSwumInItsTurn = 0.0f;  // swimming under the player's control, this turn
    // A dead hog blows up once, when its dying (or sinking) animation is
    // over; on land only its boots are left (World::explodeDeadHogs).
    bool blownUp = false;

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

    std::vector<WeaponId> weapons;  // what it carries: its own, then its team's stock
    // How many of `weapons` are its own (from its class or rank); the rest
    // come from crates its team picked up and can run out.
    std::size_t ownWeapons = static_cast<std::size_t>(-1);
    std::size_t weaponSlot = 0;     // the selected one
    // Uses left of its own weapons that do not last for ever (a Bombardier
    // has three mortar shells); a weapon not listed never runs out.
    std::map<WeaponId, int> ownAmmo;

    // What gases, darts and hiding did to it (Weapons.h, WeaponEffect):
    bool poisoned = false;          // loses 10 points when each of its turns starts
    bool asleep = false;            // misses its next turn
    float madnessSecondsLeft = 0.0f;  // runs straight ahead until it is over
    float hideShield = 0.0f;        // > 0: hidden in a disguise that takes this much damage
    bool confused = false;          // its next turn will be played with reversed controls
    bool controlsReversed = false;  // this turn: forward is back, left is right

    // Flying (Weapons.h, JETPACK and SPECIAL OPS): seconds of jetpack flight
    // left, and whether it comes down slowly under a parachute (no harm
    // when it lands).
    float jetpackSecondsLeft = 0.0f;
    bool parachute = false;

    // Choosing the spot of an airstrike or of special ops. As the manual
    // says, these weapons "present you with an aerial view of the map":
    // the player moves a crosshair with the movement keys and fires again
    // to confirm; with an airstrike the aim keys turn the way the airship
    // will come in from (the manual's Page Up and Page Down). Empty when
    // nothing is being chosen.
    struct StrikeAim {
        glm::vec3 spot{0.0f};     // the crosshair, on the ground
        float viewYaw = 0.0f;     // the aerial view looks this way (radians, as yawRadians)
        float approachYaw = 0.0f; // the airship flies this way over the spot
    };
    std::optional<StrikeAim> strikeAim;

    // Inside a building or vehicle (index into World::mounts()): safe from
    // harm, using the object's weapons. Empty when out in the open.
    std::optional<std::size_t> mount;
    bool sheltered() const { return mount.has_value(); }
    // Inside, its shots leave from the object's barrel: this high above the
    // hog's position and this far ahead (World::muzzlePosition).
    float mountMuzzleHeight = 0.0f;
    float mountMuzzleReach = 0.0f;
    // How many entries at the end of `weapons` are actions the world adds
    // by itself (getting in or out, SKIP TURN), not the hog's own.
    std::size_t footerActions = 0;
    // The rank the map gave it (its object's type field, see
    // rankFromMapType); -1 when not from a map.
    int mapRank = -1;
    // How many entries at the start of `weapons` are its own (the rest are
    // the team's stock, then the actions above).
    std::size_t ownCount() const {
        const std::size_t listed = weapons.size() - std::min(footerActions, weapons.size());
        return std::min(ownWeapons, listed);
    }

    // MAP VIEW or BINOCULARS switched on (Weapons.h): how the player looks.
    enum class Looking { Normally, AtTheMap, ThroughBinoculars };
    Looking looking = Looking::Normally;
    bool hidden() const { return hideShield > 0.0f; }

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

// Swimming, as the Hogs of War Wiki's Water page says (decision 9): the
// two officer classes, ace and legend, swim without harm; the others lose
// about 2 points a second. A hog the player steers into the water in its
// own turn loses more and more, up to about 20 a second after some 3
// seconds; a hog knocked in has no such ramp and swims back to the shore.
// A hog that drowns explodes once it has sunk, hurting those nearby.
bool swimsSafely(const Hog& hog);
inline constexpr float kSwimDamagePerSecond = 2.0f;
inline constexpr float kSteeredSwimDamagePerSecond = 20.0f;
inline constexpr float kSteeredSwimRampSeconds = 3.0f;
// The damage per second of a hog that has swum `steeredSeconds` under the
// player's control in its turn (0 for a hog nobody steers).
float swimDamagePerSecond(float steeredSeconds);
// The explosion of a dead hog, drowned (the wiki gives no figures, so a
// grenade's reach and the jetpack's 20 points, decision 149) or on land
// (the same, decision 156).
inline constexpr float kDrownedBlastRadius = 325.0f;
inline constexpr float kDrownedBlastDamage = 20.0f;

// The centre of the hog's body (half way up), the point explosions and
// bullets aim at.
glm::vec3 hogCentre(const Hog& hog);

// Matrix that takes the hog's posed model (model units, Y down, origin at
// the hips) to its place in the world.
glm::mat4 hogModelToWorld(const Hog& hog);

} // namespace openhogs::game
