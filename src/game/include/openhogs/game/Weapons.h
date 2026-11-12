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
    Pistol = 102,
    Rifle = 103,
    RifleBurst = 104,
    MachineGun = 105,
    HeavyMachineGun = 106,
    SniperRifle = 107,
    Shotgun = 108,
    RocketLauncher = 111,
    Grenade = 115,
    ClusterGrenade = 116,
    HighExplosiveGrenade = 117,
    Mortar = 124,
    Bazooka = 125,
    GrenadeLauncher = 126,
    Mine = 131,
    Tnt = 133,
};

enum class WeaponKind { Melee, Bullet, Thrown, Shell, Placed };

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
