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

// Health and weapons as listed by the manual (Grunt and first career
// ranks) and the player guides (the others), with the weapons OpenHogs
// does not have yet left out. Body models: the PC files have one model per
// career line, not per rank; the Commando wears the saboteur's (as do the
// maps' commandos) and the Hero the legend's (docs/open-decisions.md).
const std::array<RankSpec, 15> kRanks{{
    {R::Grunt, "grunt", 63, "GRUNT", C::None, 0, 50, "grunt", {W::Bayonet, W::Rifle, W::Grenade}},
    {R::Gunner, "gunner", 64, "GUNNER", C::HeavyWeapons, 1, 75, "heavy", {W::Trotter, W::Pistol, W::Bazooka}},
    {R::Bombardier, "bombardier", 65, "BOMBARDIER", C::HeavyWeapons, 2, 90, "heavy",
     {W::Trotter, W::Pistol, W::Mortar, W::Bazooka}},
    {R::Pyrotechnic, "pyrotechnic", 66, "PYROTECHNIC", C::HeavyWeapons, 3, 120, "heavy",
     {W::Trotter, W::Pistol, W::Mortar, W::Bazooka}},
    {R::Orderly, "orderly", 74, "ORDERLY", C::Medic, 1, 60, "medic", {W::Knife, W::Rifle, W::Grenade}},
    {R::Medic, "medic", 75, "MEDIC", C::Medic, 2, 80, "medic", {W::Knife, W::Rifle, W::Grenade}},
    {R::Surgeon, "surgeon", 76, "SURGEON", C::Medic, 3, 120, "medic",
     {W::Knife, W::Rifle, W::RifleBurst, W::Grenade}},
    {R::Scout, "scout", 71, "SCOUT", C::Espionage, 1, 75, "sniper", {W::Knife, W::Rifle}},
    {R::Sniper, "sniper", 72, "SNIPER", C::Espionage, 2, 90, "sniper", {W::Knife, W::SniperRifle}},
    {R::Spy, "spy", 73, "SPY", C::Espionage, 3, 120, "spy", {W::Knife, W::SniperRifle, W::Tnt}},
    {R::Sapper, "sapper", 68, "SAPPER", C::Engineer, 1, 80, "sapper", {W::Trotter, W::Shotgun, W::Mine, W::Tnt}},
    {R::Engineer, "engineer", 69, "ENGINEER", C::Engineer, 2, 100, "sapper",
     {W::Trotter, W::Shotgun, W::HighExplosiveGrenade, W::Mine, W::Tnt}},
    {R::Saboteur, "saboteur", 70, "SABOTEUR", C::Engineer, 3, 120, "saboteur",
     {W::Trotter, W::Shotgun, W::HighExplosiveGrenade, W::Mine, W::Tnt}},
    {R::Commando, "commando", 67, "COMMANDO", C::Officer, 4, 130, "saboteur",
     {W::Knife, W::MachineGun, W::SniperRifle, W::ClusterGrenade, W::Bazooka, W::Tnt}},
    {R::Hero, "hero", 77, "HERO", C::Officer, 5, 150, "legend",
     {W::Sword, W::HeavyMachineGun, W::SniperRifle, W::ClusterGrenade, W::Bazooka, W::Tnt}},
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
