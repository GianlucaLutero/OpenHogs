// FixedTimestep.cpp - see FixedTimestep.h for an overview.

#include "openhogs/game/FixedTimestep.h"

#include <stdexcept>

namespace openhogs::game {

FixedTimestep::FixedTimestep(double stepSeconds, int maxStepsPerFrame)
    : stepSeconds_(stepSeconds), maxStepsPerFrame_(maxStepsPerFrame) {
    if (stepSeconds <= 0.0 || maxStepsPerFrame < 1) {
        throw std::invalid_argument("FixedTimestep needs a positive step and at least one step per frame");
    }
}

int FixedTimestep::advance(double elapsedSeconds) {
    if (elapsedSeconds > 0.0) {
        accumulated_ += elapsedSeconds;
    }

    int steps = 0;
    while (accumulated_ >= stepSeconds_ && steps < maxStepsPerFrame_) {
        accumulated_ -= stepSeconds_;
        ++steps;
    }
    // Still a whole step or more behind: the program stalled. Drop the
    // backlog instead of trying to catch up over the next frames.
    if (accumulated_ >= stepSeconds_) {
        accumulated_ = 0.0;
    }
    return steps;
}

} // namespace openhogs::game
