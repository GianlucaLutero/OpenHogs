// IdleAnimator.cpp - see IdleAnimator.h for an overview.

#include "openhogs/game/IdleAnimator.h"

#include "openhogs/scene/CharacterLibrary.h"

#include <algorithm>
#include <array>

namespace openhogs::game {

namespace {

// Short animations that start and end in the standing stance (measured on
// the game data: their first and last frames are a few units away from the
// standing cycle's): standing cycle 2, looking around, three "brush off".
constexpr std::array<std::size_t, 5> kFidgets{28, 31, 34, 35, 36};

// Chance, in percent, of a fidget each time the standing cycle ends (the
// cycle lasts 1.44 s, so about one fidget every 7 seconds).
constexpr unsigned kFidgetChancePercent = 20;

constexpr float kCrossFadeSeconds = 0.2f;

// Animations of a replaced mcap file may be fewer than the original's.
std::size_t existingOrFirst(std::size_t animation, const std::vector<formats::Animation>& animations) {
    return animation < animations.size() ? animation : 0;
}

} // namespace

IdleAnimator::IdleAnimator(std::uint32_t seed) : random_(seed) {
    // Hogs created together must not move in step: each starts at its own
    // point of the standing cycle (36 frames long in the original data).
    frame_ = static_cast<float>(random_() % 36);
}

void IdleAnimator::startAnimation(std::size_t animation) {
    previousAnimation_ = animation_;
    previousFrame_ = frame_;
    fadeSecondsLeft_ = kCrossFadeSeconds;
    animation_ = animation;
    frame_ = 0.0f;
}

std::size_t IdleAnimator::chooseNext(const std::vector<formats::Animation>& animations) {
    if (animation_ != kStandingAnimation) {
        return kStandingAnimation;  // a fidget is always followed by standing
    }
    // mt19937 gives the same numbers on every platform; the "% n" keeps it
    // that way (the standard distributions are not portable).
    if (random_() % 100 >= kFidgetChancePercent) {
        return kStandingAnimation;
    }
    const std::size_t fidget = kFidgets[random_() % kFidgets.size()];
    return fidget < animations.size() ? fidget : kStandingAnimation;
}

void IdleAnimator::update(double seconds, const std::vector<formats::Animation>& animations) {
    if (animations.empty()) {
        return;
    }
    fadeSecondsLeft_ = std::max(0.0f, fadeSecondsLeft_ - static_cast<float>(seconds));
    frame_ += static_cast<float>(seconds) * scene::kAnimationFramesPerSecond;

    const std::size_t current = existingOrFirst(animation_, animations);
    const auto frameCount = static_cast<float>(animations[current].frames.size());
    if (frame_ < frameCount) {
        return;
    }

    // The animation ended: either keep cycling it or change.
    const float overshoot = frame_ - frameCount;
    const std::size_t next = chooseNext(animations);
    if (next == animation_) {
        frame_ = overshoot;
    } else {
        frame_ = frameCount - 1.0f;  // the cross-fade starts from the last frame
        startAnimation(next);
        frame_ = overshoot;
    }
}

scene::Pose IdleAnimator::pose(const formats::Skeleton& skeleton,
                               const std::vector<formats::Animation>& animations) const {
    const auto& current = animations.at(existingOrFirst(animation_, animations));
    scene::Pose now = scene::samplePose(skeleton, current, frame_);
    if (fadeSecondsLeft_ <= 0.0f) {
        return now;
    }
    const auto& previous = animations.at(existingOrFirst(previousAnimation_, animations));
    const scene::Pose before = scene::samplePose(skeleton, previous, previousFrame_);
    return scene::blendPoses(before, now, 1.0f - fadeSecondsLeft_ / kCrossFadeSeconds);
}

} // namespace openhogs::game
