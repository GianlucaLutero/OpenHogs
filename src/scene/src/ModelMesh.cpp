// ModelMesh.cpp - see ModelMesh.h for an overview.

#include "openhogs/scene/ModelMesh.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>

namespace openhogs::scene {

namespace {

// Direction towards the light for posed characters, in model space (where
// Y points down, so -Y is up): from above, slightly in front and to the side.
const glm::vec3 kLightDirection = glm::normalize(glm::vec3(0.4f, -1.0f, 0.3f));

// Shaded faces never go fully dark: the game's lighting is soft.
constexpr float kAmbientLight = 0.55f;

// Turns one face corner into a vertex. `place` gives the final position
// and brightness of the corner; it is where posing happens.
using PlaceCorner = std::function<void(const formats::FaceCorner&, Vertex&)>;

Vertex makeVertex(const formats::FaceCorner& corner, glm::vec2 textureSize, const PlaceCorner& place) {
    Vertex vertex;
    // +0.5 samples the centre of the texel the coordinate points at,
    // which avoids bleeding from the neighbouring texel at the edges.
    vertex.uv = {(static_cast<float>(corner.u) + 0.5f) / textureSize.x,
                 (static_cast<float>(corner.v) + 0.5f) / textureSize.y};
    place(corner, vertex);
    return vertex;
}

Mesh buildMesh(const formats::Model& model, const std::vector<glm::uvec2>& textureSizes, const PlaceCorner& place) {
    std::map<std::uint32_t, std::vector<Vertex>> trianglesByTexture;

    for (const auto& face : model.faces) {
        const glm::vec2 size = face.textureIndex < textureSizes.size()
                                   ? glm::vec2(textureSizes[face.textureIndex])
                                   : glm::vec2(1.0f);

        auto& out = trianglesByTexture[face.textureIndex];
        const auto& c = face.corners;
        out.push_back(makeVertex(c[0], size, place));
        out.push_back(makeVertex(c[1], size, place));
        out.push_back(makeVertex(c[2], size, place));
        if (c.size() == 4) {
            // Quad corners go around the edge: second triangle is 0-2-3.
            out.push_back(makeVertex(c[0], size, place));
            out.push_back(makeVertex(c[2], size, place));
            out.push_back(makeVertex(c[3], size, place));
        }
    }

    Mesh mesh;
    for (auto& [texture, vertices] : trianglesByTexture) {
        mesh.push_back({texture, std::move(vertices)});
    }
    return mesh;
}

glm::vec3 positionOf(const formats::ModelVertex& v) {
    return {static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z)};
}

} // namespace

Mesh buildModelMesh(const formats::Model& model, const std::vector<glm::uvec2>& textureSizes) {
    return buildMesh(model, textureSizes, [&](const formats::FaceCorner& corner, Vertex& vertex) {
        vertex.position = positionOf(model.vertices[corner.vertex]);
        vertex.brightness = 1.0f;
    });
}

Mesh buildPosedModelMesh(const formats::Model& model, const std::vector<glm::uvec2>& textureSizes, const Pose& pose) {
    for (const auto& v : model.vertices) {
        if (v.bone >= pose.size()) {
            throw std::invalid_argument("Model vertex uses bone " + std::to_string(v.bone) + ", the pose has only " +
                                        std::to_string(pose.size()) + " bones");
        }
    }

    return buildMesh(model, textureSizes, [&](const formats::FaceCorner& corner, Vertex& vertex) {
        const auto& source = model.vertices[corner.vertex];
        const glm::mat4& bone = pose[source.bone];
        vertex.position = glm::vec3(bone * glm::vec4(positionOf(source), 1.0f));

        vertex.brightness = 1.0f;
        if (corner.normal < model.normals.size()) {
            const auto& n = model.normals[corner.normal];
            // Bone transforms are rotations plus translations, so the
            // rotation part moves normals correctly.
            const glm::vec3 normal = glm::normalize(glm::mat3(bone) * glm::vec3(n.x, n.y, n.z));
            vertex.brightness = kAmbientLight + (1.0f - kAmbientLight) * std::max(0.0f, glm::dot(normal, kLightDirection));
        }
    });
}

Mesh buildCarriedModelMesh(const formats::Model& model, const std::vector<glm::uvec2>& textureSizes, const Pose& pose,
                           std::size_t bone) {
    if (bone >= pose.size()) {
        throw std::invalid_argument("Cannot carry an object on bone " + std::to_string(bone) + ", the pose has only " +
                                    std::to_string(pose.size()) + " bones");
    }
    // Half a turn around Y: X and Z change sign. Found by comparing the
    // weapons with the original's poses: without it every gun points
    // backwards and the jetpack's rocket sits on the hog's chest.
    const glm::mat4 halfTurn = glm::scale(glm::mat4(1.0f), glm::vec3(-1.0f, 1.0f, -1.0f));
    Pose carried(1, pose[bone] * halfTurn);
    formats::Model rigid = model;
    for (auto& vertex : rigid.vertices) vertex.bone = 0;
    return buildPosedModelMesh(rigid, textureSizes, carried);
}

} // namespace openhogs::scene
