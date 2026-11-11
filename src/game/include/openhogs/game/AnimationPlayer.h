// AnimationPlayer.h
//
// Plays the animations of one hog: which animation, which frame, whether
// it repeats, and the cross-fade from the previous animation.
//
// Every system that animates a hog (idle behaviour, movement, later
// weapons and deaths) asks the player to play an animation; the player
// takes care of timing and of blending smoothly from what the hog was
// doing before, so a hog never snaps from one stance to another.
//
// Animations play at scene::kAnimationFramesPerSecond (25), the speed of
// the original game.

#pragma once

#include "openhogs/formats/MotionCapture.h"
#include "openhogs/formats/Skeleton.h"
#include "openhogs/scene/CharacterPose.h"

#include <cstddef>
#include <vector>

namespace openhogs::game {

class AnimationPlayer {
public:
    enum class Repeat {
        Loop,  // starts again after the last frame (running, standing)
        Once,  // stops on the last frame (a jump, a fidget)
    };

    // Starts `animation`, cross-fading from the current pose. Asking for
    // the animation that is already playing (with the same repeat mode)
    // changes nothing, so callers can ask every step; `restart` forces it
    // to start again from the first frame.
    void play(std::size_t animation, Repeat repeat, bool restart = false);

    // Jumps to a frame without cross-fading (e.g. to desynchronise hogs).
    void setFrame(float frame) { frame_ = frame; }

    // Advances by `seconds`.
    void update(double seconds, const std::vector<formats::Animation>& animations);

    std::size_t animation() const { return animation_; }
    float frame() const { return frame_; }
    Repeat repeat() const { return repeat_; }

    // A "Once" animation has reached its last frame.
    bool finished() const { return finished_; }

    // The last update went past the end of a "Loop" animation, or
    // finished a "Once" one: the moment to pick what comes next.
    bool completedCycle() const { return completedCycle_; }

    // The last update moved through `frame` (used to time footsteps).
    bool passedFrame(float frame) const;

    // The pose now, including the cross-fade from the previous animation.
    scene::Pose pose(const formats::Skeleton& skeleton, const std::vector<formats::Animation>& animations) const;

private:
    std::size_t animation_ = 0;
    Repeat repeat_ = Repeat::Loop;
    float frame_ = 0.0f;
    bool finished_ = false;
    bool completedCycle_ = false;

    // Frames covered by the last update: from previousFrame_ to frame_,
    // wrapping around the end when wrapped_ is set.
    float previousFrame_ = 0.0f;
    bool wrapped_ = false;

    // Cross-fade: the pose the hog was in when the animation changed.
    std::size_t fadeAnimation_ = 0;
    float fadeFrame_ = 0.0f;
    float fadeSecondsLeft_ = 0.0f;
};

} // namespace openhogs::game
