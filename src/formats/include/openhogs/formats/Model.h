// Model.h
//
// Reader for the 3D models of the PC version. A model is made of three
// files with the same name, usually stored together in a .MAD archive
// (see docs/formats/models.md):
//
//     NAME.VTX   vertex positions (16-bit integers)
//     NAME.NO2   vertex normals   (32-bit floats)       - optional
//     NAME.FAC   faces: triangles and quads, with texture coordinates
//                and the index of the texture to use
//
// Positions are in model units, which are twice as large as world units
// (see kModelToWorldScale). The texture index refers to the entries of the
// .MTD archive that comes with the map or character.

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace openhogs::formats {

// Models are stored at twice the scale of the terrain: multiplying model
// coordinates by this factor gives world coordinates. Verified on the map
// data: with this factor, crates rest exactly on the ground.
inline constexpr float kModelToWorldScale = 0.5f;

struct ModelVertex {
    std::int16_t x = 0;
    std::int16_t y = 0;
    std::int16_t z = 0;
    std::uint16_t bone = 0;  // skeleton bone the vertex follows (characters only)
};

struct ModelNormal {
    float x = 0;
    float y = 0;
    float z = 0;
};

// One corner of a face.
struct FaceCorner {
    std::uint16_t vertex = 0;  // index into Model::vertices
    std::uint16_t normal = 0;  // index into Model::normals
    std::uint8_t u = 0;        // texture coordinates, in texels of the texture
    std::uint8_t v = 0;
};

// A triangle (3 corners) or a quad (4 corners, listed in order around its
// edge, so it splits into the triangles 0-1-2 and 0-2-3).
struct ModelFace {
    std::vector<FaceCorner> corners;
    std::uint32_t textureIndex = 0;  // index into the accompanying .MTD archive
};

struct Model {
    std::vector<ModelVertex> vertices;
    std::vector<ModelNormal> normals;  // empty when no .NO2 file was given
    std::vector<ModelFace> faces;

    // Builds a model from the bytes of its files. Throws core::FormatError
    // if a file is malformed or a face refers to a vertex or normal that
    // does not exist.
    static Model parse(std::span<const std::byte> vtx, std::span<const std::byte> fac,
                       std::optional<std::span<const std::byte>> no2 = std::nullopt);
};

// The three readers used by Model::parse, exposed for testing.
std::vector<ModelVertex> parseVtx(std::span<const std::byte> data);
std::vector<ModelNormal> parseNo2(std::span<const std::byte> data);
std::vector<ModelFace> parseFac(std::span<const std::byte> data);

} // namespace openhogs::formats
