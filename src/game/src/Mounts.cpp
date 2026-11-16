// Mounts.cpp - see Mounts.h for an overview.

#include "openhogs/game/Mounts.h"

#include "openhogs/core/PathUtils.h"

#include <array>
#include <stdexcept>
#include <string>

namespace openhogs::game {

namespace {

using W = WeaponId;

struct NamedMount {
    std::string_view model;
    MountKind kind;
};

constexpr std::array<NamedMount, 9> kMountModels{{
    {"BIG_GUN", MountKind::Artillery}, {"PILLBOX", MountKind::Pillbox}, {"SHELTER", MountKind::Shelter},
    {"TENT_S", MountKind::Shelter},    {"M_TENT1", MountKind::MedicTent}, {"M_TENT2", MountKind::MashTent},
    {"TANK", MountKind::Tank},         {"AM_TANK", MountKind::Tank},      {"CARRY", MountKind::TroopCarrier},
}};

std::vector<MountSpec> makeSpecs() {
    std::vector<MountSpec> specs;
    // The wiki's loadouts: the artillery has three long range shells and
    // one of each other shell; the pillbox three bursts of the heavy
    // machine gun and three of the flame thrower; the tank a bazooka that
    // never runs out and one airburst. Tanks and carriers hurt the hog
    // inside by 40 when destroyed, buildings by 20.
    MountSpec artillery;
    artillery.kind = MountKind::Artillery;
    artillery.lowestAimDegrees = 0.0f;
    artillery.highestAimDegrees = 45.0f;
    artillery.arsenal = {{W::LongRangeShell, 3}, {W::MineShell, 1},         {W::GasShell, 1},
                         {W::FireRainShell, 1},  {W::ThousandPoundShell, 1}, {W::ShockShell, 1}};
    specs.push_back(artillery);

    MountSpec pillbox;
    pillbox.kind = MountKind::Pillbox;
    pillbox.lowestAimDegrees = -15.0f;
    pillbox.highestAimDegrees = 20.0f;
    pillbox.arsenal = {{W::HeavyMachineGun, 3}, {W::FlameThrower, 3}};
    specs.push_back(pillbox);

    for (const MountKind tent : {MountKind::Shelter, MountKind::MedicTent, MountKind::MashTent}) {
        MountSpec spec;
        spec.kind = tent;
        specs.push_back(spec);
    }

    MountSpec tank;
    tank.kind = MountKind::Tank;
    tank.vehicle = true;
    tank.drivable = true;
    tank.damageWhenDestroyed = 40.0f;
    tank.lowestAimDegrees = -10.0f;
    tank.highestAimDegrees = 45.0f;
    tank.arsenal = {{W::Bazooka, 0}, {W::Airburst, 1}};
    specs.push_back(tank);

    MountSpec carrier;
    carrier.kind = MountKind::TroopCarrier;
    carrier.vehicle = true;
    carrier.drivable = true;
    carrier.damageWhenDestroyed = 40.0f;
    specs.push_back(carrier);
    return specs;
}

} // namespace

std::optional<MountKind> mountKind(std::string_view modelName) {
    const std::string name = core::toUpper(modelName);
    for (const auto& entry : kMountModels) {
        if (entry.model == name) return entry.kind;
    }
    return std::nullopt;
}

const MountSpec& mountSpec(MountKind kind) {
    static const auto specs = makeSpecs();
    for (const auto& spec : specs) {
        if (spec.kind == kind) return spec;
    }
    throw std::out_of_range("Unknown mount kind");
}

std::vector<std::pair<std::size_t, MountKind>> findMounts(const std::vector<formats::MapObject>& objects) {
    std::vector<std::pair<std::size_t, MountKind>> mounts;
    for (std::size_t i = 0; i < objects.size(); ++i) {
        if (const auto kind = mountKind(objects[i].modelName)) mounts.emplace_back(i, *kind);
    }
    return mounts;
}

} // namespace openhogs::game
