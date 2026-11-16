// Tests for the Boot Camp (block mission-scripts, part 5): the chain of
// hidden objects that leads the player through the training, and the
// training mode's own rules. They protect the first mission of every
// campaign: a crate must appear when its dummies are gone, never before,
// and the training must end once its crates are all taken.

#include "GameTestSupport.h"

#include "openhogs/game/Objectives.h"
#include "openhogs/game/Training.h"
#include "openhogs/game/World.h"

using namespace openhogs;
using namespace openhogs::tests;

namespace {

formats::MapObject scripted(const char* model, int type, std::uint8_t group, std::uint8_t parameter) {
    formats::MapObject object = makeObject(model, 0, 0, 0);
    object.scriptType = static_cast<std::int16_t>(type);
    object.scriptGroup = group;
    object.scriptParams[0] = parameter;
    return object;
}

formats::MapObject crate(std::uint8_t group, std::uint8_t item, float x) {
    formats::MapObject object = makeObject("CRATE1", static_cast<int>(x), 96, 0);
    object.scriptType = kPickupScriptType;
    object.scriptGroup = group;
    object.scriptParams[0] = item;
    object.scriptParams[1] = 1;
    object.boxSizeX = object.boxSizeY = 3;
    object.boxSizeZ = 4;
    return object;
}

int countEvents(const std::vector<GameEvent>& events, GameEventType type) {
    int n = 0;
    for (const auto& event : events) n += event.type == type ? 1 : 0;
    return n;
}

bool hasTip(const std::vector<GameEvent>& events, int text) {
    for (const auto& event : events) {
        if (event.type == GameEventType::TrainingTip && event.text == text) return true;
    }
    return false;
}

} // namespace

TEST_CASE("The Boot Camp's chain reveals each group when the one before is gone", "[game][training]") {
    // As on CAMP: a dummy (type 20) reveals group 2, whose two dummies
    // (type 23) must both go before group 3 appears. Hidden objects are
    // types 21 and 23, and in training the crates with a group; a crate's
    // objective ends as it appears, so it does not hold its group back.
    std::vector<formats::MapObject> objects{scripted("DUMMY", 20, 1, 2), scripted("DUMMY", 23, 2, 3),
                                            scripted("DUMMY", 23, 2, 3), crate(2, 7, 0.0f), crate(3, 11, 0.0f)};
    CHECK_FALSE(startsHidden(objects[0], true));
    CHECK(startsHidden(objects[1], true));
    CHECK(startsHidden(objects[3], true));
    CHECK_FALSE(startsHidden(objects[3], false));  // outside training crates show

    auto objectives = findObjectives(objects, true);
    REQUIRE(objectives.size() == 5);
    const std::vector<std::size_t> crates{3, 4};

    const auto first = destroyObjective(objectives, 0);
    REQUIRE(first.has_value());
    REQUIRE(first->revealGroup == 2);
    CHECK(revealGroup(objectives, 2, crates) == std::vector<std::size_t>{1, 2, 3});

    CHECK_FALSE(destroyObjective(objectives, 1).has_value());  // one dummy of group 2 still stands
    const auto second = destroyObjective(objectives, 2);
    REQUIRE(second.has_value());
    CHECK(second->revealGroup == 3);
    CHECK(revealGroup(objectives, 3, crates) == std::vector<std::size_t>{4});
}

TEST_CASE("Training: the dummy falls, the crate appears with its tip, and taking it ends the training",
          "[game][training]") {
    // The hog starts without weapons and punches the dummy in front of it;
    // the hidden rifle crate appears (FOLLOW THE YELLOW PATH...), cannot be
    // taken before, and once taken gives a rifle that never runs out.
    // With every crate of the chain taken the training is over.
    std::vector<formats::MapObject> objects{scripted("DUMMY", 20, 1, 2), crate(2, 7, 300.0f)};
    CollisionBox dummyBox = makeBox({90.0f, 150.0f, 0.0f}, {20.0f, 150.0f, 20.0f});
    dummyBox.object = 0;
    CollisionBox crateBox = makeBox({300.0f, 96.0f, 0.0f}, {96.0f, 96.0f, 128.0f});
    crateBox.object = 1;
    const auto map = flatMap({dummyBox, crateBox});
    std::vector<Hog> hogs{makeHog(map, 0, 0, 0)};
    hogs[0].weapons = {WeaponId::Rifle};
    World world(map, hogs, {}, findPickups(objects));
    world.setObjectives(findObjectives(objects, true), objects.size());
    world.setDestructibles({{0, ObjectClass::Scenery, 1.0f, false}});
    world.setTraining(true);
    world.hideObjects({1});
    CHECK(world.hogs()[0].weapons.empty());
    CHECK(world.isHidden(1));

    static const auto animations = makeAnimations();
    std::vector<GameEvent> events;
    const auto run = [&](const PlayerInput& input, int steps) {
        for (int i = 0; i < steps; ++i) {
            const auto step = world.update(kStep, input, animations);
            events.insert(events.end(), step.begin(), step.end());
        }
    };
    run(PlayerInput{}, 1);
    CHECK(hasTip(events, 211));  // FOLLOW THE GREEN PATH AND COLLECT THE CRATE.

    PlayerInput punch;
    punch.fire = true;
    run(punch, 1);
    run(PlayerInput{}, 50 * 3);
    CHECK(world.destructibles()[0].destroyed);
    CHECK(countEvents(events, GameEventType::ObjectRevealed) == 1);
    CHECK_FALSE(world.isHidden(1));
    CHECK(hasTip(events, 215));
    CHECK(world.phase() != MatchPhase::Over);

    PlayerInput walk;
    walk.movement.forward = true;
    for (int i = 0; i < 400 && world.phase() != MatchPhase::Over; ++i) run(walk, 1);
    CHECK(world.pickups()[0].taken);
    CHECK(hasTip(events, 217));
    REQUIRE_FALSE(world.hogs()[0].weapons.empty());
    CHECK(world.hogs()[0].selectedWeapon() == WeaponId::Rifle);
    for (int i = 0; i < 50 * 5 && world.phase() != MatchPhase::Over; ++i) run(PlayerInput{}, 1);
    CHECK(world.phase() == MatchPhase::Over);
    CHECK(world.winner() == 0);
}

TEST_CASE("Training tips and speech follow the executable", "[game][training]") {
    // Crates pick their tip by weapon (and number of grenades), health
    // crates by amount; the instructor's files are numbered by line.
    Pickup grenades;
    grenades.kind = PickupKind::Weapon;
    grenades.item = 19;
    grenades.amount = 10;
    REQUIRE(tipWhenCrateAppears(grenades).has_value());
    CHECK(tipWhenCrateAppears(grenades)->text == 224);  // FOLLOW THE BLUE PATH ...
    CHECK(tipWhenCratePicked(grenades)->line == 16);
    Pickup health;
    health.kind = PickupKind::Health;
    health.amount = 25;
    CHECK(tipWhenCratePicked(health)->text == 233);  // USE BACKSPACE ...
    CHECK_FALSE(tipWhenCrateAppears(health).has_value());
    CHECK(trainingSpeechPath(2) == "Speech/Sku1/Train1/tr1_en02.wav");
    CHECK(isTrainingMap("camp"));
    CHECK_FALSE(isTrainingMap("ESTU"));
}
