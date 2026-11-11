// CharacterPose.cpp - see CharacterPose.h for an overview.

#include "openhogs/scene/CharacterPose.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

namespace openhogs::scene {

namespace {

// A bone's local transform, kept as separate rotation and translation so
// two of them can be blended (blending whole matrices would shear them).
struct LocalTransform {
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 translation{0.0f};
};

glm::vec3 boneOffset(const formats::Skeleton& skeleton, std::size_t bone, const formats::AnimationFrame& frame) {
    if (bone == 0) {
        // The root moves with the animation; its offset in the skeleton is 0.
        return {static_cast<float>(frame.rootPosition[0]), static_cast<float>(frame.rootPosition[1]),
                static_cast<float>(frame.rootPosition[2])};
    }
    const auto& b = skeleton.bones[bone];
    return {static_cast<float>(b.x), static_cast<float>(b.y), static_cast<float>(b.z)};
}

std::vector<LocalTransform> localTransforms(const formats::Skeleton& skeleton, const formats::AnimationFrame& frame) {
    if (frame.bones.size() != skeleton.bones.size()) {
        throw std::invalid_argument("Animation frame has " + std::to_string(frame.bones.size()) +
                                    " bone rotations, the skeleton has " + std::to_string(skeleton.bones.size()) +
                                    " bones");
    }
    std::vector<LocalTransform> locals(skeleton.bones.size());
    for (std::size_t bone = 0; bone < locals.size(); ++bone) {
        locals[bone].rotation = glm::quat_cast(glm::mat3(boneRotation(frame.bones[bone])));
        locals[bone].translation = boneOffset(skeleton, bone, frame);
    }
    return locals;
}

// Chains the local transforms down the hierarchy. The skeleton parser
// guarantees every parent comes before its children.
Pose chain(const formats::Skeleton& skeleton, const std::vector<LocalTransform>& locals) {
    Pose pose(locals.size());
    for (std::size_t bone = 0; bone < locals.size(); ++bone) {
        const glm::mat4 local =
            glm::translate(glm::mat4(1.0f), locals[bone].translation) * glm::mat4_cast(locals[bone].rotation);
        pose[bone] = bone == 0 ? local : pose[skeleton.bones[bone].parent] * local;
    }
    return pose;
}

} // namespace

glm::mat4 boneRotation(const formats::BoneRotation& angles) {
    // The original engine multiplies row vectors by Rz * Ry * Rx (DirectX
    // style). For the column vectors used here that is the transpose of
    // the same matrix, i.e. Rx(-x) * Ry(-y) * Rz(-z).
    const glm::mat4 identity(1.0f);
    const glm::mat4 rowVectorMatrix = glm::rotate(identity, angles.z, glm::vec3(0, 0, 1)) *
                                      glm::rotate(identity, angles.y, glm::vec3(0, 1, 0)) *
                                      glm::rotate(identity, angles.x, glm::vec3(1, 0, 0));
    return glm::transpose(rowVectorMatrix);
}

Pose computePose(const formats::Skeleton& skeleton, const formats::AnimationFrame& frame) {
    return chain(skeleton, localTransforms(skeleton, frame));
}

Pose samplePose(const formats::Skeleton& skeleton, const formats::Animation& animation, float frame) {
    if (animation.frames.empty()) {
        throw std::invalid_argument("Cannot sample an animation without frames");
    }
    const float frameCount = static_cast<float>(animation.frames.size());
    const float position = frame - frameCount * std::floor(frame / frameCount);  // wrap into [0, count)

    const auto first = std::min(static_cast<std::size_t>(position), animation.frames.size() - 1);
    const auto second = (first + 1) % animation.frames.size();
    const float blend = position - static_cast<float>(first);

    const auto a = localTransforms(skeleton, animation.frames[first]);
    const auto b = localTransforms(skeleton, animation.frames[second]);

    std::vector<LocalTransform> blended(a.size());
    for (std::size_t bone = 0; bone < a.size(); ++bone) {
        // glm::slerp takes the shortest path between the two rotations.
        blended[bone].rotation = glm::slerp(a[bone].rotation, b[bone].rotation, blend);
        blended[bone].translation = glm::mix(a[bone].translation, b[bone].translation, blend);
    }
    return chain(skeleton, blended);
}

Pose blendPoses(const Pose& from, const Pose& to, float amount) {
    if (from.size() != to.size()) {
        throw std::invalid_argument("Cannot blend poses with different bone counts");
    }
    // Bone transforms are a rotation plus a translation, so each part can
    // be blended on its own (blending the matrices would distort the bone).
    Pose blended(from.size());
    for (std::size_t bone = 0; bone < from.size(); ++bone) {
        const glm::quat rotation =
            glm::slerp(glm::quat_cast(glm::mat3(from[bone])), glm::quat_cast(glm::mat3(to[bone])), amount);
        const glm::vec3 translation = glm::mix(glm::vec3(from[bone][3]), glm::vec3(to[bone][3]), amount);
        blended[bone] = glm::translate(glm::mat4(1.0f), translation) * glm::mat4_cast(rotation);
    }
    return blended;
}

} // namespace openhogs::scene
