// ModelMesh.cpp - see ModelMesh.h for an overview.

#include "openhogs/scene/ModelMesh.h"

#include <map>
#include <utility>

namespace openhogs::scene {

namespace {

Vertex makeVertex(const formats::Model& model, const formats::FaceCorner& corner, glm::vec2 textureSize) {
    const auto& p = model.vertices[corner.vertex];
    Vertex vertex;
    vertex.position = {p.x, p.y, p.z};
    // +0.5 samples the centre of the texel the coordinate points at,
    // which avoids bleeding from the neighbouring texel at the edges.
    vertex.uv = {(static_cast<float>(corner.u) + 0.5f) / textureSize.x,
                 (static_cast<float>(corner.v) + 0.5f) / textureSize.y};
    vertex.brightness = 1.0f;
    return vertex;
}

} // namespace

Mesh buildModelMesh(const formats::Model& model, const std::vector<glm::uvec2>& textureSizes) {
    std::map<std::uint32_t, std::vector<Vertex>> trianglesByTexture;

    for (const auto& face : model.faces) {
        const glm::vec2 size = face.textureIndex < textureSizes.size()
                                   ? glm::vec2(textureSizes[face.textureIndex])
                                   : glm::vec2(1.0f);

        auto& out = trianglesByTexture[face.textureIndex];
        const auto& c = face.corners;
        out.push_back(makeVertex(model, c[0], size));
        out.push_back(makeVertex(model, c[1], size));
        out.push_back(makeVertex(model, c[2], size));
        if (c.size() == 4) {
            // Quad corners go around the edge: second triangle is 0-2-3.
            out.push_back(makeVertex(model, c[0], size));
            out.push_back(makeVertex(model, c[2], size));
            out.push_back(makeVertex(model, c[3], size));
        }
    }

    Mesh mesh;
    for (auto& [texture, vertices] : trianglesByTexture) {
        mesh.push_back({texture, std::move(vertices)});
    }
    return mesh;
}

} // namespace openhogs::scene
