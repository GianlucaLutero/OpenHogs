// Weapons.cpp - see Weapons.h for an overview.

#include "openhogs/game/Weapons.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>

namespace openhogs::game {

namespace {

// Animation numbers (scene::animationName).
constexpr std::size_t kUsingGrenade = 19;
constexpr std::size_t kSwordKnife = 21;
constexpr std::size_t kBayonet = 22;
constexpr std::size_t kAimingHandgun = 23;
constexpr std::size_t kAimingRifle = 24;
constexpr std::size_t kAimingMachineGun = 25;
constexpr std::size_t kAimingHeavyWeapon = 26;

WeaponSpec melee(WeaponId id, std::string_view name, float damage, float range, std::string_view sound) {
    WeaponSpec spec;
    spec.id = id;
    spec.kind = WeaponKind::Melee;
    spec.fallbackName = name;
    spec.damage = damage;
    spec.range = range;
    spec.knockback = 450.0f;
    spec.launchSound = sound;
    return spec;
}

WeaponSpec gun(WeaponId id, std::string_view name, float damage, float range, int shots, float spread,
               std::string_view sound) {
    WeaponSpec spec;
    spec.id = id;
    spec.kind = WeaponKind::Bullet;
    spec.fallbackName = name;
    spec.damage = damage;
    spec.range = range;
    spec.shots = shots;
    spec.secondsBetweenShots = 0.1f;
    spec.spreadDegrees = spread;
    spec.knockback = 120.0f;
    spec.launchSound = sound;
    return spec;
}

WeaponSpec explosive(WeaponId id, WeaponKind kind, std::string_view name, float damage, float radius,
                     float speed, std::string_view sound) {
    WeaponSpec spec;
    spec.id = id;
    spec.kind = kind;
    spec.fallbackName = name;
    spec.damage = damage;
    spec.blastRadius = radius;
    spec.launchSpeed = speed;
    spec.usesPower = kind == WeaponKind::Thrown || kind == WeaponKind::Shell;
    spec.knockback = 900.0f;
    spec.launchSound = sound;
    return spec;
}

// The presentation of each weapon in the original's weapon table
// (warhogs_.exe, 80-byte rows at 0x4D7300, one per text entry from 96):
// the model in the hand (row +0x0C, 1-based in WEAPONS.MAD), the
// animations to take it out (+0x54), to aim (+0x50) and to use it (+0x20).
struct Presentation {
    WeaponId id;
    std::string_view model;
    std::size_t draw;
    std::size_t aim;
    std::size_t use;
};

constexpr std::size_t kNone = kNoAnimation;
// "Getting out" animations (scene::animationName 12-18).
constexpr std::size_t kDrawHandgun = 12, kDrawRifle = 13, kDrawMachineGun = 14, kDrawHeavy = 15,
                      kDrawGrenade = 17, kDrawBlade = 18;
// Putting a mine or TNT down (animation 77, unnamed in the lists).
constexpr std::size_t kPlaceExplosive = 77;

constexpr std::array<Presentation, 20> kPresentation{{
    {WeaponId::Trotter, "", kDrawGrenade, kNone, kSwordKnife},
    {WeaponId::Knife, "WE_KNIFE", kDrawBlade, kNone, kSwordKnife},
    {WeaponId::Bayonet, "WE_RIF", kDrawMachineGun, kAimingMachineGun, kBayonet},
    {WeaponId::Sword, "WE_SWORD", kDrawBlade, kNone, kSwordKnife},
    {WeaponId::Pistol, "WE_PIS", kDrawHandgun, kAimingHandgun, kNone},
    {WeaponId::Rifle, "WE_RIF", kDrawRifle, kAimingRifle, kNone},
    {WeaponId::RifleBurst, "WE_RIF", kDrawRifle, kAimingRifle, kNone},
    {WeaponId::MachineGun, "WE_MGUN", kDrawMachineGun, kAimingMachineGun, kNone},
    {WeaponId::HeavyMachineGun, "WE_LEWIS", kDrawMachineGun, kAimingMachineGun, kNone},
    {WeaponId::SniperRifle, "WE_TELR", kDrawRifle, kAimingRifle, kNone},
    {WeaponId::Shotgun, "WE_BLUND", kDrawRifle, kAimingRifle, kNone},
    {WeaponId::RocketLauncher, "rocklau", kDrawHeavy, kAimingHeavyWeapon, kNone},
    {WeaponId::Grenade, "WE_GREN", kDrawGrenade, kNone, kUsingGrenade},
    {WeaponId::ClusterGrenade, "WE_GREN", kDrawGrenade, kNone, kUsingGrenade},
    {WeaponId::HighExplosiveGrenade, "WE_GREN", kDrawGrenade, kNone, kUsingGrenade},
    {WeaponId::Mortar, "mortr", kDrawHeavy, kAimingHeavyWeapon, kNone},
    {WeaponId::Bazooka, "bazookr", kDrawHeavy, kAimingHeavyWeapon, kNone},
    {WeaponId::GrenadeLauncher, "grenlau", kDrawHeavy, kAimingHeavyWeapon, kNone},
    {WeaponId::Mine, "WE_MINE", kDrawGrenade, kNone, kPlaceExplosive},
    {WeaponId::Tnt, "WE_TNT", kDrawGrenade, kNone, kPlaceExplosive},
}};

// The original's presentation over the rules of `spec`.
void present(WeaponSpec& spec) {
    for (const auto& row : kPresentation) {
        if (row.id != spec.id) continue;
        spec.model = row.model;
        spec.drawAnimation = row.draw;
        spec.aimAnimation = row.aim;
        spec.useAnimation = row.use;
        return;
    }
}

std::array<WeaponSpec, 20> makeWeapons() {
    using W = WeaponId;
    using K = WeaponKind;

    auto grenade = explosive(W::Grenade, K::Thrown, "GRENADE", 40, 400, 1600, "");
    grenade.fuseSeconds = 3.0f;
    grenade.bounciness = 0.4f;

    auto cluster = grenade;
    cluster.id = W::ClusterGrenade;
    cluster.fallbackName = "CLUSTER GRENADE";
    cluster.damage = 25;
    cluster.blastRadius = 300;
    cluster.bomblets = 5;

    auto highExplosive = grenade;
    highExplosive.id = W::HighExplosiveGrenade;
    highExplosive.fallbackName = "H-EXPL. GRENADE";
    highExplosive.damage = 60;
    highExplosive.blastRadius = 550;

    auto launcher = grenade;
    launcher.id = W::GrenadeLauncher;
    launcher.fallbackName = "GRENADE LAUNCHER";
    launcher.launchSpeed = 2600;
    launcher.fuseSeconds = 2.5f;
    launcher.launchSound = "L_MORT";

    auto mortar = explosive(W::Mortar, K::Shell, "MORTAR", 50, 500, 2400, "L_MORT");
    auto bazooka = explosive(W::Bazooka, K::Shell, "BAZOOKA", 55, 450, 2800, "L_BAZOO");
    bazooka.gravityFactor = 0.3f;  // a rocket: flies almost straight
    auto rocket = explosive(W::RocketLauncher, K::Shell, "ROCKET LAUNCHER", 45, 400, 3200, "L_ROC");
    rocket.gravityFactor = 0.0f;
    rocket.usesPower = false;

    auto tnt = explosive(W::Tnt, K::Placed, "TNT", 70, 600, 0, "");
    tnt.fuseSeconds = 4.0f;
    auto mine = explosive(W::Mine, K::Placed, "MINE", 50, 400, 0, "L_MINETR");
    mine.triggerRadius = 150.0f;

    auto burst = gun(W::RifleBurst, "RIFLE BURST", 15, 6000, 3, 1.5f, "L_RIFLE");
    auto shotgun = gun(W::Shotgun, "SHOTGUN", 6, 2000, 1, 8.0f, "L_SHOTG");
    shotgun.pellets = 8;
    shotgun.knockback = 60.0f;  // per pellet

    std::array<WeaponSpec, 20> table{{
        melee(W::Trotter, "TROTTER", 15, 160, "P_PUNCH"),
        melee(W::Knife, "KNIFE", 25, 170, "I_STAB"),
        melee(W::Bayonet, "BAYONET", 30, 220, "I_STAB"),
        melee(W::Sword, "SWORD", 40, 240, "I_SWORD"),
        gun(W::Pistol, "PISTOL", 15, 4000, 1, 1.0f, "L_PISTOL"),
        gun(W::Rifle, "RIFLE", 25, 6000, 1, 0.5f, "L_RIFLE"),
        burst,
        gun(W::MachineGun, "MACHINE GUN", 8, 5000, 8, 3.0f, "L_MGUN"),
        gun(W::HeavyMachineGun, "HEAVY M-GUN", 12, 6000, 10, 4.0f, "L_HVYMG"),
        gun(W::SniperRifle, "SNIPER RIFLE", 50, 12000, 1, 0.0f, "L_RIFLE"),
        shotgun,
        rocket,
        grenade,
        cluster,
        highExplosive,
        mortar,
        bazooka,
        launcher,
        mine,
        tnt,
    }};
    for (auto& spec : table) present(spec);
    return table;
}

const std::array<WeaponSpec, 20>& weapons() {
    static const auto table = makeWeapons();
    return table;
}

struct Loadout {
    std::string_view classId;
    std::array<WeaponId, 6> weapons;
    std::size_t count;
};

using W = WeaponId;
constexpr std::array<Loadout, 9> kLoadouts{{
    {"grunt", {W::Trotter, W::Knife, W::Pistol, W::Rifle, W::Grenade}, 5},
    {"heavy", {W::Trotter, W::MachineGun, W::HeavyMachineGun, W::Bazooka, W::Mortar, W::Grenade}, 6},
    {"sniper", {W::Trotter, W::Knife, W::Rifle, W::SniperRifle, W::Grenade}, 5},
    {"sapper", {W::Trotter, W::Pistol, W::Tnt, W::Mine, W::Grenade}, 5},
    {"saboteur", {W::Knife, W::Pistol, W::RifleBurst, W::ClusterGrenade, W::Tnt}, 5},
    {"spy", {W::Knife, W::Pistol, W::Shotgun, W::HighExplosiveGrenade}, 4},
    {"medic", {W::Trotter, W::Pistol, W::Grenade}, 3},
    {"ace", {W::Sword, W::Pistol, W::MachineGun, W::GrenadeLauncher, W::RocketLauncher}, 5},
    {"legend", {W::Sword, W::RifleBurst, W::HeavyMachineGun, W::HighExplosiveGrenade, W::Bazooka, W::Mortar}, 6},
}};

constexpr std::array<WeaponId, 1> kFistOnly{WeaponId::Trotter};

} // namespace

float aimPoseFrame(float degrees) {
    constexpr float kLastAimFrame = 64.0f;
    const float clamped = std::clamp(degrees, -90.0f, 90.0f);
    return (90.0f - clamped) / 180.0f * kLastAimFrame;
}

const WeaponSpec& weaponSpec(WeaponId id) {
    for (const auto& spec : weapons()) {
        if (spec.id == id) {
            return spec;
        }
    }
    throw std::invalid_argument("Unknown weapon " + std::to_string(static_cast<int>(id)));
}

std::span<const WeaponSpec> allWeapons() {
    return weapons();
}

std::span<const WeaponId> classLoadout(std::string_view classId) {
    for (const auto& loadout : kLoadouts) {
        if (loadout.classId == classId) {
            return std::span<const WeaponId>(loadout.weapons.data(), loadout.count);
        }
    }
    return kFistOnly;
}

} // namespace openhogs::game
