// AnimationPlayer.cpp - see AnimationPlayer.h for an overview.

#include "openhogs/game/AnimationPlayer.h"

#include "openhogs/scene/CharacterLibrary.h"

#include <algorithm>

namespace openhogs::game {

namespace {

// Long enough to hide the change of stance, short enough to keep the
// controls responsive.
constexpr float kCrossFadeSeconds = 0.2f;

// Animations of a replaced mcap file may be fewer than the original's.
std::size_t existingOrFirst(std::size_t animation, const std::vector<formats::Animation>& animations) {
    return animation < animations.size() ? animation : 0;
}

} // namespace

void AnimationPlayer::play(std::size_t animation, Repeat repeat, bool restart) {
    if (animation == animation_ && repeat == repeat_ && !restart) {
        return;
    }
    fadeAnimation_ = animation_;
    fadeFrame_ = frame_;
    fadeSecondsLeft_ = kCrossFadeSeconds;

    animation_ = animation;
    repeat_ = repeat;
    frame_ = 0.0f;
    previousFrame_ = 0.0f;
    finished_ = false;
    completedCycle_ = false;
    wrapped_ = false;
}

void AnimationPlayer::hold(std::size_t animation, float frame) {
    play(animation, Repeat::Hold);
    frame_ = std::max(0.0f, frame);
    previousFrame_ = frame_;
}

void AnimationPlayer::update(double seconds, const std::vector<formats::Animation>& animations) {
    completedCycle_ = false;
    wrapped_ = false;
    previousFrame_ = frame_;
    if (animations.empty()) {
        return;
    }
    fadeSecondsLeft_ = std::max(0.0f, fadeSecondsLeft_ - static_cast<float>(seconds));
    if (finished_) {
        return;
    }

    const auto& current = animations[existingOrFirst(animation_, animations)];
    const auto lastFrame = static_cast<float>(current.frames.size() - 1);
    if (repeat_ == Repeat::Hold) {
        frame_ = std::min(frame_, lastFrame);
        return;
    }
    frame_ += static_cast<float>(seconds) * scene::kAnimationFramesPerSecond;

    if (repeat_ == Repeat::Once) {
        if (frame_ >= lastFrame) {
            frame_ = lastFrame;
            finished_ = true;
            completedCycle_ = true;
        }
        return;
    }

    // Looping: frame N is frame 0 again.
    const float frameCount = lastFrame + 1.0f;
    if (frame_ >= frameCount) {
        frame_ -= frameCount * static_cast<float>(static_cast<int>(frame_ / frameCount));
        completedCycle_ = true;
        wrapped_ = true;
    }
}

bool AnimationPlayer::passedFrame(float frame) const {
    if (wrapped_) {
        return frame > previousFrame_ || frame <= frame_;
    }
    return frame > previousFrame_ && frame <= frame_;
}

scene::Pose AnimationPlayer::pose(const formats::Skeleton& skeleton,
                                  const std::vector<formats::Animation>& animations) const {
    const auto& current = animations.at(existingOrFirst(animation_, animations));
    scene::Pose now = scene::samplePose(skeleton, current, frame_);
    if (fadeSecondsLeft_ <= 0.0f) {
        return now;
    }
    const auto& previous = animations.at(existingOrFirst(fadeAnimation_, animations));
    const scene::Pose before = scene::samplePose(skeleton, previous, fadeFrame_);
    return scene::blendPoses(before, now, 1.0f - fadeSecondsLeft_ / kCrossFadeSeconds);
}

} // namespace openhogs::game
