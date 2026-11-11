// FixedTimestep.h
//
// Runs the game simulation at a fixed rate, whatever the frame rate of the
// display.
//
// Screens refresh at 60, 144 or any other rate, and a frame can take longer
// when the computer is busy. If the game advanced by "the time since the
// last frame", physics and timing would differ from one computer to the
// next. Instead the simulation always advances in steps of the same length
// (kSimulationStepSeconds); each frame runs as many steps as the elapsed
// time contains and keeps the remainder for the next frame.
//
// If the program stalls (window dragged, breakpoint...), the backlog is
// capped: the game slows down for a moment instead of freezing while it
// simulates seconds of missed time.

#pragma once

namespace openhogs::game {

// 50 steps per second: twice the 25 frames per second of the original
// animations, so an animation frame is always a whole number of steps.
inline constexpr double kSimulationStepSeconds = 1.0 / 50.0;

class FixedTimestep {
public:
    explicit FixedTimestep(double stepSeconds = kSimulationStepSeconds, int maxStepsPerFrame = 5);

    // Adds the real time elapsed since the last call and returns how many
    // simulation steps to run now (0 to maxStepsPerFrame).
    int advance(double elapsedSeconds);

    double stepSeconds() const { return stepSeconds_; }

    // How far the real time is into the next step, from 0 to 1. Drawing can
    // use it to place moving things between two simulation steps.
    double alpha() const { return accumulated_ / stepSeconds_; }

private:
    double stepSeconds_;
    int maxStepsPerFrame_;
    double accumulated_ = 0.0;
};

} // namespace openhogs::game
