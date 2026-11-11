// MatchAudio.h
//
// The voices, music and background sounds of a match, chosen as the
// original does (game::MatchSounds, docs/formats/audio.md) and played with
// the SoundPlayer:
//
//   - the start of the match: a battle tune;
//   - a turn starts: the hog of a person says its "ready" line; a hog of
//     the computer goes "hmmm" and its nation's tune plays;
//   - a hog attacks: its battle cry (once per turn); a hog is hurt: its cry
//     of pain; once a turn is over, the sergeant's comment;
//   - a dawdler in a match between people is told to hurry up;
//   - the end: the victory tune if a person won, the theme otherwise;
//   - all along: distant battle noise and sounds of nature.
//
// The sound effects of events (footsteps, shots, explosions...) stay in
// Game::handleEvents.

#pragma once

#include "SoundPlayer.h"

#include "openhogs/game/GameEvents.h"
#include "openhogs/game/MatchSounds.h"
#include "openhogs/game/World.h"

#include <functional>
#include <random>
#include <vector>

namespace openhogs::gameapp {

class MatchAudio {
public:
    // `isComputerTeam` tells which teams the computer plays.
    MatchAudio(SoundPlayer& sounds, const game::World& world, std::function<bool(int)> isComputerTeam);

    // The match has started (its first turn too).
    void matchStarted();

    // One event of a simulation step.
    void handle(const game::GameEvent& event);

    // Once per simulation step, after the events.
    void update(double seconds);

private:
    void turnStarted();
    void turnEnded();
    bool say(std::size_t hog, game::HogLine line);  // false if nothing was said
    bool isPerson(int team) const { return !isComputerTeam_(team); }

    SoundPlayer& sounds_;
    const game::World& world_;
    std::function<bool(int)> isComputerTeam_;
    std::mt19937 random_;

    std::vector<int> voices_;  // per hog
    std::vector<game::HogSpeech> speech_;
    game::Sergeant sergeant_;
    game::MusicDirector music_;
    game::AmbienceScheduler ambience_;

    int people_ = 0;              // teams played by a person
    int turnTeam_ = -1;           // the team whose turn it is
    game::TurnOutcome outcome_;   // of the current turn
    bool attacked_ = false;       // the active hog has used a weapon this turn
};

} // namespace openhogs::gameapp
