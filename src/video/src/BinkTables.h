// BinkTables.h - constant tables of the Bink video codec (private to the
// video module). The data comes from FFmpeg (see BinkTables.cpp for its
// license, LGPL 2.1 or later).

#pragma once

#include <cstdint>

namespace openhogs::video::bink {

// Order in which the 64 coefficients of an 8x8 DCT or residue block are
// stored (index in the stream -> position in the block).
extern const std::uint8_t kScan[64];

// The 16 Huffman trees the bundles choose from: for each tree, the code
// of each of its 16 leaves, least significant bit first, and its length.
extern const std::uint8_t kTreeCodes[16][16];
extern const std::uint8_t kTreeLengths[16][16];

// The 16 orders in which "run" blocks visit their 64 pixels.
extern const std::uint8_t kRunPatterns[16][64];

// Dequantisation factors (fixed point, 11 fractional bits) for the 16
// quality levels of intra and inter DCT blocks.
extern const std::int32_t kIntraQuant[16][64];
extern const std::int32_t kInterQuant[16][64];

} // namespace openhogs::video::bink
