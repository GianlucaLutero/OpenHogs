// IdleAnimator.cpp - see IdleAnimator.h for an overview.

#include "openhogs/game/IdleAnimator.h"

#include <array>
#include <algorithm>

namespace openhogs::game {

namespace {

// Short animations that start and end in the standing stance (measured on
// the game data: their first and last frames are a few units away from the
// standing cycle's): standing cycle 2, looking around, three "brush off".
constexpr std::array<std::size_t, 5> kFidgets{28, 31, 34, 35, 36};

// Chance, in percent, of a fidget each time the standing cycle ends (the
// cycle lasts 1.44 s, so about one fidget every 7 seconds).
constexpr unsigned kFidgetChancePercent = 20;

// Length of the standing cycle in the original data.
constexpr unsigned kStandingFrames = 36;

} // namespace

IdleAnimator::IdleAnimator(std::uint32_t seed) : random_(seed) {}

void IdleAnimator::start(AnimationPlayer& player) {
    player.play(kStandingAnimation, AnimationPlayer::Repeat::Loop);
    player.setFrame(static_cast<float>(random_() % kStandingFrames));
}

bool IdleAnimator::isIdleAnimation(std::size_t animation) const {
    return animation == kStandingAnimation ||
           std::find(kFidgets.begin(), kFidgets.end(), animation) != kFidgets.end();
}

void IdleAnimator::update(AnimationPlayer& player, const std::vector<formats::Animation>& animations) {
    if (!isIdleAnimation(player.animation())) {
        player.play(kStandingAnimation, AnimationPlayer::Repeat::Loop);
        return;
    }
    if (!player.completedCycle()) {
        return;
    }
    if (player.animation() != kStandingAnimation) {
        player.play(kStandingAnimation, AnimationPlayer::Repeat::Loop);  // a fidget ended
        return;
    }
    // mt19937 gives the same numbers on every platform; the "% n" keeps it
    // that way (the standard distributions are not portable).
    if (random_() % 100 >= kFidgetChancePercent) {
        return;
    }
    const std::size_t fidget = kFidgets[random_() % kFidgets.size()];
    if (fidget < animations.size()) {
        player.play(fidget, AnimationPlayer::Repeat::Once);
    }
}

} // namespace openhogs::game
