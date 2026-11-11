// World.h
//
// The state of the match being played: the map, the hogs on it and whose
// turn it is.
//
// The world only changes through update(), which the game calls at a
// fixed rate (see FixedTimestep). It knows nothing about drawing or sound:
// the application reads the world and shows it, and turns the events of
// each step into sounds.

#pragma once

#include "openhogs/core/VirtualFileSystem.h"
#include "openhogs/game/Hog.h"
#include "openhogs/game/HogMovement.h"
#include "openhogs/game/MapCollision.h"
#include "openhogs/game/TurnSystem.h"

#include <optional>
#include <string_view>
#include <vector>

namespace openhogs::game {

class World {
public:
    World(MapCollision map, std::vector<Hog> hogs, TurnRules rules = {});

    const MapCollision& map() const { return map_; }
    const TerrainHeightField& terrain() const { return map_.terrain(); }
    const std::vector<Hog>& hogs() const { return hogs_; }

    // Empty when the map has no hogs (a map without hogs is only scenery).
    const std::optional<TurnSystem>& turns() const { return turns_; }

    // The hog whose turn it is; empty when the map has no hogs.
    std::optional<std::size_t> activeHog() const;

    // Seconds of simulated time since the match started.
    double timeSeconds() const { return timeSeconds_; }

    // Advances the match by one simulation step. `input` controls the
    // active hog; the other hogs wait. Returns what the active hog did.
    MovementEvents update(double seconds, const MovementInput& input, const std::vector<formats::Animation>& animations);

    // Picks another hog of the active team (only before the active hog has
    // moved). Returns true if the active hog changed.
    bool switchHog(int step);

    // Ends the active team's turn now.
    void endTurn();

private:
    MapCollision map_;
    std::vector<Hog> hogs_;
    std::optional<TurnSystem> turns_;
    double timeSeconds_ = 0.0;
};

// Loads the terrain (Maps/<NAME>.PMG), the objects' collision boxes and
// the hogs (Maps/<NAME>.POG, with the models in Maps/<NAME>.MAD) of a map.
// Throws std::runtime_error if the terrain is missing; a map without a .POG
// file simply has no hogs and no obstacles.
World loadWorld(const core::VirtualFileSystem& vfs, std::string_view mapName, TurnRules rules = {});

} // namespace openhogs::game
