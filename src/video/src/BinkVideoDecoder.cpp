// BinkVideoDecoder.cpp - see BinkVideoDecoder.h for an overview.
//
// The algorithm follows FFmpeg's libavcodec/bink.c and binkdsp.c, whose
// notice is:
//
//   Bink video decoder
//   Copyright (c) 2009 Konstantin Shishkov
//   Copyright (C) 2011 Peter Ross <pross@xvid.org>
//
//   FFmpeg is free software; you can redistribute it and/or modify it
//   under the terms of the GNU Lesser General Public License as published
//   by the Free Software Foundation; either version 2.1 of the License, or
//   (at your option) any later version.
//
// This file is part of OpenHogs and distributed under the GNU GPL v3 (the
// LGPL allows that). Changes: written in C++ for OpenHogs, without the
// old revision 'b', alpha planes and the 'k' extensions.

#include "openhogs/video/BinkVideoDecoder.h"

#include "BinkTables.h"
#include "openhogs/core/BinaryReader.h"
#include "openhogs/video/BitReader.h"

#include <algorithm>
#include <bit>
#include <string>

namespace openhogs::video {

namespace {

// The bundles of a plane, in the order they are read.
enum BundleId {
    BlockTypes = 0,  // type of each 8x8 block
    SubBlockTypes,   // type of each 16x16 block (a subset of the 8x8 types)
    Colours,         // pixel values
    Patterns,        // 8-bit rows of two-colour patterns
    MotionX,         // horizontal motion
    MotionY,         // vertical motion
    IntraDc,         // first coefficient of intra DCT blocks
    InterDc,         // first coefficient of inter DCT blocks
    Runs,            // run lengths of run blocks
    BundleCount
};

enum BlockType {
    SkipBlock = 0,  // the same as in the previous picture
    ScaledBlock,    // a 16x16 block, coded as an 8x8 one of another type at half size
    MotionBlock,    // copied from the previous picture, moved
    RunBlock,       // runs of colours along one of 16 scan patterns
    ResidueBlock,   // moved copy with corrections
    IntraBlock,     // DCT coded
    FillBlock,      // one colour
    InterBlock,     // moved copy with DCT coded corrections
    PatternBlock,   // two colours along an 8x8 bit pattern
    RawBlock,       // 64 pixel values
};

constexpr int kTreeLookupBits = 7;  // the longest code of the 16 trees
constexpr int kDcStartBits = 11;
constexpr std::array<int, 4> kBlockTypeRuns{4, 8, 12, 32};

int floorLog2(unsigned value) { return std::bit_width(value) - 1; }

[[noreturn]] void damaged(const char* what) { throw core::FormatError(std::string("Bink video: ") + what); }

// --- Inverse DCT (binkdsp.c) -----------------------------------------------

constexpr int kA1 = 2896;  // (1 / sqrt(2)) << 12
constexpr int kA2 = 2217;
constexpr int kA3 = 3784;
constexpr int kA4 = -5352;

int multiply(int x, int y) { return static_cast<int>(static_cast<unsigned>(x) * static_cast<unsigned>(y)) >> 11; }

// One 8-point inverse transform: reads src[0], src[step], ... and writes
// eight values through `store`.
template <typename Load, typename Store>
void idct8(Load load, Store store) {
    const int a0 = load(0) + load(4);
    const int a1 = load(0) - load(4);
    const int a2 = load(2) + load(6);
    const int a3 = multiply(kA1, load(2) - load(6));
    const int a4 = load(5) + load(3);
    const int a5 = load(5) - load(3);
    const int a6 = load(1) + load(7);
    const int a7 = load(1) - load(7);
    const int b0 = a4 + a6;
    const int b1 = multiply(kA3, a5 + a7);
    const int b2 = multiply(kA4, a5) - b0 + b1;
    const int b3 = multiply(kA1, a6 - a4) - b2;
    const int b4 = multiply(kA2, a7) + b3 - b1;
    store(0, a0 + a2 + b0);
    store(1, a1 + a3 - a2 + b2);
    store(2, a1 - a3 + a2 + b3);
    store(3, a0 - a2 - b4);
    store(4, a0 - a2 + b4);
    store(5, a1 - a3 + a2 - b3);
    store(6, a1 + a3 - a2 - b2);
    store(7, a0 + a2 - b0);
}

// The 2D inverse DCT of `block`; result[i] for the 64 pixels, rounded.
std::array<int, 64> inverseDct(const std::array<std::int32_t, 64>& block) {
    std::array<int, 64> columns{};
    for (int x = 0; x < 8; ++x) {
        bool onlyDc = true;
        for (int y = 1; y < 8; ++y) onlyDc = onlyDc && block[static_cast<std::size_t>(x + 8 * y)] == 0;
        if (onlyDc) {
            for (int y = 0; y < 8; ++y) columns[static_cast<std::size_t>(x + 8 * y)] = block[static_cast<std::size_t>(x)];
            continue;
        }
        idct8([&](int i) { return block[static_cast<std::size_t>(x + 8 * i)]; },
              [&](int i, int v) { columns[static_cast<std::size_t>(x + 8 * i)] = v; });
    }
    std::array<int, 64> result{};
    for (int y = 0; y < 8; ++y) {
        idct8([&](int i) { return columns[static_cast<std::size_t>(8 * y + i)]; },
              [&](int i, int v) { result[static_cast<std::size_t>(8 * y + i)] = (v + 0x7F) >> 8; });
    }
    return result;
}

void dequantise(std::array<std::int32_t, 64>& block, const std::int32_t (&quant)[64], const std::array<int, 64>& indices,
                int count) {
    const auto scale = [](std::int32_t value, std::int32_t factor) {
        return static_cast<std::int32_t>(static_cast<std::uint32_t>(value) * static_cast<std::uint32_t>(factor)) >> 11;
    };
    block[0] = scale(block[0], quant[0]);
    for (int i = 0; i < count; ++i) {
        const int index = indices[static_cast<std::size_t>(i)];
        auto& value = block[bink::kScan[index]];
        value = scale(value, quant[index]);
    }
}

} // namespace

BinkVideoDecoder::BinkVideoDecoder(int width, int height, char revision, std::uint32_t flags)
    : width_(width), height_(height), revision_(revision) {
    constexpr std::uint32_t kAlphaFlag = 0x00100000;
    if (revision < 'c' || revision > 'i') damaged("only revisions c to i are supported");
    if ((flags & kAlphaFlag) != 0) damaged("videos with transparency are not supported");
    if (width <= 0 || height <= 0) damaged("invalid size");

    for (int i = 0; i < 3; ++i) {
        const bool chroma = i > 0;
        BinkPlane plane;
        plane.width = width >> (chroma ? 1 : 0);
        plane.height = height >> (chroma ? 1 : 0);
        const int blocksWide = chroma ? (width + 15) >> 4 : (width + 7) >> 3;
        const int blocksHigh = chroma ? (height + 15) >> 4 : (height + 7) >> 3;
        plane.stride = (blocksWide + 1) * 8;
        plane.pixels.assign(static_cast<std::size_t>(plane.stride * (blocksHigh + 1) * 8), chroma ? 128 : 16);
        current_[static_cast<std::size_t>(i)] = plane;
        previous_[static_cast<std::size_t>(i)] = plane;
    }
    const std::size_t blocks = static_cast<std::size_t>(((width + 7) >> 3) * ((height + 7) >> 3));
    for (auto& bundle : bundles_) bundle.values.resize(blocks * 64);

    // Lookup tables: every 7-bit value starting with a leaf's code.
    for (int t = 0; t < 16; ++t) {
        auto& lookup = trees_[static_cast<std::size_t>(t)];
        lookup.bits = kTreeLookupBits;
        lookup.symbol.assign(1u << kTreeLookupBits, 0);
        lookup.length.assign(1u << kTreeLookupBits, 0);
        for (int leaf = 0; leaf < 16; ++leaf) {
            const int length = bink::kTreeLengths[t][leaf];
            const int code = bink::kTreeCodes[t][leaf];
            for (int rest = 0; rest < (1 << (kTreeLookupBits - length)); ++rest) {
                const auto index = static_cast<std::size_t>(code | (rest << length));
                lookup.symbol[index] = static_cast<std::uint8_t>(leaf);
                lookup.length[index] = static_cast<std::uint8_t>(length);
            }
        }
    }
}

// --- Bundles -----------------------------------------------------------------

int BinkVideoDecoder::readSymbol(BitReader& bits, const Tree& tree) const {
    const auto& lookup = trees_[static_cast<std::size_t>(tree.number)];
    const auto index = bits.peek(lookup.bits);
    bits.skip(lookup.length[index]);
    return tree.symbols[lookup.symbol[index]];
}

// Merges two sorted halves of `source` into `target`, guided by bits.
static void mergeHalves(BitReader& bits, std::uint8_t* target, const std::uint8_t* source, int size) {
    const std::uint8_t* second = source + size;
    int left = size;
    int right = size;
    while (left > 0 && right > 0) {
        if (!bits.readBit()) {
            *target++ = *source++;
            --left;
        } else {
            *target++ = *second++;
            --right;
        }
    }
    while (left-- > 0) *target++ = *source++;
    while (right-- > 0) *target++ = *second++;
}

void BinkVideoDecoder::readTree(BitReader& bits, Tree& tree) const {
    tree.number = static_cast<int>(bits.read(4));
    if (tree.number == 0) {
        for (int i = 0; i < 16; ++i) tree.symbols[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(i);
        return;
    }
    if (bits.readBit()) {
        // The first symbols are listed; the others follow in order.
        std::array<bool, 16> listed{};
        int last = static_cast<int>(bits.read(3));
        for (int i = 0; i <= last; ++i) {
            const auto symbol = static_cast<std::uint8_t>(bits.read(4));
            tree.symbols[static_cast<std::size_t>(i)] = symbol;
            listed[symbol] = true;
        }
        for (int i = 0; i < 16 && last < 15; ++i) {
            if (!listed[static_cast<std::size_t>(i)]) tree.symbols[static_cast<std::size_t>(++last)] = static_cast<std::uint8_t>(i);
        }
    } else {
        // A permutation built by merging runs of 1, 2, 4, 8 symbols.
        const int passes = static_cast<int>(bits.read(2));
        std::array<std::uint8_t, 16> in{};
        std::array<std::uint8_t, 16> out{};
        for (int i = 0; i < 16; ++i) in[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(i);
        for (int pass = 0; pass <= passes; ++pass) {
            const int size = 1 << pass;
            for (int t = 0; t < 16; t += size << 1) {
                mergeHalves(bits, out.data() + t, in.data() + t, size);
            }
            std::swap(in, out);
        }
        tree.symbols = in;
    }
}

bool BinkVideoDecoder::needsData(Bundle& bundle, BitReader& bits, int& count) const {
    // New values are read only once the previous ones are used up.
    if (bundle.finished || bundle.decoded > bundle.read) return false;
    count = static_cast<int>(bits.read(bundle.lengthBits));
    if (count == 0) {
        bundle.finished = true;
        return false;
    }
    if (bundle.decoded + static_cast<std::size_t>(count) > bundle.values.size()) damaged("too many values");
    return true;
}

void BinkVideoDecoder::readBlockTypes(BitReader& bits, Bundle& bundle) {
    int count = 0;
    if (!needsData(bundle, bits, count)) return;
    const std::size_t end = bundle.decoded + static_cast<std::size_t>(count);
    if (bits.readBit()) {
        std::fill(bundle.values.begin() + static_cast<std::ptrdiff_t>(bundle.decoded),
                  bundle.values.begin() + static_cast<std::ptrdiff_t>(end), static_cast<std::int16_t>(bits.read(4)));
        bundle.decoded = end;
        return;
    }
    std::int16_t last = 0;
    while (bundle.decoded < end) {
        const int value = readSymbol(bits, bundle.tree);
        if (value < 12) {
            last = static_cast<std::int16_t>(value);
            bundle.values[bundle.decoded++] = last;
        } else {
            // 12 .. 15: repeat the last type 4, 8, 12 or 32 times.
            const auto run = static_cast<std::size_t>(kBlockTypeRuns[static_cast<std::size_t>(value - 12)]);
            if (end - bundle.decoded < run) damaged("block type run too long");
            for (std::size_t i = 0; i < run; ++i) bundle.values[bundle.decoded++] = last;
        }
    }
}

void BinkVideoDecoder::readColours(BitReader& bits, Bundle& bundle) {
    int count = 0;
    if (!needsData(bundle, bits, count)) return;
    // A colour is a high nibble (whose tree depends on the previous high
    // nibble) and a low nibble; before revision 'i' it is stored signed.
    const auto readColour = [&]() {
        lastColourHigh_ = readSymbol(bits, colourHighTrees_[static_cast<std::size_t>(lastColourHigh_)]);
        int value = (lastColourHigh_ << 4) | readSymbol(bits, bundle.tree);
        if (revision_ < 'i') {
            const int sign = (value & 0x80) ? -1 : 0;
            value = ((value & 0x7F) ^ sign) - sign;
            value += 0x80;
        }
        return static_cast<std::int16_t>(value & 0xFF);
    };
    const std::size_t end = bundle.decoded + static_cast<std::size_t>(count);
    if (bits.readBit()) {
        const auto value = readColour();
        while (bundle.decoded < end) bundle.values[bundle.decoded++] = value;
    } else {
        while (bundle.decoded < end) bundle.values[bundle.decoded++] = readColour();
    }
}

void BinkVideoDecoder::readPatterns(BitReader& bits, Bundle& bundle) {
    int count = 0;
    if (!needsData(bundle, bits, count)) return;
    for (int i = 0; i < count; ++i) {
        const int low = readSymbol(bits, bundle.tree);
        const int high = readSymbol(bits, bundle.tree);
        bundle.values[bundle.decoded++] = static_cast<std::int16_t>(low | (high << 4));
    }
}

void BinkVideoDecoder::readMotion(BitReader& bits, Bundle& bundle) {
    int count = 0;
    if (!needsData(bundle, bits, count)) return;
    const auto withSign = [&](int value) { return static_cast<std::int16_t>(value != 0 && bits.readBit() ? -value : value); };
    const std::size_t end = bundle.decoded + static_cast<std::size_t>(count);
    if (bits.readBit()) {
        const auto value = withSign(static_cast<int>(bits.read(4)));
        while (bundle.decoded < end) bundle.values[bundle.decoded++] = value;
    } else {
        while (bundle.decoded < end) bundle.values[bundle.decoded++] = withSign(readSymbol(bits, bundle.tree));
    }
}

void BinkVideoDecoder::readDcValues(BitReader& bits, Bundle& bundle, bool hasSign) {
    int count = 0;
    if (!needsData(bundle, bits, count)) return;
    // The first value in full, then groups of up to 8 differences that
    // share a bit width.
    int value = static_cast<int>(bits.read(kDcStartBits - (hasSign ? 1 : 0)));
    if (value != 0 && hasSign && bits.readBit()) value = -value;
    bundle.values[bundle.decoded++] = static_cast<std::int16_t>(value);
    --count;
    for (int i = 0; i < count; i += 8) {
        const int group = std::min(count - i, 8);
        const int width = static_cast<int>(bits.read(4));
        for (int j = 0; j < group; ++j) {
            if (width != 0) {
                int difference = static_cast<int>(bits.read(width));
                if (difference != 0 && bits.readBit()) difference = -difference;
                value += difference;
                if (value < -32768 || value > 32767) damaged("DC value out of range");
            }
            bundle.values[bundle.decoded++] = static_cast<std::int16_t>(value);
        }
    }
}

void BinkVideoDecoder::readRuns(BitReader& bits, Bundle& bundle) {
    int count = 0;
    if (!needsData(bundle, bits, count)) return;
    const std::size_t end = bundle.decoded + static_cast<std::size_t>(count);
    if (bits.readBit()) {
        const auto value = static_cast<std::int16_t>(bits.read(4));
        while (bundle.decoded < end) bundle.values[bundle.decoded++] = value;
    } else {
        while (bundle.decoded < end) bundle.values[bundle.decoded++] = static_cast<std::int16_t>(readSymbol(bits, bundle.tree));
    }
}

int BinkVideoDecoder::next(int id) {
    auto& bundle = bundles_[static_cast<std::size_t>(id)];
    if (bundle.read >= bundle.decoded) damaged("a bundle ran out of values");
    return bundle.values[bundle.read++];
}

// --- DCT coefficients and residues ---------------------------------------------

int BinkVideoDecoder::readDctCoefficients(BitReader& bits, std::array<std::int32_t, 64>& block,
                                          std::array<int, 64>& indices, int& count) const {
    // The coefficients are found by refining a list of candidate groups,
    // one bit plane at a time (from the highest).
    std::array<int, 128> coefficient{};
    std::array<int, 128> mode{};
    int listStart = 64;
    int listEnd = 64;
    const auto append = [&](int c, int m) {
        coefficient[static_cast<std::size_t>(listEnd)] = c;
        mode[static_cast<std::size_t>(listEnd++)] = m;
    };
    append(4, 0);
    append(24, 0);
    append(44, 0);
    append(1, 3);
    append(2, 3);
    append(3, 3);
    count = 0;

    const auto readValue = [&](int bitsLeft) {
        if (bitsLeft == 0) return bits.readBit() ? -1 : 1;
        int value = static_cast<int>(bits.read(bitsLeft)) | (1 << bitsLeft);
        if (bits.readBit()) value = -value;
        return value;
    };

    for (int plane = static_cast<int>(bits.read(4)) - 1; plane >= 0; --plane) {
        int position = listStart;
        while (position < listEnd) {
            const auto p = static_cast<std::size_t>(position);
            if ((mode[p] | coefficient[p]) == 0 || !bits.readBit()) {
                ++position;
                continue;
            }
            int current = coefficient[p];
            const int currentMode = mode[p];
            if (currentMode == 0 || currentMode == 2) {
                if (currentMode == 0) {
                    coefficient[p] = current + 4;
                    mode[p] = 1;
                } else {
                    coefficient[p] = 0;
                    mode[p] = 0;
                    ++position;
                }
                for (int i = 0; i < 4; ++i, ++current) {
                    if (bits.readBit()) {
                        --listStart;
                        coefficient[static_cast<std::size_t>(listStart)] = current;
                        mode[static_cast<std::size_t>(listStart)] = 3;
                    } else {
                        block[bink::kScan[current]] = readValue(plane);
                        indices[static_cast<std::size_t>(count++)] = current;
                    }
                }
            } else if (currentMode == 1) {
                mode[p] = 2;
                for (int i = 0; i < 3; ++i) {
                    current += 4;
                    append(current, 2);
                }
            } else {  // 3: a single coefficient
                block[bink::kScan[current]] = readValue(plane);
                indices[static_cast<std::size_t>(count++)] = current;
                coefficient[p] = 0;
                mode[p] = 0;
                ++position;
            }
        }
    }
    return static_cast<int>(bits.read(4));  // quantiser
}

void BinkVideoDecoder::readResidue(BitReader& bits, std::array<std::int16_t, 64>& block, int masksCount) const {
    std::array<int, 128> coefficient{};
    std::array<int, 128> mode{};
    int listStart = 64;
    int listEnd = 64;
    const auto append = [&](int c, int m) {
        coefficient[static_cast<std::size_t>(listEnd)] = c;
        mode[static_cast<std::size_t>(listEnd++)] = m;
    };
    append(4, 0);
    append(24, 0);
    append(44, 0);
    append(0, 2);
    std::array<int, 64> nonZero{};
    int nonZeroCount = 0;

    for (int mask = 1 << bits.read(3); mask != 0; mask >>= 1) {
        // Refine the coefficients already found.
        for (int i = 0; i < nonZeroCount; ++i) {
            if (!bits.readBit()) continue;
            auto& value = block[static_cast<std::size_t>(nonZero[static_cast<std::size_t>(i)])];
            value = static_cast<std::int16_t>(value < 0 ? value - mask : value + mask);
            if (--masksCount < 0) return;
        }
        // Find new ones.
        int position = listStart;
        while (position < listEnd) {
            const auto p = static_cast<std::size_t>(position);
            if ((coefficient[p] | mode[p]) == 0 || !bits.readBit()) {
                ++position;
                continue;
            }
            int current = coefficient[p];
            const int currentMode = mode[p];
            const auto setNew = [&](int c) {
                const int at = bink::kScan[c];
                nonZero[static_cast<std::size_t>(nonZeroCount++)] = at;
                block[static_cast<std::size_t>(at)] = static_cast<std::int16_t>(bits.readBit() ? -mask : mask);
            };
            if (currentMode == 0 || currentMode == 2) {
                if (currentMode == 0) {
                    coefficient[p] = current + 4;
                    mode[p] = 1;
                } else {
                    coefficient[p] = 0;
                    mode[p] = 0;
                    ++position;
                }
                for (int i = 0; i < 4; ++i, ++current) {
                    if (bits.readBit()) {
                        --listStart;
                        coefficient[static_cast<std::size_t>(listStart)] = current;
                        mode[static_cast<std::size_t>(listStart)] = 3;
                    } else {
                        setNew(current);
                        if (--masksCount < 0) return;
                    }
                }
            } else if (currentMode == 1) {
                mode[p] = 2;
                for (int i = 0; i < 3; ++i) {
                    current += 4;
                    append(current, 2);
                }
            } else {
                setNew(current);
                coefficient[p] = 0;
                mode[p] = 0;
                ++position;
                if (--masksCount < 0) return;
            }
        }
    }
}

// --- Planes and pictures -----------------------------------------------------------

void BinkVideoDecoder::decode(std::span<const std::byte> packet) {
    std::swap(current_, previous_);
    BitReader bits(packet);
    if (revision_ >= 'i') bits.skip(32);
    // Revisions 'h' and later store V before U.
    const bool swapChroma = revision_ >= 'h';
    for (int plane = 0; plane < 3; ++plane) {
        const int index = (plane == 0 || !swapChroma) ? plane : (plane ^ 3);
        decodePlane(bits, index, plane > 0);
        if (bits.bitsLeft() <= 0) break;
    }
}

void BinkVideoDecoder::decodePlane(BitReader& bits, int planeIndex, bool chroma) {
    auto& target = current_[static_cast<std::size_t>(planeIndex)];
    const auto& source = previous_[static_cast<std::size_t>(planeIndex)];
    const int stride = target.stride;
    const int blocksWide = chroma ? (width_ + 15) >> 4 : (width_ + 7) >> 3;
    const int blocksHigh = chroma ? (height_ + 15) >> 4 : (height_ + 7) >> 3;

    // How many bits give the count of values in each bundle.
    const int width = ((std::max(target.width, 8) + 7) / 8) * 8;
    const auto lengthBits = [](int value) { return floorLog2(static_cast<unsigned>(value + 511)) + 1; };
    bundles_[BlockTypes].lengthBits = lengthBits(width >> 3);
    bundles_[SubBlockTypes].lengthBits = lengthBits(width >> 4);
    bundles_[Colours].lengthBits = lengthBits(blocksWide * 64);
    bundles_[IntraDc].lengthBits = bundles_[InterDc].lengthBits = lengthBits(width >> 3);
    bundles_[MotionX].lengthBits = bundles_[MotionY].lengthBits = lengthBits(width >> 3);
    bundles_[Patterns].lengthBits = lengthBits(blocksWide << 3);
    bundles_[Runs].lengthBits = lengthBits(blocksWide * 48);

    for (int id = 0; id < BundleCount; ++id) {
        auto& bundle = bundles_[static_cast<std::size_t>(id)];
        if (id == Colours) {
            for (auto& tree : colourHighTrees_) readTree(bits, tree);
            lastColourHigh_ = 0;
        }
        if (id != IntraDc && id != InterDc) readTree(bits, bundle.tree);
        bundle.decoded = bundle.read = 0;
        bundle.finished = false;
    }

    std::uint8_t* const out = target.pixels.data();
    const std::uint8_t* const in = source.pixels.data();
    const std::ptrdiff_t lastReference = (blocksWide - 1) * 8 + static_cast<std::ptrdiff_t>(stride) * (blocksHigh - 1) * 8;

    const auto copyMoved = [&](std::ptrdiff_t at) {
        const int dx = next(MotionX);
        const int dy = next(MotionY);
        const std::ptrdiff_t from = at + dx + static_cast<std::ptrdiff_t>(dy) * stride;
        if (from < 0 || from > lastReference) damaged("motion outside the picture");
        for (int y = 0; y < 8; ++y) std::copy_n(in + from + y * stride, 8, out + at + y * stride);
    };
    const auto runFill = [&](auto put) {
        const std::uint8_t* scan = bink::kRunPatterns[bits.read(4)];
        int i = 0;
        do {
            const int run = next(Runs) + 1;
            i += run;
            if (i > 64) damaged("run outside the block");
            if (bits.readBit()) {
                const int colour = next(Colours);
                for (int j = 0; j < run; ++j) put(*scan++, colour);
            } else {
                for (int j = 0; j < run; ++j) put(*scan++, next(Colours));
            }
        } while (i < 63);
        if (i == 63) put(*scan++, next(Colours));
    };
    const auto storeDct = [&](std::ptrdiff_t at, std::array<std::int32_t, 64>& block, bool add) {
        const auto pixels = inverseDct(block);
        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 8; ++x) {
                auto& pixel = out[at + y * stride + x];
                const int value = pixels[static_cast<std::size_t>(y * 8 + x)];
                pixel = static_cast<std::uint8_t>(add ? pixel + value : value);  // wraps as the original
            }
        }
    };

    for (int by = 0; by < blocksHigh; ++by) {
        readBlockTypes(bits, bundles_[BlockTypes]);
        readBlockTypes(bits, bundles_[SubBlockTypes]);
        readColours(bits, bundles_[Colours]);
        readPatterns(bits, bundles_[Patterns]);
        readMotion(bits, bundles_[MotionX]);
        readMotion(bits, bundles_[MotionY]);
        readDcValues(bits, bundles_[IntraDc], false);
        readDcValues(bits, bundles_[InterDc], true);
        readRuns(bits, bundles_[Runs]);
        if (bits.bitsLeft() < 0) damaged("picture data too short");

        for (int bx = 0; bx < blocksWide; ++bx) {
            const std::ptrdiff_t at = static_cast<std::ptrdiff_t>(8 * by) * stride + 8 * bx;
            int type = next(BlockTypes);
            // A 16x16 block also covers its right and lower neighbours.
            if (((by & 1) || (bx & 1)) && type == ScaledBlock) {
                ++bx;
                continue;
            }
            switch (type) {
            case SkipBlock:
                for (int y = 0; y < 8; ++y) std::copy_n(in + at + y * stride, 8, out + at + y * stride);
                break;
            case ScaledBlock: {
                std::array<std::uint8_t, 64> small{};
                type = next(SubBlockTypes);
                switch (type) {
                case RunBlock:
                    runFill([&](int index, int colour) { small[static_cast<std::size_t>(index)] = static_cast<std::uint8_t>(colour); });
                    break;
                case IntraBlock: {
                    std::array<std::int32_t, 64> block{};
                    std::array<int, 64> indices{};
                    int count = 0;
                    block[0] = next(IntraDc);
                    const int quant = readDctCoefficients(bits, block, indices, count);
                    dequantise(block, bink::kIntraQuant[quant], indices, count);
                    const auto pixels = inverseDct(block);
                    for (int i = 0; i < 64; ++i) small[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(pixels[static_cast<std::size_t>(i)]);
                    break;
                }
                case FillBlock: {
                    const auto colour = static_cast<std::uint8_t>(next(Colours));
                    for (int y = 0; y < 16; ++y) std::fill_n(out + at + y * stride, 16, colour);
                    break;
                }
                case PatternBlock: {
                    const int colours[2] = {next(Colours), next(Colours)};
                    for (int y = 0; y < 8; ++y) {
                        int pattern = next(Patterns);
                        for (int x = 0; x < 8; ++x, pattern >>= 1) small[static_cast<std::size_t>(y * 8 + x)] = static_cast<std::uint8_t>(colours[pattern & 1]);
                    }
                    break;
                }
                case RawBlock:
                    for (auto& pixel : small) pixel = static_cast<std::uint8_t>(next(Colours));
                    break;
                default:
                    damaged("invalid 16x16 block type");
                }
                if (type != FillBlock) {
                    for (int y = 0; y < 16; ++y) {
                        for (int x = 0; x < 16; ++x) out[at + y * stride + x] = small[static_cast<std::size_t>((y / 2) * 8 + x / 2)];
                    }
                }
                ++bx;
                break;
            }
            case MotionBlock:
                copyMoved(at);
                break;
            case RunBlock:
                runFill([&](int index, int colour) { out[at + (index >> 3) * stride + (index & 7)] = static_cast<std::uint8_t>(colour); });
                break;
            case ResidueBlock: {
                copyMoved(at);
                std::array<std::int16_t, 64> residue{};
                readResidue(bits, residue, static_cast<int>(bits.read(7)));
                for (int y = 0; y < 8; ++y) {
                    for (int x = 0; x < 8; ++x) {
                        auto& pixel = out[at + y * stride + x];
                        pixel = static_cast<std::uint8_t>(pixel + residue[static_cast<std::size_t>(y * 8 + x)]);
                    }
                }
                break;
            }
            case IntraBlock: {
                std::array<std::int32_t, 64> block{};
                std::array<int, 64> indices{};
                int count = 0;
                block[0] = next(IntraDc);
                const int quant = readDctCoefficients(bits, block, indices, count);
                dequantise(block, bink::kIntraQuant[quant], indices, count);
                storeDct(at, block, false);
                break;
            }
            case FillBlock: {
                const auto colour = static_cast<std::uint8_t>(next(Colours));
                for (int y = 0; y < 8; ++y) std::fill_n(out + at + y * stride, 8, colour);
                break;
            }
            case InterBlock: {
                copyMoved(at);
                std::array<std::int32_t, 64> block{};
                std::array<int, 64> indices{};
                int count = 0;
                block[0] = next(InterDc);
                const int quant = readDctCoefficients(bits, block, indices, count);
                dequantise(block, bink::kInterQuant[quant], indices, count);
                storeDct(at, block, true);
                break;
            }
            case PatternBlock: {
                const int colours[2] = {next(Colours), next(Colours)};
                for (int y = 0; y < 8; ++y) {
                    int pattern = next(Patterns);
                    for (int x = 0; x < 8; ++x, pattern >>= 1) out[at + y * stride + x] = static_cast<std::uint8_t>(colours[pattern & 1]);
                }
                break;
            }
            case RawBlock:
                for (int y = 0; y < 8; ++y) {
                    for (int x = 0; x < 8; ++x) out[at + y * stride + x] = static_cast<std::uint8_t>(next(Colours));
                }
                break;
            default:
                damaged("invalid block type");
            }
        }
    }
    bits.alignTo32();  // the next plane starts on a 32-bit boundary
}

formats::RgbaImage BinkVideoDecoder::toRgba() const {
    formats::RgbaImage image;
    image.width = static_cast<std::uint32_t>(width_);
    image.height = static_cast<std::uint32_t>(height_);
    image.pixels.resize(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_) * 4);
    const auto& y = current_[0];
    const auto& u = current_[1];
    const auto& v = current_[2];
    const auto clamp = [](int value) { return static_cast<std::uint8_t>(std::clamp(value, 0, 255)); };
    for (int row = 0; row < height_; ++row) {
        for (int column = 0; column < width_; ++column) {
            // BT.601, limited range, in 16.16 fixed point.
            const int luma = (y.pixels[static_cast<std::size_t>(row * y.stride + column)] - 16) * 76309;
            const int cb = u.pixels[static_cast<std::size_t>((row / 2) * u.stride + column / 2)] - 128;
            const int cr = v.pixels[static_cast<std::size_t>((row / 2) * v.stride + column / 2)] - 128;
            auto* pixel = &image.pixels[(static_cast<std::size_t>(row) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(column)) * 4];
            pixel[0] = clamp((luma + 104597 * cr + 32768) >> 16);
            pixel[1] = clamp((luma - 25675 * cb - 53279 * cr + 32768) >> 16);
            pixel[2] = clamp((luma + 132201 * cb + 32768) >> 16);
            pixel[3] = 255;
        }
    }
    return image;
}

} // namespace openhogs::video
