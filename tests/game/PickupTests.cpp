// Tests for the crates of a map (block mission-scripts): what a crate
// holds, read from its map object, and what happens when a hog walks into
// it. They protect the player's rewards: a crate must give what the map
// says, once, and a weapon from a crate must run out when it is used up.

#include "GameTestSupport.h"

#include "openhogs/game/Pickups.h"
#include "openhogs/game/World.h"

using namespace openhogs;
using namespace openhogs::tests;
using Catch::Approx;

namespace {

formats::MapObject crate(const char* model, std::uint8_t item, std::uint8_t amount, float x = 0.0f, float z = 0.0f) {
    formats::MapObject object;
    object.modelName = model;
    object.scriptType = kPickupScriptType;
    object.scriptParams[0] = item;
    object.scriptParams[1] = amount;
    object.x = static_cast<std::int16_t>(x);
    object.z = static_cast<std::int16_t>(z);
    object.y = 95;
    object.boxSizeX = object.boxSizeY = object.boxSizeZ = 3;
    return object;
}

PlayerInput walkForward() {
    PlayerInput input;
    input.movement.forward = true;
    return input;
}

} // namespace

TEST_CASE("Crates hold what their map object says", "[game][pickups]") {
    // Item 255 is health, PROPOINT gives promotion points, and other items
    // go through the executable's table: item 30 is the medicine ball
    // (weapon 33), item 13 the flame thrower (14), item 14 the rocket
    // launcher (15). Other objects are not pickups.
    std::vector<formats::MapObject> objects{crate("CRATE2", 255, 50), crate("PROPOINT", 1, 1),
                                            crate("CRATE1", 14, 3), crate("CRATE1", 30, 1)};
    formats::MapObject tree;
    tree.modelName = "TREEG";
    objects.push_back(tree);
    const auto pickups = findPickups(objects);
    REQUIRE(pickups.size() == 4);
    CHECK(pickups[0].kind == PickupKind::Health);
    CHECK(pickups[0].amount == 50);
    CHECK(pickups[1].kind == PickupKind::PromotionPoints);
    CHECK(pickups[2].kind == PickupKind::Weapon);
    CHECK(pickups[2].weapon() == WeaponId::RocketLauncher);
    CHECK(pickups[2].amount == 3);
    CHECK(pickups[3].item == 33);
    CHECK_FALSE(pickups[3].weapon().has_value());  // the medicine ball is not made yet
    CHECK(crateItemWeapon(13) == 14);
    CHECK(crateItemWeapon(73) == -1);
}

TEST_CASE("A hog walking into a health crate is healed, once, up to its full health", "[game][pickups]") {
    // The crate gives at most what is missing and disappears: neither its
    // picture nor its box may stay to be picked up again.
    const auto map = flatMap();
    std::vector<Hog> hogs{makeHog(map, 0, 0, 0), makeHog(map, -3000, 0, 1)};
    hogs[0].health = 60.0f;
    World world(map, hogs, {}, findPickups({crate("CRATE2", 255, 20, 250.0f)}));
    std::vector<GameEvent> picked;
    for (int step = 0; step < 100; ++step) {
        for (const auto& event : world.update(kStep, walkForward(), {})) {
            if (event.type == GameEventType::PickedUp) picked.push_back(event);
        }
    }
    REQUIRE(picked.size() == 1);
    CHECK(world.hogs()[0].health == Approx(80.0f));
    CHECK(picked[0].amount == Approx(20.0f));
    CHECK(world.pickups()[0].taken);
}

TEST_CASE("A weapon crate stocks its team, and the weapon runs out when used up", "[game][pickups]") {
    // Every hog of the team can use the crate's weapon; each use spends
    // one, and the last one takes it out of their hands, while the hogs'
    // own weapons never run out.
    const auto map = flatMap();
    std::vector<Hog> hogs{makeHog(map, 0, 0, 0), makeHog(map, 0, 600, 0), makeHog(map, -3000, 0, 1)};
    World world(map, hogs, {}, findPickups({crate("CRATE1", 14, 1, 250.0f)}));
    for (int step = 0; step < 100 && !world.pickups()[0].taken; ++step) world.update(kStep, walkForward(), {});
    REQUIRE(world.pickups()[0].taken);
    CHECK(world.stock(0, WeaponId::RocketLauncher) == 1);
    for (std::size_t hog : {std::size_t{0}, std::size_t{1}}) {
        const auto& weapons = world.hogs()[hog].weapons;
        CHECK(std::find(weapons.begin(), weapons.end(), WeaponId::RocketLauncher) != weapons.end());
    }
    CHECK(world.stock(1, WeaponId::RocketLauncher) == 0);

    // Select the rocket launcher and fire it.
    PlayerInput select;
    select.weaponStep = 1;
    for (int tries = 0; tries < 20 && world.hogs()[0].selectedWeapon() != WeaponId::RocketLauncher; ++tries) {
        world.update(kStep, select, {});
    }
    REQUIRE(world.hogs()[0].selectedWeapon() == WeaponId::RocketLauncher);
    PlayerInput fire;
    fire.fire = true;
    world.update(kStep, fire, {});
    world.update(kStep, {}, {});
    CHECK(world.stock(0, WeaponId::RocketLauncher) == 0);
    const auto& weapons = world.hogs()[1].weapons;
    CHECK(std::find(weapons.begin(), weapons.end(), WeaponId::RocketLauncher) == weapons.end());
}
