// Tests for the weapons added by block weapons-complete (part 2): gases
// and darts with lasting effects, the flame thrower's sweep, hiding, pick
// pocketing and weapons that run out. They protect the rules the Hogs of
// War Wiki gives for them, which change who may act on a turn and how
// long a hog survives, so a regression would quietly unbalance matches.

#include "GameTestSupport.h"

#include "openhogs/game/World.h"

#include <glm/gtc/constants.hpp>

#include <algorithm>

using namespace openhogs;
using namespace openhogs::tests;

namespace {

std::vector<GameEvent> run(World& world, bool fire, int steps) {
    static const auto animations = makeAnimations();
    PlayerInput input;
    input.fire = fire;
    std::vector<GameEvent> all;
    for (int i = 0; i < steps; ++i) {
        const auto step = world.update(kStep, input, animations);
        all.insert(all.end(), step.begin(), step.end());
    }
    return all;
}

// Fire once, then let the shot and the end of the turn play out.
std::vector<GameEvent> fireAndWait(World& world) {
    auto events = run(world, true, 1);
    const auto later = run(world, false, 50 * 4);
    events.insert(events.end(), later.begin(), later.end());
    return events;
}

int count(const std::vector<GameEvent>& events, GameEventType type) {
    return static_cast<int>(std::count_if(events.begin(), events.end(), [type](const GameEvent& e) { return e.type == type; }));
}

// A shooter at the origin facing +x, and a target `distance` ahead.
std::vector<Hog> facing(const MapCollision& map, WeaponId weapon, float distance, int targetTeam = 1) {
    std::vector<Hog> hogs{makeHog(map, 0, 0, 0), makeHog(map, distance, 0, targetTeam)};
    if (targetTeam == 0) hogs.push_back(makeHog(map, 3000, 3000, 1));  // someone to play against
    hogs[1].yawRadians = glm::pi<float>();
    hogs[0].weapons = {weapon};
    hogs[1].weapons = {WeaponId::Rifle};
    for (auto& hog : hogs) hog.aimDegrees = 0.0f;
    return hogs;
}

bool carries(const Hog& hog, WeaponId weapon) {
    return std::find(hog.weapons.begin(), hog.weapons.end(), weapon) != hog.weapons.end();
}

} // namespace

TEST_CASE("A tranquilised hog misses its next turn", "[game][weapons][effects]") {
    // The dart puts the target to sleep: when its turn comes it passes at
    // once, and the hog wakes up for the turn after.
    const auto map = flatMap();
    World world(map, facing(map, WeaponId::Tranquiliser, 400));
    const auto events = fireAndWait(world);
    CHECK(world.hogs()[1].health < kMaxHealth);
    CHECK(count(events, GameEventType::TurnSlept) == 1);
    CHECK_FALSE(world.hogs()[1].asleep);
    CHECK(world.activeHog() == std::optional<std::size_t>(0));  // the turn came straight back
}

TEST_CASE("Poison hurts at each of the hog's turns until it is healed", "[game][weapons][effects]") {
    // The wiki: 10 points at the start of each of its turns; any healing
    // cures it, so a medic can save a poisoned friend.
    const auto map = flatMap();
    auto hogs = facing(map, WeaponId::SkipTurn, 400);
    hogs[1].poisoned = true;
    World poisoned(map, hogs);
    fireAndWait(poisoned);  // hog 0 passes; hog 1's turn begins
    CHECK(poisoned.hogs()[1].health == Approx(kMaxHealth - 10.0f));

    hogs = facing(map, WeaponId::MedicineDart, 400, 0);
    hogs[1].poisoned = true;
    hogs[1].health = 50.0f;
    World cured(map, hogs);
    fireAndWait(cured);
    CHECK_FALSE(cured.hogs()[1].poisoned);
    CHECK(cured.hogs()[1].health == Approx(50.0f + weaponSpec(WeaponId::MedicineDart).damage));
}

TEST_CASE("The flame thrower sweeps: only part of its flames reach one target", "[game][weapons][effects]") {
    // All its flames go out, across about 90 degrees, so a hog straight
    // ahead is caught by a few of them, not by the whole burst.
    const auto map = flatMap();
    World world(map, facing(map, WeaponId::FlameThrower, 300));
    const auto events = fireAndWait(world);
    const WeaponSpec& flame = weaponSpec(WeaponId::FlameThrower);
    CHECK(count(events, GameEventType::Fired) == flame.shots);
    const float lost = kMaxHealth - world.hogs()[1].health;
    CHECK(lost > 0.0f);
    CHECK(lost < flame.damage * static_cast<float>(flame.shots) / 2.0f);
}

TEST_CASE("Weapons with a number of uses run out", "[game][weapons][effects]") {
    // A rank's limited weapons (three grenades for a grunt...) disappear
    // once used up; its other weapons stay.
    const auto map = flatMap();
    auto hogs = facing(map, WeaponId::Tranquiliser, 400);
    hogs[0].weapons = {WeaponId::Tranquiliser, WeaponId::Rifle};
    hogs[0].ownAmmo = {{WeaponId::Tranquiliser, 1}};
    World world(map, hogs);
    fireAndWait(world);
    CHECK_FALSE(carries(world.hogs()[0], WeaponId::Tranquiliser));
    CHECK(carries(world.hogs()[0], WeaponId::Rifle));
    CHECK(world.hogs()[0].selectedWeapon() == WeaponId::Rifle);
}

TEST_CASE("Pick pocket steals one use of an enemy's weapon and the turn goes on", "[game][weapons][effects]") {
    // The victim's only mine passes to the thief, who still has its turn.
    const auto map = flatMap();
    auto hogs = facing(map, WeaponId::PickPocket, 100);
    hogs[1].weapons = {WeaponId::Trotter, WeaponId::Mine};
    hogs[1].ownAmmo = {{WeaponId::Mine, 1}};
    World world(map, hogs);
    const auto events = fireAndWait(world);
    CHECK(count(events, GameEventType::PickedPocket) == 1);
    CHECK_FALSE(carries(world.hogs()[1], WeaponId::Mine));
    CHECK(carries(world.hogs()[0], WeaponId::Mine));
    CHECK(world.hogs()[0].ownAmmo.at(WeaponId::Mine) == 1);
    CHECK(world.activeHog() == std::optional<std::size_t>(0));
    CHECK(world.phase() == MatchPhase::Playing);
}

TEST_CASE("A hiding hog's disguise takes damage first and ends at its turn", "[game][weapons][effects]") {
    // The disguise absorbs up to 20 points; what is left hurts the hog.
    // It is removed when the hog's own turn starts.
    const auto map = flatMap();
    auto hogs = facing(map, WeaponId::Rifle, 400);
    hogs[1].hideShield = weaponSpec(WeaponId::Hide).damage;
    World world(map, hogs);
    run(world, true, 1);
    run(world, false, 5);
    const float rifle = weaponSpec(WeaponId::Rifle).damage;
    CHECK(world.hogs()[1].health == Approx(kMaxHealth - std::max(0.0f, rifle - weaponSpec(WeaponId::Hide).damage)));
    run(world, false, 50 * 4);
    CHECK_FALSE(world.hogs()[1].hidden());
}
