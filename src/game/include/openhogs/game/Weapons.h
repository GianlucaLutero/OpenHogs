// Weapons.h
//
// The weapons of the game: what each one does and which hog carries which.
//
// The original lists its weapons, in this order, in the game text table
// (Language/Text/gtext): entry 97 is TROTTER (the hog's fist), then KNIFE,
// BAYONET, SWORD... up to SURRENDER. A weapon is identified here by that
// text entry, so its name always comes from the text table, in the
// player's language (an Italian pack translates it for free).
//
// Weapons work in a few ways (WeaponKind):
//   - Melee:  hits a hog right in front (fist, knife, sword);
//   - Bullet: hits the first thing along the line of fire, at once
//             (pistol, rifle, machine guns, shotgun);
//   - Thrown: a grenade that bounces and explodes when its fuse runs out;
//   - Shell:  a projectile that explodes when it hits something (bazooka,
//             mortar, rocket);
//   - Placed: put down at the hog's feet (TNT with a fuse, mines that go
//             off when a hog comes near).
// Shells may also steer themselves (homing missile) or be steered by the
// player (guided missile): see Guidance.
//
// The numbers (damage, blast radius, speed...) are OpenHogs' own until the
// original's are known: see docs/open-decisions.md.

#pragma once

#include <cstddef>
#include <span>
#include <string_view>

namespace openhogs::game {

// Text table entries of the weapons OpenHogs implements so far.
enum class WeaponId {
    Trotter = 97,
    Knife = 98,
    Bayonet = 99,
    Sword = 100,
    CattleProd = 101,
    Pistol = 102,
    Rifle = 103,
    RifleBurst = 104,
    MachineGun = 105,
    HeavyMachineGun = 106,
    SniperRifle = 107,
    Shotgun = 108,
    SuperShotgun = 109,
    FlameThrower = 110,
    RocketLauncher = 111,
    GuidedMissile = 112,
    MedicineDart = 113,
    Tranquiliser = 114,
    Grenade = 115,
    ClusterGrenade = 116,
    HighExplosiveGrenade = 117,
    RollerGrenade = 118,
    ConfusionGas = 119,
    FreezeGas = 120,
    MadnessGas = 121,
    PoisonGas = 122,
    ShrapnelGrenade = 123,
    Mortar = 124,
    Bazooka = 125,
    GrenadeLauncher = 126,
    Airburst = 127,
    SuperAirburst = 128,
    MedicineBall = 129,
    HomingMissile = 130,
    Mine = 131,
    AntiPersonnelMine = 132,
    Tnt = 133,
    SuperTnt = 134,
    LongRangeShell = 135,
    MineShell = 136,
    GasShell = 137,
    FireRainShell = 138,
    ThousandPoundShell = 139,
    ShockShell = 140,
    Jetpack = 141,
    Suicide = 142,
    HealingHands = 143,
    SelfHeal = 144,
    PickPocket = 145,
    Hide = 146,
    Shockwave = 147,
    SpecialOps = 148,
    Airstrike = 149,
    FireRainAirstrike = 150,
    VehicleInOut = 151,
    BuildingInOut = 152,
    EjectPig = 153,
    MapView = 154,
    Binoculars = 155,
    SkipTurn = 156,
    Surrender = 157,
};

// Melee, Bullet, Thrown, Shell and Placed hurt; the others are actions:
//   - Heal: health for the hog itself (range 0) or the nearest friend in
//     reach (healing hands);
//   - Suicide: the hog blows itself up (blastRadius, damage);
//   - SkipTurn: the turn ends at once;
//   - Surrender: the team gives up: all its hogs fall.
//   - Shockwave: every hog around is thrown up and away;
//   - Hide: the hog hides in a disguise until its next turn;
//   - PickPocket: steals a weapon from the nearest enemy in reach;
//   - Jetpack: the hog flies off along its aim; fire again (or the fuel
//     running out) drops the jetpack, which explodes where it lands
//     (damage, blastRadius), and a parachute brings the hog down; the
//     turn goes on;
//   - SpecialOps: the hog is dropped by parachute on a chosen spot;
//   - Airstrike: bombs fall from the sky along a line over a chosen spot;
//   - InOut: gets into the building or vehicle next to the hog, or out of
//     the one it is in (Mounts.h);
//   - MapView, Binoculars: other ways of looking, switched on and off; they
//     do not use up the turn.
// The chosen spot of SpecialOps and Airstrike lies straight ahead of the
// hog, as far as the power the player charges (up to `range`).
enum class WeaponKind {
    Melee, Bullet, Thrown, Shell, Placed, Heal, Suicide, SkipTurn, Surrender, Shockwave, Hide, PickPocket,
    Jetpack, SpecialOps, Airstrike, InOut, MapView, Binoculars
};

// How a shell finds its way:
//   - None: a plain ballistic flight;
//   - Homing: after a moment it turns towards the nearest enemy;
//   - Guided: the player steers it while it flies (turn and aim keys).
enum class Guidance { None, Homing, Guided };

// What a weapon does to the hogs it reaches besides harm (the wiki's
// descriptions of the gases, darts and medicine):
//   - Heal: gives `damage` health instead of taking it (and cures);
//   - Poison: 10 points lost at the start of each of the hog's turns;
//   - Sleep: the hog misses its next turn;
//   - Madness: the hog runs straight ahead for a while;
//   - Confusion: during the hog's next turn its controls are reversed.
enum class WeaponEffect { None, Heal, Poison, Sleep, Madness, Confusion };

struct WeaponSpec {
    WeaponId id = WeaponId::Trotter;
    WeaponKind kind = WeaponKind::Melee;
    std::string_view fallbackName;  // English, if the text table is missing

    // Damage to a hog hit directly (melee, bullet) or at the centre of the
    // explosion; it falls off linearly to 0 at the edge of blastRadius.
    float damage = 0.0f;
    float blastRadius = 0.0f;     // world units; 0 = no explosion
    float range = 0.0f;           // melee and bullets: how far they reach

    // Bullets: how many shots, how far apart in time, how many pellets per
    // shot (shotguns), and the spread around the aim.
    int shots = 1;
    float secondsBetweenShots = 0.0f;
    int pellets = 1;
    float spreadDegrees = 0.0f;

    // Thrown and shells: launch speed at full power, whether the player
    // sets the power (hold the fire key), and how strongly gravity pulls.
    float launchSpeed = 0.0f;
    bool usesPower = false;
    float gravityFactor = 1.0f;

    // Thrown and placed: seconds until it explodes; bounciness when it
    // hits the ground (0 = stops dead, 1 = keeps all its speed).
    float fuseSeconds = 0.0f;
    float bounciness = 0.0f;

    // Cluster grenades: how many bomblets fly out when it explodes.
    int bomblets = 0;

    // Mines: distance at which a hog sets it off.
    float triggerRadius = 0.0f;

    // How hard a hit throws a hog (world units per second at the centre).
    float knockback = 0.0f;

    WeaponEffect effect = WeaponEffect::None;
    // False for the few that leave the turn going on (healing hands, pick
    // pocket, a single mine: the wiki's notes on them).
    bool endsTurn = true;
    // Bullets: how far the shots of a burst swing round from first to last
    // (the flame thrower sweeps about 90 degrees).
    float sweepDegrees = 0.0f;

    // Shells: how they steer (missiles), and what they leave where they
    // burst (the mine shell lays a mine; `hasPayload` false: nothing).
    Guidance guidance = Guidance::None;
    bool hasPayload = false;
    // Aimed no lower than this many degrees above the horizon: the
    // executable keeps the mortar and the guided missile at 45 degrees or
    // more (FUN_0045c130) and marks the limit on the angle gauge.
    float lowestAimDegrees = -90.0f;
    // The bomblets' own blast, when it differs from the weapon's (the
    // anti-personnel mine's fragments: 20 points, very small); 0 = the same.
    float bombletDamage = 0.0f;
    float bombletRadius = 0.0f;
    WeaponId payload = WeaponId::Mine;

    // Guns the player may aim through the rifle sight: a first-person view
    // along the barrel, inside a round sight with a crosshair (the
    // executable's "rifle cam", FUN_00492820: weapons 6-15, 17 and 18,
    // that is PISTOL to ROCKET LAUNCHER, MEDICINE DART and TRANQUILISER).
    bool rifleView = false;

    // Presentation, as the original's weapon table (warhogs_.exe at
    // 0x4D7300, see docs/formats/weapons.md); animation numbers as in
    // scene::animationName:
    //   - drawAnimation: the hog takes the weapon out ("Getting out Rifle");
    //   - aimAnimation: the aiming pose, whose frames go from straight up
    //     (0) to straight down (the last), see aimPoseFrame(); kNoAnimation
    //     for weapons without one (the hog keeps the end of drawAnimation);
    //   - useAnimation: played when it is used (a punch, a throw);
    //     kNoAnimation for guns, which fire from the aiming pose;
    //   - model: the weapon in the hog's hand, in Chars/WEAPONS.MAD (empty:
    //     none), and the launch sound in the bank Audio/sfxday.srl.
    std::size_t drawAnimation = 0;
    std::size_t aimAnimation = 0;
    std::size_t useAnimation = 0;
    std::string_view launchSound;
    std::string_view model;
};

// "No animation" in WeaponSpec.
inline constexpr std::size_t kNoAnimation = static_cast<std::size_t>(-1);

// The frame of an aiming animation for an aim `degrees` above the horizon:
// the original's 65-frame aiming animations hold one pose per angle, from
// straight up (frame 0) through level (32) to straight down (64).
float aimPoseFrame(float degrees);

// The specification of a weapon.
const WeaponSpec& weaponSpec(WeaponId id);

// Every weapon OpenHogs implements.
std::span<const WeaponSpec> allWeapons();

// The weapons a hog of a class carries (class ids of
// scene::characterClasses, e.g. "grunt"); the fist for an unknown class.
std::span<const WeaponId> classLoadout(std::string_view classId);

} // namespace openhogs::game
