// IdleAnimator.h
//
// Decides what a hog does while it has nothing to do, so a waiting team
// looks alive instead of frozen.
//
// The hog repeats the "standing around" cycle. Each time the cycle ends it
// may play one short fidget (looking around, brushing dust off...) and then
// goes back to standing. Changes of animation are cross-faded over a
// fraction of a second so the body never snaps from one stance to another.
//
// The choices come from a random generator seeded per hog: hogs do not
// fidget in unison, and the same seed always gives the same sequence, which
// keeps the game reproducible and testable.
//
// This is OpenHogs' own behaviour; how the original chooses idle
// animations is not known yet.

#pragma once

#include "openhogs/formats/MotionCapture.h"
#include "openhogs/formats/Skeleton.h"
#include "openhogs/scene/CharacterPose.h"

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace openhogs::game {

// Animation numbers (names in scene::animationName).
inline constexpr std::size_t kStandingAnimation = 27;  // "Standing around cycle 1"

class IdleAnimator {
public:
    explicit IdleAnimator(std::uint32_t seed = 0);

    // Advances the animation by `seconds`.
    void update(double seconds, const std::vector<formats::Animation>& animations);

    std::size_t animation() const { return animation_; }
    float frame() const { return frame_; }

    // The hog's pose now, including the cross-fade from the previous
    // animation while one is in progress.
    scene::Pose pose(const formats::Skeleton& skeleton, const std::vector<formats::Animation>& animations) const;

private:
    void startAnimation(std::size_t animation);
    std::size_t chooseNext(const std::vector<formats::Animation>& animations);

    std::mt19937 random_;
    std::size_t animation_ = kStandingAnimation;
    float frame_ = 0.0f;

    // Cross-fade: the pose the hog was in when the animation changed.
    std::size_t previousAnimation_ = kStandingAnimation;
    float previousFrame_ = 0.0f;
    float fadeSecondsLeft_ = 0.0f;
};

} // namespace openhogs::game
