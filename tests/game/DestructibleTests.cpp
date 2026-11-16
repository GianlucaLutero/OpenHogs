// Tests for destructible map objects (block mission-scripts, part 3):
// how much health buildings, vehicles and scenery have, and how weapons
// destroy them. They protect the battlefield as the original plays it:
// a tent goes with one charge but a bunker takes several, an object that
// is gone must not block shots or hold hogs up, and destroying the target
// of an objective must pay its reward.

#include "GameTestSupport.h"

#include "openhogs/game/Destructibles.h"
#include "openhogs/game/Objectives.h"
#include "openhogs/game/World.h"

#include <glm/gtc/constants.hpp>

using namespace openhogs;
using namespace openhogs::tests;

namespace {

std::vector<GameEvent> runWorld(World& world, const PlayerInput& input, int steps) {
    static const auto animations = makeAnimations();
    std::vector<GameEvent> all;
    for (int i = 0; i < steps; ++i) {
        const auto events = world.update(kStep, input, animations);
        all.insert(all.end(), events.begin(), events.end());
    }
    return all;
}

PlayerInput fireKey() {
    PlayerInput input;
    input.fire = true;
    return input;
}

int count(const std::vector<GameEvent>& events, GameEventType type) {
    int n = 0;
    for (const auto& event : events) n += event.type == type ? 1 : 0;
    return n;
}

// Two hogs facing each other along X, carrying only `weapon`.
std::vector<Hog> duel(const MapCollision& map, WeaponId weapon, float distance) {
    std::vector<Hog> hogs{makeHog(map, 0, 0, 0), makeHog(map, distance, 0, 1)};
    hogs[0].object = 100;
    hogs[1].object = 101;
    hogs[1].yawRadians = glm::pi<float>();
    for (auto& hog : hogs) {
        hog.weapons = {weapon};
        hog.aimDegrees = 0.0f;
    }
    return hogs;
}

CollisionBox objectBox(std::size_t object, glm::vec3 centre, glm::vec3 halfSize) {
    CollisionBox box = makeBox(centre, halfSize);
    box.object = object;
    return box;
}

} // namespace

TEST_CASE("Objects take their health from the executable's tables", "[game][destructibles]") {
    // Buildings, vehicles and scenery each have their own table; village
    // walls and roofs share the usual 60 points; a map may set its own
    // value; scenery marked indestructible, hogs and crates cannot be
    // destroyed at all (bunkers keep their health even when marked).
    CHECK(objectHealth(makeObject("PILLBOX", 0, 0, 0)) == 100.0f);
    CHECK(objectHealth(makeObject("tent_s", 0, 0, 0)) == 25.0f);  // names are not case-sensitive
    CHECK(objectHealth(makeObject("TANK", 0, 0, 0)) == 150.0f);
    CHECK(objectHealth(makeObject("TREEG", 0, 0, 0)) == 80.0f);
    CHECK(objectHealth(makeObject("M1W06PPP", 0, 0, 0)) == 60.0f);
    CHECK(objectHealth(makeObject("DUMMY", 0, 0, 0)) == 1.0f);
    CHECK(objectClass("BIG_GUN") == ObjectClass::Building);
    CHECK(objectClass("CARRY") == ObjectClass::Vehicle);

    auto strongWall = makeObject("M1W06PPP", 0, 0, 0);
    strongWall.health = 250;
    CHECK(objectHealth(strongWall) == 250.0f);

    auto sparedBridge = makeObject("BRIDGE_S", 0, 0, 0);
    sparedBridge.presence = 0x7F;  // bit 6 set
    CHECK_FALSE(objectHealth(sparedBridge).has_value());
    auto markedBunker = makeObject("PILLBOX", 0, 0, 0);
    markedBunker.presence = 0x7F;
    CHECK(objectHealth(markedBunker) == 100.0f);

    CHECK_FALSE(objectHealth(makeObject("GR_ME", 0, 0, 0)).has_value());
    CHECK_FALSE(objectHealth(makeObject("CRATE1", 0, 0, 0)).has_value());
    CHECK_FALSE(objectHealth(makeObject("FISH", 0, 0, 0)).has_value());
}

TEST_CASE("An object is gone once it has less than one point left", "[game][destructibles]") {
    // The original destroys at 127/128 of a point; damage on a destroyed
    // object changes nothing and is not reported again.
    Destructible tent{0, ObjectClass::Building, 25.0f, false};
    CHECK_FALSE(damageObject(tent, 24.0f));
    CHECK(tent.health == Approx(1.0f));
    CHECK(damageObject(tent, 0.5f));
    CHECK(tent.destroyed);
    CHECK_FALSE(damageObject(tent, 10.0f));
}

TEST_CASE("An explosion destroys a tent, clears its box and pays its objective", "[game][destructibles]") {
    // A hog puts TNT down next to a tent: the tent blows up, its box no
    // longer blocks the way, and the objective on it drops its crate.
    const glm::vec3 tentCentre{180.0f, 100.0f, 0.0f};
    const auto map = flatMap({objectBox(0, tentCentre, {60.0f, 100.0f, 60.0f})});
    World world(map, duel(map, WeaponId::Tnt, 2000.0f));
    world.setDestructibles({{0, ObjectClass::Building, 25.0f, false}});
    formats::MapObject tent = makeObject("TENT_S", 180, 100, 0);
    tent.scriptType = 1;
    tent.scriptParams = {255, 20};
    tent.scriptX = 600;
    world.setObjectives(findObjectives({tent}), 1);

    runWorld(world, fireKey(), 1);
    const auto events = runWorld(world, PlayerInput{}, 50 * 6);
    REQUIRE(count(events, GameEventType::ObjectDestroyed) == 1);
    for (const auto& event : events) {
        if (event.type == GameEventType::ObjectDestroyed) {
            CHECK(event.object == 0);
            CHECK(event.amount == 1.0f);  // a building blows up
        }
    }
    CHECK(world.destructibles()[0].destroyed);
    CHECK(world.map().boxes().empty());
    CHECK(count(events, GameEventType::ObjectiveDone) == 1);
    REQUIRE(world.pickups().size() == 1);
    CHECK(world.pickups()[0].kind == PickupKind::Health);
}

TEST_CASE("A bullet that hits an object damages it, and stops there", "[game][destructibles]") {
    // A dummy (one point) between two hogs takes the rifle shot meant for
    // the enemy behind it and falls; a strong wall only loses health.
    const auto map = flatMap({objectBox(0, {200.0f, 150.0f, 0.0f}, {20.0f, 200.0f, 60.0f})});

    World dummy(map, duel(map, WeaponId::Rifle, 400.0f));
    dummy.setDestructibles({{0, ObjectClass::Scenery, 1.0f, false}});
    const auto shot = runWorld(dummy, fireKey(), 1);  // the shot leaves in this step
    const auto after = runWorld(dummy, PlayerInput{}, 5);
    CHECK(count(shot, GameEventType::ObjectDestroyed) + count(after, GameEventType::ObjectDestroyed) == 1);
    CHECK(dummy.hogs()[1].health == Approx(kMaxHealth));
    CHECK(dummy.map().boxes().empty());

    World wall(map, duel(map, WeaponId::Rifle, 400.0f));
    wall.setDestructibles({{0, ObjectClass::Scenery, 250.0f, false}});
    runWorld(wall, fireKey(), 1);
    runWorld(wall, PlayerInput{}, 5);
    CHECK(wall.destructibles()[0].health == Approx(250.0f - weaponSpec(WeaponId::Rifle).damage));
    CHECK_FALSE(wall.destructibles()[0].destroyed);
    CHECK(wall.map().boxes().size() == 1);
}
