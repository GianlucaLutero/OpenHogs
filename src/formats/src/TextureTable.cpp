// TextureTable.cpp - see TextureTable.h for an overview.

#include "openhogs/formats/TextureTable.h"

#include "openhogs/core/BinaryReader.h"

#include <string>

namespace openhogs::formats {

std::vector<std::span<const std::byte>> splitTextureTable(std::span<const std::byte> data) {
    core::BinaryReader reader(data);
    const std::uint32_t count = reader.readU32();

    std::vector<std::span<const std::byte>> textures;
    if (count == 0) {
        return textures;
    }

    // There is no size field: every image has the same size, so the size
    // is the remaining data divided by the count.
    const std::size_t imagesBytes = reader.remaining();
    if (imagesBytes % count != 0) {
        throw core::FormatError("PTG: " + std::to_string(imagesBytes) +
                                " bytes cannot be split into " + std::to_string(count) +
                                " images of equal size");
    }
    const std::size_t imageSize = imagesBytes / count;

    textures.reserve(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        textures.push_back(data.subspan(reader.position() + i * imageSize, imageSize));
    }
    return textures;
}

} // namespace openhogs::formats
