// Weapons.cpp - see Weapons.h for an overview.

#include "openhogs/game/Weapons.h"

#include <array>
#include <stdexcept>
#include <string>

namespace openhogs::game {

namespace {

// Animation numbers (scene::animationName).
constexpr std::size_t kUsingGrenade = 19;
constexpr std::size_t kUsingPunch = 20;
constexpr std::size_t kSwordKnife = 21;
constexpr std::size_t kBayonet = 22;
constexpr std::size_t kAimingHandgun = 23;
constexpr std::size_t kAimingRifle = 24;
constexpr std::size_t kAimingMachineGun = 25;
constexpr std::size_t kAimingHeavyWeapon = 26;
constexpr std::size_t kLayMine = 53;
// Weapons with no aiming pose (fists, blades, grenades, explosives to put
// down) keep the normal standing animation while the player aims.
constexpr std::size_t kStanding = 27;

WeaponSpec melee(WeaponId id, std::string_view name, float damage, float range, std::size_t animation,
                 std::string_view sound, std::string_view model) {
    WeaponSpec spec;
    spec.id = id;
    spec.kind = WeaponKind::Melee;
    spec.fallbackName = name;
    spec.damage = damage;
    spec.range = range;
    spec.knockback = 450.0f;
    spec.aimAnimation = kStanding;
    spec.useAnimation = animation;
    spec.launchSound = sound;
    spec.model = model;
    return spec;
}

WeaponSpec gun(WeaponId id, std::string_view name, float damage, float range, int shots, float spread,
               std::size_t aimAnimation, std::string_view sound, std::string_view model) {
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
    spec.aimAnimation = aimAnimation;
    spec.useAnimation = aimAnimation;
    spec.launchSound = sound;
    spec.model = model;
    return spec;
}

WeaponSpec explosive(WeaponId id, WeaponKind kind, std::string_view name, float damage, float radius,
                     float speed, std::size_t aimAnimation, std::size_t useAnimation, std::string_view sound,
                     std::string_view model) {
    WeaponSpec spec;
    spec.id = id;
    spec.kind = kind;
    spec.fallbackName = name;
    spec.damage = damage;
    spec.blastRadius = radius;
    spec.launchSpeed = speed;
    spec.usesPower = kind == WeaponKind::Thrown || kind == WeaponKind::Shell;
    spec.knockback = 900.0f;
    spec.aimAnimation = aimAnimation;
    spec.useAnimation = useAnimation;
    spec.launchSound = sound;
    spec.model = model;
    return spec;
}

std::array<WeaponSpec, 20> makeWeapons() {
    using W = WeaponId;
    using K = WeaponKind;

    auto grenade = explosive(W::Grenade, K::Thrown, "GRENADE", 40, 400, 1600, kStanding, kUsingGrenade, "", "WE_GREN");
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
    highExplosive.model = "WE_HEG";

    auto launcher = grenade;
    launcher.id = W::GrenadeLauncher;
    launcher.fallbackName = "GRENADE LAUNCHER";
    launcher.launchSpeed = 2600;
    launcher.fuseSeconds = 2.5f;
    launcher.aimAnimation = kAimingHeavyWeapon;
    launcher.useAnimation = kAimingHeavyWeapon;
    launcher.launchSound = "L_MORT";
    launcher.model = "grenlau";

    auto mortar = explosive(W::Mortar, K::Shell, "MORTAR", 50, 500, 2400, kAimingHeavyWeapon, kAimingHeavyWeapon, "L_MORT", "mortr");
    auto bazooka = explosive(W::Bazooka, K::Shell, "BAZOOKA", 55, 450, 2800, kAimingHeavyWeapon, kAimingHeavyWeapon, "L_BAZOO", "bazookr");
    bazooka.gravityFactor = 0.3f;  // a rocket: flies almost straight
    auto rocket = explosive(W::RocketLauncher, K::Shell, "ROCKET LAUNCHER", 45, 400, 3200, kAimingHeavyWeapon, kAimingHeavyWeapon, "L_ROC", "rocklau");
    rocket.gravityFactor = 0.0f;
    rocket.usesPower = false;

    auto tnt = explosive(W::Tnt, K::Placed, "TNT", 70, 600, 0, kStanding, kLayMine, "", "WE_TNT");
    tnt.fuseSeconds = 4.0f;
    auto mine = explosive(W::Mine, K::Placed, "MINE", 50, 400, 0, kStanding, kLayMine, "L_MINETR", "WE_MINE");
    mine.triggerRadius = 150.0f;

    auto burst = gun(W::RifleBurst, "RIFLE BURST", 15, 6000, 3, 1.5f, kAimingRifle, "L_RIFLE", "WE_RIF");
    auto shotgun = gun(W::Shotgun, "SHOTGUN", 6, 2000, 1, 8.0f, kAimingRifle, "L_SHOTG", "WE_RIF");
    shotgun.pellets = 8;
    shotgun.knockback = 60.0f;  // per pellet

    return {{
        melee(W::Trotter, "TROTTER", 15, 160, kUsingPunch, "P_PUNCH", ""),
        melee(W::Knife, "KNIFE", 25, 170, kSwordKnife, "I_STAB", "WE_KNIFE"),
        melee(W::Bayonet, "BAYONET", 30, 220, kBayonet, "I_STAB", "WE_RIF"),
        melee(W::Sword, "SWORD", 40, 240, kSwordKnife, "I_SWORD", "WE_SWORD"),
        gun(W::Pistol, "PISTOL", 15, 4000, 1, 1.0f, kAimingHandgun, "L_PISTOL", "WE_PIS"),
        gun(W::Rifle, "RIFLE", 25, 6000, 1, 0.5f, kAimingRifle, "L_RIFLE", "WE_RIF"),
        burst,
        gun(W::MachineGun, "MACHINE GUN", 8, 5000, 8, 3.0f, kAimingMachineGun, "L_MGUN", "WE_MGUN"),
        gun(W::HeavyMachineGun, "HEAVY M-GUN", 12, 6000, 10, 4.0f, kAimingHeavyWeapon, "L_HVYMG", "WE_LEWIS"),
        gun(W::SniperRifle, "SNIPER RIFLE", 50, 12000, 1, 0.0f, kAimingRifle, "L_RIFLE", "WE_RIF"),
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
