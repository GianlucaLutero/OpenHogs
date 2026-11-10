// Tests for formats::Model (.VTX / .NO2 / .FAC).
//
// A misread face shows up as exploded or wrongly textured geometry for
// every object of the game, so these tests pin down the field layout.

#include "openhogs/formats/Model.h"

#include "ByteBuilder.h"
#include "openhogs/core/BinaryReader.h"  // FormatError

#include <catch2/catch_test_macros.hpp>

#include <bit>

using namespace openhogs::formats;
using openhogs::core::FormatError;
using openhogs::tests::ByteBuilder;

namespace {

// Four vertices of a unit square (model units).
ByteBuilder squareVtx() {
    ByteBuilder vtx;
    vtx.u16(0).u16(0).u16(0).u16(0);
    vtx.u16(10).u16(0).u16(0).u16(0);
    vtx.u16(10).u16(0).u16(10).u16(0);
    vtx.u16(0).u16(0).u16(10).u16(0);
    return vtx;
}

// FAC with one triangle (vertices 0,1,2) and one quad (vertices 0,1,2,3).
ByteBuilder triangleAndQuadFac() {
    ByteBuilder fac;
    fac.fixedString("", 16);  // unused header
    fac.u32(1);               // one triangle
    fac.u8(1).u8(2).u8(3).u8(4).u8(5).u8(6);  // u,v per corner
    fac.u16(0).u16(1).u16(2);                 // vertices
    fac.u16(0).u16(0).u16(0);                 // normals
    fac.u16(0);                               // padding (triangles only)
    fac.u32(7);                               // texture index
    fac.u16(0).u16(0).u16(0).u16(0);          // unknown
    fac.u32(1);               // one quad
    fac.u8(10).u8(11).u8(12).u8(13).u8(14).u8(15).u8(16).u8(17);
    fac.u16(0).u16(1).u16(2).u16(3);
    fac.u16(0).u16(0).u16(0).u16(0);
    fac.u32(9);
    fac.u16(0).u16(0).u16(0).u16(0);
    return fac;
}

} // namespace

// FAC groups fields by kind (all UVs, then all vertex indices...) and
// triangles carry a padding field that quads do not have. Reading one
// field in the wrong place would misalign every following face.
TEST_CASE("Model reads triangles and quads with their texture data", "[formats][Model]") {
    const auto model = Model::parse(squareVtx().bytes(), triangleAndQuadFac().bytes());

    REQUIRE(model.vertices.size() == 4);
    CHECK(model.vertices[2].x == 10);
    CHECK(model.vertices[2].z == 10);

    REQUIRE(model.faces.size() == 2);
    const auto& triangle = model.faces[0];
    REQUIRE(triangle.corners.size() == 3);
    CHECK(triangle.corners[2].vertex == 2);
    CHECK(triangle.corners[1].u == 3);
    CHECK(triangle.corners[1].v == 4);
    CHECK(triangle.textureIndex == 7);

    const auto& quad = model.faces[1];
    REQUIRE(quad.corners.size() == 4);
    CHECK(quad.corners[3].vertex == 3);
    CHECK(quad.corners[3].u == 16);
    CHECK(quad.textureIndex == 9);
}

// Normals are optional on disk but, when present, are 32-bit floats with a
// fourth unused value per record.
TEST_CASE("Model reads optional normals", "[formats][Model]") {
    ByteBuilder no2;
    for (int i = 0; i < 4; ++i) {
        no2.u32(std::bit_cast<std::uint32_t>(0.0f));
        no2.u32(std::bit_cast<std::uint32_t>(1.0f));
        no2.u32(std::bit_cast<std::uint32_t>(0.0f));
        no2.u32(0);
    }
    const auto model = Model::parse(squareVtx().bytes(), triangleAndQuadFac().bytes(), no2.bytes());

    REQUIRE(model.normals.size() == 4);
    CHECK(model.normals[0].y == 1.0f);
}

// A face pointing at a vertex that does not exist would make the renderer
// read outside its buffers; it must be rejected when the model is loaded.
TEST_CASE("Model rejects faces that use missing vertices", "[formats][Model]") {
    ByteBuilder twoVertices;
    twoVertices.u16(0).u16(0).u16(0).u16(0).u16(1).u16(1).u16(1).u16(0);

    CHECK_THROWS_AS(Model::parse(twoVertices.bytes(), triangleAndQuadFac().bytes()), FormatError);
}
