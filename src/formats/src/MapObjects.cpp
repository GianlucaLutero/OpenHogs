// MapObjects.cpp - see MapObjects.h for an overview.
//
// Record layout (94 bytes) based on the OpenHoW format documentation (see
// the Credits section of the README). The offsets from the team field on
// were corrected against the game data: in ICE.POG the team masks 01, 02,
// 04 and 08 appear at offset 59, five hogs each. See docs/formats/maps.md.

#include "openhogs/formats/MapObjects.h"

#include "openhogs/core/BinaryReader.h"

namespace openhogs::formats {

namespace {

constexpr std::size_t kRecordSize = 94;
constexpr std::size_t kNameLength = 16;

MapObject readObject(core::BinaryReader& reader) {
    const std::size_t start = reader.position();

    MapObject object;
    object.modelName = reader.readFixedString(kNameLength);
    reader.skip(kNameLength);  // second name field, always "NULL"

    object.x = reader.readI16();
    object.y = reader.readI16();
    object.z = reader.readI16();
    object.id = reader.readI16();
    object.angleX = reader.readI16();
    object.angleY = reader.readI16();
    object.angleZ = reader.readI16();
    object.typeId = reader.readI16();

    reader.skip(8);  // bounding box width, height, length and type
    reader.skip(3);  // unknown 16-bit + 8-bit fields
    object.team = reader.readU8();            // offset 59
    object.scriptType = reader.readI16();     // offset 60
    reader.skip(1 + 19 + 6);  // script group, parameters, script position
    object.flags = reader.readU16();          // offset 88

    // Jump to the next record whatever we read, so a misunderstanding of
    // one field can never shift the following objects.
    reader.seek(start + kRecordSize);
    return object;
}

} // namespace

std::vector<MapObject> parseMapObjects(std::span<const std::byte> data) {
    core::BinaryReader reader(data);
    const std::uint16_t count = reader.readU16();

    if (reader.remaining() < std::size_t{count} * kRecordSize) {
        throw core::FormatError("POG: file announces " + std::to_string(count) +
                                " objects but is too short to hold them");
    }

    // Some files end with a few extra bytes after the last record; they
    // are ignored.
    std::vector<MapObject> objects;
    objects.reserve(count);
    for (std::uint16_t i = 0; i < count; ++i) {
        objects.push_back(readObject(reader));
    }
    return objects;
}

} // namespace openhogs::formats
