// Tests for the weapons added by block weapons-complete (part 1): healing,
// suicide, skipping a turn, surrendering and the new explosives. They
// protect the turn-based balance: a heal must not raise a hog above its
// full health, surrender must end the match for the team that gives up,
// and the new crate weapons must exist so crates can hand them out.

#include "GameTestSupport.h"

#include "openhogs/game/Pickups.h"
#include "openhogs/game/World.h"

#include <glm/gtc/constants.hpp>

using namespace openhogs;
using namespace openhogs::tests;

namespace {

std::vector<GameEvent> fireOnce(World& world) {
    static const auto animations = makeAnimations();
    PlayerInput fire;
    fire.fire = true;
    std::vector<GameEvent> events = world.update(kStep, fire, animations);
    for (int i = 0; i < 50 * 6; ++i) {
        const auto step = world.update(kStep, PlayerInput{}, animations);
        events.insert(events.end(), step.begin(), step.end());
    }
    return events;
}

std::vector<Hog> teams(const MapCollision& map, WeaponId weapon) {
    std::vector<Hog> hogs{makeHog(map, 0, 0, 0), makeHog(map, 200, 0, 0), makeHog(map, 3000, 0, 1)};
    hogs[0].weapons = {weapon};
    return hogs;
}

} // namespace

TEST_CASE("Healing hands heal the nearest friend; self heal the hog itself, never above full health",
          "[game][weapons]") {
    const auto map = flatMap();
    auto hogs = teams(map, WeaponId::HealingHands);
    hogs[1].health = 30.0f;
    World hands(map, hogs);
    fireOnce(hands);
    CHECK(hands.hogs()[1].health == Approx(30.0f + weaponSpec(WeaponId::HealingHands).damage));

    hogs = teams(map, WeaponId::SelfHeal);
    hogs[0].health = 90.0f;
    World self(map, hogs);
    fireOnce(self);
    CHECK(self.hogs()[0].health == Approx(hogs[0].maxHealth));
}

TEST_CASE("Suicide takes the hog and its neighbours; surrender ends the match", "[game][weapons]") {
    // The friend standing 200 units away is caught in the blast; the enemy
    // far away is not. A surrendering team loses all its hogs at once, so
    // the other team wins.
    const auto map = flatMap();
    World suicide(map, teams(map, WeaponId::Suicide));
    fireOnce(suicide);
    CHECK_FALSE(suicide.hogs()[0].alive);
    CHECK(suicide.hogs()[1].health < kMaxHealth);
    CHECK(suicide.hogs()[2].health == Approx(kMaxHealth));

    World surrender(map, teams(map, WeaponId::Surrender));
    fireOnce(surrender);
    CHECK_FALSE(surrender.hogs()[0].alive);
    CHECK_FALSE(surrender.hogs()[1].alive);
    CHECK(surrender.phase() == MatchPhase::Over);
    CHECK(surrender.winner() == 1);
}

TEST_CASE("The new weapons come out of crates", "[game][weapons]") {
    // Crate items go through the executable's table to these weapons, so
    // the maps' crates now hand them out (before, they gave nothing).
    for (const WeaponId weapon : {WeaponId::SuperShotgun, WeaponId::ShrapnelGrenade, WeaponId::SuperTnt,
                                  WeaponId::HealingHands, WeaponId::Airburst}) {
        Pickup crate;
        crate.kind = PickupKind::Weapon;
        crate.item = static_cast<int>(weapon) - 96;
        CHECK(crate.weapon() == weapon);
    }
    CHECK(weaponSpec(WeaponId::SuperTnt).damage == Approx(200.0f));  // ammunition 440: 25600 / 128
}
