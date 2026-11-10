// TextureTable.h
//
// Reader for .PTG files: the terrain textures of a map.
//
// A PTG file is a 32-bit count followed by that many TIM images, all with
// the same size in bytes (in the game data: 32 x 32, 4-bit, 576 bytes each).
// Terrain tiles refer to these images by index.

#pragma once

#include <cstddef>
#include <span>
#include <vector>

namespace openhogs::formats {

// Returns one span per TIM image, pointing into `data` (which must outlive
// the result). Throws core::FormatError if the sizes do not add up.
std::vector<std::span<const std::byte>> splitTextureTable(std::span<const std::byte> data);

} // namespace openhogs::formats
