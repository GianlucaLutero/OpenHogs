// Tests for mission objectives (block mission-scripts, part 2): the jobs a
// map gives through the script types of its objects, and the reward crates
// they drop. They protect the campaign's rewards: an objective must pay
// what the map says, where it says, only once, and a group must pay only
// when all of it is gone.

#include "GameTestSupport.h"

#include "openhogs/game/Objectives.h"
#include "openhogs/game/World.h"

using namespace openhogs;
using namespace openhogs::tests;
using Catch::Approx;

namespace {

formats::MapObject scripted(int type, std::uint8_t item, std::uint8_t amount, std::uint8_t group = 0) {
    formats::MapObject object;
    object.modelName = "GR_ME";
    object.scriptType = static_cast<std::int16_t>(type);
    object.scriptGroup = group;
    object.scriptParams[0] = item;
    object.scriptParams[1] = amount;
    object.scriptX = 1000;
    object.scriptY = 40;
    object.scriptZ = -2000;
    return object;
}

PlayerInput walkForward() {
    PlayerInput input;
    input.movement.forward = true;
    return input;
}

} // namespace

TEST_CASE("Objectives read their reward from the map object", "[game][objectives]") {
    // Crate rewards go through the executable's item table like crates do
    // (item 14 is the rocket launcher, weapon 15), health stays 255, and
    // the reward lies at the script position. Crates themselves (19) and
    // plain objects (0) are not objectives.
    std::vector<formats::MapObject> objects{scripted(1, 14, 2), scripted(7, 255, 30), scripted(19, 1, 1),
                                            scripted(0, 0, 0), scripted(4, 3, 0)};
    const auto objectives = findObjectives(objects);
    REQUIRE(objectives.size() == 3);
    CHECK(objectives[0].object == 0);
    CHECK(objectives[0].item == 15);
    CHECK(objectives[0].amount == 2);
    CHECK(objectives[0].target == glm::vec3(1000.0f, 40.0f, -2000.0f));
    CHECK(objectives[1].item == 255);
    CHECK(objectives[2].type == 4);
    CHECK(objectives[2].item == 3);  // points to lose: not a crate item
}

TEST_CASE("A reach objective pays once its hog stands in the target cell", "[game][objectives]") {
    // The original compares 512-unit cells, not exact points: anywhere in
    // the target's cell counts, the next cell does not. The reward lies at
    // the hog's feet, and an objective done is not paid again.
    Objective objective = findObjectives({scripted(7, 255, 30)})[0];
    CHECK(objectiveCell(1000.0f) == objectiveCell(1023.0f));
    CHECK_FALSE(reachObjective(objective, {1100.0f, 0.0f, -2000.0f}).has_value());  // next cell
    CHECK_FALSE(reachObjective(objective, {1010.0f, 50.0f, -1400.0f}).has_value());  // right x, wrong z
    const auto result = reachObjective(objective, {1010.0f, 50.0f, -2040.0f});
    REQUIRE(result.has_value());
    REQUIRE(result->drop.has_value());
    CHECK(result->drop->kind == PickupKind::Health);
    CHECK(result->drop->amount == 30);
    CHECK(result->drop->model == "CRATE2");
    CHECK(result->drop->centre.x == Approx(1010.0f));
    CHECK(result->drop->centre.y > 50.0f);  // standing on the ground, not in it
    CHECK(result->sergeantSection == 5);
    CHECK_FALSE(reachObjective(objective, {1010.0f, 50.0f, -2040.0f}).has_value());
}

TEST_CASE("A group objective pays only when the last of its group is destroyed", "[game][objectives]") {
    // A bridge in five parts gives its crate for the last part, wherever
    // in the list it is; objectives of other groups do not hold it back.
    std::vector<formats::MapObject> objects{scripted(13, 14, 1, 30), scripted(13, 14, 1, 30),
                                            scripted(13, 14, 1, 30), scripted(1, 255, 20, 0)};
    auto objectives = findObjectives(objects);
    CHECK_FALSE(destroyObjective(objectives, 2).has_value());
    CHECK_FALSE(destroyObjective(objectives, 0).has_value());
    CHECK_FALSE(destroyObjective(objectives, 0).has_value());  // already gone
    const auto last = destroyObjective(objectives, 1);
    REQUIRE(last.has_value());
    REQUIRE(last->drop.has_value());
    CHECK(last->drop->kind == PickupKind::Weapon);
    CHECK(last->drop->weapon() == WeaponId::RocketLauncher);
    CHECK(last->drop->centre.x == Approx(1000.0f));  // at the target, not where the part was
    CHECK(last->sergeantSection == 4);
}

TEST_CASE("Some objectives cost points, and a hog that dies loses its errand", "[game][objectives]") {
    // Destroying what should be spared (type 4) takes promotion points;
    // a hog killed before reaching its target (type 8) brings nothing,
    // even if its body lies in the target cell.
    auto objectives = findObjectives({scripted(4, 2, 0), scripted(8, 1, 0)});
    const auto spared = destroyObjective(objectives, 0);
    REQUIRE(spared.has_value());
    CHECK(spared->pointsLost == 2);
    CHECK_FALSE(spared->drop.has_value());
    CHECK_FALSE(destroyObjective(objectives, 1).has_value());
    CHECK_FALSE(reachObjective(objectives[1], {1000.0f, 40.0f, -2000.0f}).has_value());
}

TEST_CASE("In a match, reaching the target drops a crate the hog picks up", "[game][objectives]") {
    // The whole chain as the player sees it: the hog walks into the target
    // cell, the reward crate appears (with an object number after the
    // map's objects, so it can be drawn and hidden), comes down by
    // parachute, and the hog takes it once it has landed.
    const auto map = flatMap();
    std::vector<Hog> hogs{makeHog(map, 0, 0, 0), makeHog(map, -3000, 0, 1)};
    hogs[0].object = 0;
    hogs[0].health = 50.0f;
    hogs[1].object = 1;
    formats::MapObject errand = scripted(7, 255, 25);
    errand.scriptX = 700;
    errand.scriptZ = 0;
    World world(map, hogs);
    world.setObjectives(findObjectives({errand}), 2);

    int done = 0;
    int picked = 0;
    // Walk to the target cell, then wait for the crate, which comes down by
    // parachute at the hog's feet.
    for (int step = 0; step < 150 + 50 * 6; ++step) {
        const PlayerInput input = done == 0 ? walkForward() : PlayerInput{};
        for (const auto& event : world.update(kStep, input, {})) {
            if (event.type == GameEventType::ObjectiveDone) {
                ++done;
                CHECK(event.dropped);
                CHECK(event.sergeantSection == 5);
                CHECK(world.pickups()[event.pickup].dropHeight == Approx(kCrateDropHeight));  // up in the sky
            }
            if (event.type == GameEventType::PickedUp) ++picked;
        }
    }
    CHECK(done == 1);
    REQUIRE(world.pickups().size() == 1);
    CHECK(world.pickups()[0].object == 2);
    CHECK(world.pickups()[0].taken);
    CHECK(picked == 1);
    CHECK(world.hogs()[0].health == Approx(75.0f));
}
