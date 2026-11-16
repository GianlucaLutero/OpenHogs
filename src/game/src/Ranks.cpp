// Ranks.cpp - see Ranks.h for an overview.

#include "openhogs/game/Ranks.h"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace openhogs::game {

namespace {

using W = WeaponId;
using R = RankId;
using C = Career;

// Health, weapons and uses as listed by the Hogs of War Wiki's rank
// pages (they agree with the manual for the Grunt and the first career
// ranks). Body
// models: the PC files have one model per career line, not per rank; the
// Commando wears the saboteur's (as do the maps' commandos) and the Hero
// the legend's (docs/open-decisions.md).
const std::array<RankSpec, 15> kRanks{{
    {R::Grunt, "grunt", 63, "GRUNT", C::None, 0, 50, "grunt", {W::Bayonet, W::Rifle, W::Grenade}, {{W::Grenade, 3}}},
    {R::Gunner, "gunner", 64, "GUNNER", C::HeavyWeapons, 1, 75, "heavy", {W::Trotter, W::Pistol, W::Bazooka}, {}},
    {R::Bombardier, "bombardier", 65, "BOMBARDIER", C::HeavyWeapons, 2, 90, "heavy",
     {W::Trotter, W::Pistol, W::Bazooka, W::Mortar}, {{W::Mortar, 3}}},
    {R::Pyrotechnic, "pyrotechnic", 66, "PYROTECHNIC", C::HeavyWeapons, 3, 120, "heavy",
     {W::Trotter, W::Pistol, W::Bazooka, W::Mortar, W::FlameThrower, W::Airburst},
     {{W::Mortar, 5}, {W::FlameThrower, 3}, {W::Airburst, 1}}},
    {R::Orderly, "orderly", 74, "ORDERLY", C::Medic, 1, 60, "medic", {W::Knife, W::Rifle, W::Grenade, W::HealingHands},
     {{W::Grenade, 3}, {W::HealingHands, 3}}},
    {R::Medic, "medic", 75, "MEDIC", C::Medic, 2, 80, "medic",
     {W::Knife, W::Rifle, W::Grenade, W::HealingHands, W::MedicineDart, W::MedicineBall},
     {{W::Grenade, 3}, {W::HealingHands, 3}, {W::MedicineDart, 3}, {W::MedicineBall, 3}}},
    {R::Surgeon, "surgeon", 76, "SURGEON", C::Medic, 3, 120, "medic",
     {W::Knife, W::Rifle, W::RifleBurst, W::Grenade, W::HealingHands, W::MedicineDart, W::MedicineBall, W::Tranquiliser},
     {{W::RifleBurst, 3}, {W::Grenade, 3}, {W::HealingHands, 3}, {W::MedicineDart, 3}, {W::MedicineBall, 3},
      {W::Tranquiliser, 1}}},
    {R::Scout, "scout", 71, "SCOUT", C::Espionage, 1, 75, "sniper", {W::Knife, W::Rifle, W::PoisonGas, W::PickPocket, W::Hide},
     {{W::PoisonGas, 1}, {W::PickPocket, 1}}},
    {R::Sniper, "sniper", 72, "SNIPER", C::Espionage, 2, 90, "sniper",
     {W::Knife, W::SniperRifle, W::PoisonGas, W::Suicide, W::PickPocket, W::Hide},
     {{W::PoisonGas, 1}, {W::Suicide, 1}, {W::PickPocket, 2}}},
    {R::Spy, "spy", 73, "SPY", C::Espionage, 3, 120, "spy",
     {W::Knife, W::CattleProd, W::SniperRifle, W::PoisonGas, W::Tnt, W::PickPocket, W::Hide},
     {{W::CattleProd, 3}, {W::PoisonGas, 1}, {W::Tnt, 1}, {W::PickPocket, 3}}},
    {R::Sapper, "sapper", 68, "SAPPER", C::Engineer, 1, 80, "sapper", {W::Trotter, W::Shotgun, W::Mine, W::Tnt},
     {{W::Mine, 3}, {W::Tnt, 1}}},
    {R::Engineer, "engineer", 69, "ENGINEER", C::Engineer, 2, 100, "sapper",
     {W::Trotter, W::Shotgun, W::Mine, W::Tnt, W::ShrapnelGrenade, W::Suicide},
     {{W::Mine, 3}, {W::Tnt, 2}, {W::ShrapnelGrenade, 3}, {W::Suicide, 1}}},
    {R::Saboteur, "saboteur", 70, "SABOTEUR", C::Engineer, 3, 120, "saboteur",
     {W::Trotter, W::SuperShotgun, W::Mine, W::Tnt, W::ShrapnelGrenade, W::Suicide},
     {{W::Mine, 3}, {W::Tnt, 3}, {W::ShrapnelGrenade, 3}, {W::Suicide, 1}}},
    {R::Commando, "commando", 67, "COMMANDO", C::Officer, 4, 130, "saboteur",
     {W::Knife, W::MachineGun, W::SniperRifle, W::MedicineDart, W::ClusterGrenade, W::PoisonGas, W::Bazooka, W::Airburst,
      W::Tnt, W::Jetpack, W::Hide},
     {{W::MachineGun, 1}, {W::MedicineDart, 3}, {W::ClusterGrenade, 1}, {W::PoisonGas, 1}, {W::Airburst, 1}, {W::Tnt, 1},
      {W::Jetpack, 1}}},
    {R::Hero, "hero", 77, "HERO", C::Officer, 5, 150, "legend",
     {W::Sword, W::HeavyMachineGun, W::SniperRifle, W::MedicineDart, W::ClusterGrenade, W::PoisonGas, W::Bazooka,
      W::Airburst, W::Tnt, W::Jetpack, W::SelfHeal, W::Hide, W::SpecialOps, W::Airstrike},
     {{W::HeavyMachineGun, 1}, {W::MedicineDart, 3}, {W::ClusterGrenade, 1}, {W::PoisonGas, 1}, {W::Airburst, 1},
      {W::Tnt, 1}, {W::Jetpack, 1}, {W::SelfHeal, 1}, {W::SpecialOps, 1}, {W::Airstrike, 1}}},
}};

} // namespace

const RankSpec& rankSpec(RankId rank) {
    const auto found = std::find_if(kRanks.begin(), kRanks.end(), [rank](const RankSpec& spec) { return spec.id == rank; });
    if (found == kRanks.end()) {
        throw std::out_of_range("Unknown rank");
    }
    return *found;
}

std::span<const RankSpec> allRanks() {
    return kRanks;
}

std::optional<RankId> findRank(std::string_view key) {
    for (const auto& spec : kRanks) {
        if (spec.key == key) return spec.id;
    }
    return std::nullopt;
}

std::vector<RankId> promotionsFrom(RankId rank) {
    const RankSpec& from = rankSpec(rank);
    switch (from.career) {
    case Career::None:
        return {R::Gunner, R::Orderly, R::Scout, R::Sapper};
    case Career::Officer:
        return rank == R::Commando ? std::vector<RankId>{R::Hero} : std::vector<RankId>{};
    default:
        break;
    }
    if (from.level == 3) {
        return {R::Commando};
    }
    // The next rank of the same career.
    for (const auto& spec : kRanks) {
        if (spec.career == from.career && spec.level == from.level + 1) {
            return {spec.id};
        }
    }
    return {};
}

int promotionCost(RankId rank) {
    return rankSpec(rank).level;
}

} // namespace openhogs::game
