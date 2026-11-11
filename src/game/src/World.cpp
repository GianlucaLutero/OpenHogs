// World.cpp - see World.h for an overview.

#include "openhogs/game/World.h"

#include "openhogs/core/PathUtils.h"
#include "openhogs/formats/MapObjects.h"
#include "openhogs/formats/MapTerrain.h"
#include "openhogs/formats/PackageArchive.h"

#include <stdexcept>
#include <string>
#include <utility>

namespace openhogs::game {

World::World(MapCollision map, std::vector<Hog> hogs, TurnRules rules) : map_(std::move(map)), hogs_(std::move(hogs)) {
    if (!hogs_.empty()) {
        turns_.emplace(hogs_, rules);
    }
}

std::optional<std::size_t> World::activeHog() const {
    if (!turns_) {
        return std::nullopt;
    }
    return turns_->activeHog();
}

MovementEvents World::update(double seconds, const MovementInput& input,
                             const std::vector<formats::Animation>& animations) {
    timeSeconds_ += seconds;
    MovementEvents events;
    const auto active = activeHog();

    for (std::size_t i = 0; i < hogs_.size(); ++i) {
        if (active && i == *active) {
            events = moveHog(hogs_, i, input, turns_->canMove(), seconds, map_, animations);
        } else {
            // Waiting hogs keep moving only if they are still in the air
            // (e.g. the previous team's hog was mid-jump when its turn ended).
            moveHog(hogs_, i, MovementInput{}, false, seconds, map_, animations);
        }
    }
    if (turns_) {
        turns_->update(seconds, events.distanceMoved, hogs_);
    }
    return events;
}

bool World::switchHog(int step) {
    return turns_ && turns_->switchHog(step, hogs_);
}

void World::endTurn() {
    if (turns_) {
        turns_->endTurn(hogs_);
    }
}

World loadWorld(const core::VirtualFileSystem& vfs, std::string_view mapName, TurnRules rules) {
    const std::string base = "maps/" + core::toLower(mapName);
    if (!vfs.exists(base + ".pmg")) {
        throw std::runtime_error("Map '" + std::string(mapName) + "' not found (no " + base + ".pmg)");
    }
    TerrainHeightField terrain(formats::MapTerrain::parse(vfs.readFile(base + ".pmg")));

    std::vector<Hog> hogs;
    std::vector<CollisionBox> boxes;
    if (vfs.exists(base + ".pog")) {
        const auto objects = formats::parseMapObjects(vfs.readFile(base + ".pog"));
        hogs = spawnHogs(objects, terrain);

        // Only objects with a model in the map's archive get a collision box.
        std::optional<formats::PackageArchive> models;
        if (vfs.exists(base + ".mad")) {
            models = formats::PackageArchive::parse(vfs.readFile(base + ".mad"));
        }
        boxes = collisionBoxes(objects, [&](const std::string& name) {
            return models && models->find(name + ".vtx") != nullptr;
        });
    }
    return World(MapCollision(std::move(terrain), std::move(boxes)), std::move(hogs), rules);
}

} // namespace openhogs::game
