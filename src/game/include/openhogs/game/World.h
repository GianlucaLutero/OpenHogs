// World.h
//
// The state of the match being played: the terrain and the hogs on it.
//
// The world only changes through update(), which the game calls at a
// fixed rate (see FixedTimestep). It knows nothing about drawing or sound:
// the application reads the world and shows it.

#pragma once

#include "openhogs/core/VirtualFileSystem.h"
#include "openhogs/game/Hog.h"
#include "openhogs/game/TerrainHeightField.h"

#include <optional>
#include <string_view>
#include <vector>

namespace openhogs::game {

class World {
public:
    World(TerrainHeightField terrain, std::vector<Hog> hogs);

    const TerrainHeightField& terrain() const { return terrain_; }
    const std::vector<Hog>& hogs() const { return hogs_; }

    // Seconds of simulated time since the match started.
    double timeSeconds() const { return timeSeconds_; }

    // Advances the match by one simulation step.
    void update(double seconds, const std::vector<formats::Animation>& animations);

    // The hog the camera follows (and, in later blocks, the player moves).
    // Empty when the map has no hogs. Starts on the first hog marked for
    // the player, or the first hog.
    std::optional<std::size_t> selectedHog() const { return selectedHog_; }

    // Selects the next (+1) or previous (-1) hog, wrapping around.
    void selectNextHog(int step);

private:
    TerrainHeightField terrain_;
    std::vector<Hog> hogs_;
    std::optional<std::size_t> selectedHog_;
    double timeSeconds_ = 0.0;
};

// Loads the terrain (Maps/<NAME>.PMG) and the hogs (Maps/<NAME>.POG) of a
// map. Throws std::runtime_error if the terrain is missing; a map without
// a .POG file simply has no hogs.
World loadWorld(const core::VirtualFileSystem& vfs, std::string_view mapName);

} // namespace openhogs::game
