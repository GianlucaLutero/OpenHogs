// Geometry.h
//
// Basic types shared by the mesh builders and the renderer.
//
// Meshes are plain lists of triangles (three vertices per triangle, no
// index buffer) grouped by texture: the renderer binds one texture and
// draws one group at a time. The game's models are small, so the simplicity
// is worth more than the memory an index buffer would save.

#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace openhogs::scene {

struct Vertex {
    glm::vec3 position{0.0f};  // world (terrain) or model units
    glm::vec2 uv{0.0f};        // texture coordinates, 0..1
    float brightness = 1.0f;   // multiplies the texture colour (baked lighting)
};

// Triangles that share one texture.
struct MeshPart {
    std::uint32_t textureIndex = 0;
    std::vector<Vertex> vertices;  // 3 per triangle
};

using Mesh = std::vector<MeshPart>;

} // namespace openhogs::scene
