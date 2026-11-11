// World.cpp - see World.h for an overview.

#include "openhogs/game/World.h"

#include "openhogs/core/PathUtils.h"
#include "openhogs/formats/MapObjects.h"
#include "openhogs/formats/MapTerrain.h"

#include <stdexcept>
#include <string>
#include <utility>

namespace openhogs::game {

World::World(TerrainHeightField terrain, std::vector<Hog> hogs) : terrain_(std::move(terrain)), hogs_(std::move(hogs)) {
    for (std::size_t i = 0; i < hogs_.size(); ++i) {
        if (hogs_[i].playerControlled) {
            selectedHog_ = i;
            break;
        }
    }
    if (!selectedHog_ && !hogs_.empty()) {
        selectedHog_ = 0;
    }
}

void World::update(double seconds, const std::vector<formats::Animation>& animations) {
    timeSeconds_ += seconds;
    for (auto& hog : hogs_) {
        hog.animator.update(seconds, animations);
    }
}

void World::selectNextHog(int step) {
    if (!selectedHog_) {
        return;
    }
    const auto count = static_cast<long long>(hogs_.size());
    const long long next = (static_cast<long long>(*selectedHog_) + step) % count;
    selectedHog_ = static_cast<std::size_t>(next < 0 ? next + count : next);
}

World loadWorld(const core::VirtualFileSystem& vfs, std::string_view mapName) {
    const std::string base = "maps/" + core::toLower(mapName);
    if (!vfs.exists(base + ".pmg")) {
        throw std::runtime_error("Map '" + std::string(mapName) + "' not found (no " + base + ".pmg)");
    }
    TerrainHeightField terrain(formats::MapTerrain::parse(vfs.readFile(base + ".pmg")));

    std::vector<Hog> hogs;
    if (vfs.exists(base + ".pog")) {
        hogs = spawnHogs(formats::parseMapObjects(vfs.readFile(base + ".pog")), terrain);
    }
    return World(std::move(terrain), std::move(hogs));
}

} // namespace openhogs::game
