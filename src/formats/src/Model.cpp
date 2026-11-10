// Model.cpp - see Model.h for an overview.

#include "openhogs/formats/Model.h"

#include "openhogs/core/BinaryReader.h"

#include <bit>
#include <string>

namespace openhogs::formats {

namespace {

constexpr std::size_t kVtxRecordSize = 8;   // 3 x int16 position + uint16 bone
constexpr std::size_t kNo2RecordSize = 16;  // 3 x float normal + float (bone, unused)
constexpr std::size_t kFacHeaderSize = 16;  // unused, always zero in the game data

void requireMultipleOf(std::span<const std::byte> data, std::size_t recordSize, const char* format) {
    if (data.size() % recordSize != 0) {
        throw core::FormatError(std::string(format) + ": size " + std::to_string(data.size()) +
                                " is not a multiple of " + std::to_string(recordSize) + " bytes");
    }
}

float readFloat(core::BinaryReader& reader) {
    // IEEE 754 single precision, stored little-endian like everything else.
    return std::bit_cast<float>(reader.readU32());
}

// Reads one face of `cornerCount` corners. The on-disk layout groups the
// fields by kind: all texture coordinates, then all vertex indices, then all
// normal indices.
ModelFace readFace(core::BinaryReader& reader, std::size_t cornerCount, bool hasPaddingField) {
    ModelFace face;
    face.corners.resize(cornerCount);

    for (auto& corner : face.corners) {
        corner.u = reader.readU8();
        corner.v = reader.readU8();
    }
    for (auto& corner : face.corners) {
        corner.vertex = reader.readU16();
    }
    for (auto& corner : face.corners) {
        corner.normal = reader.readU16();
    }
    if (hasPaddingField) {
        reader.skip(2);  // triangles only: an unused 16-bit field
    }
    face.textureIndex = reader.readU32();
    reader.skip(8);  // four 16-bit fields whose meaning is unknown
    return face;
}

} // namespace

std::vector<ModelVertex> parseVtx(std::span<const std::byte> data) {
    requireMultipleOf(data, kVtxRecordSize, "VTX");

    std::vector<ModelVertex> vertices(data.size() / kVtxRecordSize);
    core::BinaryReader reader(data);
    for (auto& vertex : vertices) {
        vertex.x = reader.readI16();
        vertex.y = reader.readI16();
        vertex.z = reader.readI16();
        vertex.bone = reader.readU16();
    }
    return vertices;
}

std::vector<ModelNormal> parseNo2(std::span<const std::byte> data) {
    requireMultipleOf(data, kNo2RecordSize, "NO2");

    std::vector<ModelNormal> normals(data.size() / kNo2RecordSize);
    core::BinaryReader reader(data);
    for (auto& normal : normals) {
        normal.x = readFloat(reader);
        normal.y = readFloat(reader);
        normal.z = readFloat(reader);
        reader.skip(4);  // bone index stored as a float, duplicated from VTX
    }
    return normals;
}

std::vector<ModelFace> parseFac(std::span<const std::byte> data) {
    core::BinaryReader reader(data);
    reader.skip(kFacHeaderSize);

    std::vector<ModelFace> faces;

    // Triangles (32 bytes each), then quads (36 bytes each).
    const std::uint32_t triangleCount = reader.readU32();
    for (std::uint32_t i = 0; i < triangleCount; ++i) {
        faces.push_back(readFace(reader, 3, /*hasPaddingField=*/true));
    }
    const std::uint32_t quadCount = reader.readU32();
    for (std::uint32_t i = 0; i < quadCount; ++i) {
        faces.push_back(readFace(reader, 4, /*hasPaddingField=*/false));
    }
    return faces;
}

Model Model::parse(std::span<const std::byte> vtx, std::span<const std::byte> fac,
                   std::optional<std::span<const std::byte>> no2) {
    Model model;
    model.vertices = parseVtx(vtx);
    model.faces = parseFac(fac);
    if (no2) {
        model.normals = parseNo2(*no2);
    }

    // Check every index once here, so renderers can trust the model.
    for (std::size_t f = 0; f < model.faces.size(); ++f) {
        for (const FaceCorner& corner : model.faces[f].corners) {
            if (corner.vertex >= model.vertices.size()) {
                throw core::FormatError("FAC face " + std::to_string(f) + " uses vertex " +
                                        std::to_string(corner.vertex) + " but the model has only " +
                                        std::to_string(model.vertices.size()));
            }
            if (!model.normals.empty() && corner.normal >= model.normals.size()) {
                throw core::FormatError("FAC face " + std::to_string(f) + " uses normal " +
                                        std::to_string(corner.normal) + " but the model has only " +
                                        std::to_string(model.normals.size()));
            }
        }
    }
    return model;
}

} // namespace openhogs::formats
