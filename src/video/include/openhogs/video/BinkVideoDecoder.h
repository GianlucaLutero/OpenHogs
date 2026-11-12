// BinkVideoDecoder.h
//
// Decodes the pictures of a Bink 1 video (revisions 'c' to 'i'; the game's
// videos are 'f'). A picture is three planes (brightness at full size, two
// colour planes at half size, YUV 4:2:0) cut into 8x8 blocks; each block
// is copied from the previous picture (possibly moved and corrected),
// filled with one or two colours, run-length coded, raw, or DCT coded.
// The values of each kind come in "bundles", Huffman coded. See
// docs/formats/bink.md.
//
// The decoding follows FFmpeg's libavcodec/bink.c and binkdsp.c (LGPL 2.1
// or later), written anew for OpenHogs; see the notice in the .cpp file.

#pragma once

#include "openhogs/formats/TimImage.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace openhogs::video {

class BitReader;

// One plane of a picture, with a margin of one block on the right and at
// the bottom so 16x16 blocks at the edge stay inside it.
struct BinkPlane {
    int width = 0;   // visible size
    int height = 0;
    int stride = 0;  // bytes per row
    std::vector<std::uint8_t> pixels;
};

class BinkVideoDecoder {
public:
    // Throws core::FormatError for revision 'b' or videos with alpha.
    BinkVideoDecoder(int width, int height, char revision, std::uint32_t flags);

    // Decodes the next picture. Throws core::FormatError on damaged data.
    void decode(std::span<const std::byte> packet);

    // The last decoded picture: Y, U, V.
    const BinkPlane& plane(int index) const { return current_[static_cast<std::size_t>(index)]; }

    // The last decoded picture in RGBA (the video's colours are in the
    // limited "TV" range of BT.601).
    formats::RgbaImage toRgba() const;

private:
    // A Huffman tree choice and the order of its 16 symbols.
    struct Tree {
        int number = 0;
        std::array<std::uint8_t, 16> symbols{};
    };

    // One kind of value of a plane, decoded as needed.
    struct Bundle {
        int lengthBits = 0;            // bits giving the count of values to decode next
        Tree tree;
        std::vector<std::int16_t> values;
        std::size_t decoded = 0;       // values decoded so far
        std::size_t read = 0;          // values used so far
        bool finished = false;         // a count of 0 was read: nothing more this plane
    };

    void decodePlane(BitReader& bits, int planeIndex, bool chroma);
    void readTree(BitReader& bits, Tree& tree) const;
    int readSymbol(BitReader& bits, const Tree& tree) const;
    bool needsData(Bundle& bundle, BitReader& bits, int& count) const;

    void readBlockTypes(BitReader& bits, Bundle& bundle);
    void readColours(BitReader& bits, Bundle& bundle);
    void readPatterns(BitReader& bits, Bundle& bundle);
    void readMotion(BitReader& bits, Bundle& bundle);
    void readDcValues(BitReader& bits, Bundle& bundle, bool hasSign);
    void readRuns(BitReader& bits, Bundle& bundle);
    int next(int bundle);

    int readDctCoefficients(BitReader& bits, std::array<std::int32_t, 64>& block, std::array<int, 64>& indices,
                            int& count) const;
    void readResidue(BitReader& bits, std::array<std::int16_t, 64>& block, int masksCount) const;

    int width_;
    int height_;
    char revision_;
    std::array<BinkPlane, 3> current_;
    std::array<BinkPlane, 3> previous_;
    std::array<Bundle, 9> bundles_;
    std::array<Tree, 16> colourHighTrees_;
    int lastColourHigh_ = 0;

    // Lookup tables for the 16 Huffman trees: (symbol index, length) for
    // every value of the next kTreeLookupBits bits.
    struct TreeLookup {
        int bits = 0;
        std::vector<std::uint8_t> symbol;
        std::vector<std::uint8_t> length;
    };
    std::array<TreeLookup, 16> trees_;
};

} // namespace openhogs::video
