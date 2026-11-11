// IdleAnimator.h
//
// Decides what a hog does while it has nothing to do, so a waiting team
// looks alive instead of frozen.
//
// The hog repeats the "standing around" cycle. Each time the cycle ends it
// may play one short fidget (looking around, brushing dust off...) and then
// goes back to standing. The animation itself is played by an
// AnimationPlayer, which also cross-fades the changes.
//
// The choices come from a random generator seeded per hog: hogs do not
// fidget in unison, and the same seed always gives the same sequence, which
// keeps the game reproducible and testable.
//
// This is OpenHogs' own behaviour; how the original chooses idle
// animations is not known yet (see docs/open-decisions.md).

#pragma once

#include "openhogs/game/AnimationPlayer.h"

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

    // Starts standing at a random point of the cycle, so hogs created
    // together do not move in step.
    void start(AnimationPlayer& player);

    // Call after player.update(): chooses what comes next when the current
    // idle animation ends. If the player is playing something that is not
    // an idle animation (the hog just stopped running...), it goes back to
    // standing.
    void update(AnimationPlayer& player, const std::vector<formats::Animation>& animations);

private:
    bool isIdleAnimation(std::size_t animation) const;

    std::mt19937 random_;
};

} // namespace openhogs::game
