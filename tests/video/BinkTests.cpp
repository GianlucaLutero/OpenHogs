// Tests for the video module: the bit reader, the Bink container and the
// parts of the decoders that can be checked without the game's videos
// (which cannot be part of the repository).
//
// The cutscenes are decoded from scratch: a wrong bit order, a wrong
// packet split or a wrong transform scale gives garbage pictures or
// noise for the whole video, so these basics must hold.

#include "openhogs/core/BinaryReader.h"
#include "openhogs/video/BinkAudioDecoder.h"
#include "openhogs/video/BinkFile.h"
#include "openhogs/video/BinkVideoDecoder.h"
#include "openhogs/video/BitReader.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstring>

using namespace openhogs;
using namespace openhogs::video;
using Catch::Approx;

namespace {

std::vector<std::byte> bytes(std::initializer_list<int> values) {
    std::vector<std::byte> out;
    for (const int value : values) out.push_back(static_cast<std::byte>(value));
    return out;
}

void putU32(std::vector<std::byte>& out, std::uint32_t value) {
    for (int i = 0; i < 4; ++i) out.push_back(static_cast<std::byte>((value >> (8 * i)) & 0xFF));
}

void putU16(std::vector<std::byte>& out, std::uint16_t value) {
    out.push_back(static_cast<std::byte>(value & 0xFF));
    out.push_back(static_cast<std::byte>(value >> 8));
}

// A Bink header with `frames` frames of the given sizes (each frame: an
// audio packet of `audioSize` bytes, then video bytes).
std::vector<std::byte> makeBink(const std::vector<std::uint32_t>& frameSizes, std::uint32_t audioSize) {
    std::vector<std::byte> file = bytes({'B', 'I', 'K', 'f'});
    putU32(file, 0);  // size, not checked
    putU32(file, static_cast<std::uint32_t>(frameSizes.size()));
    putU32(file, 0);
    putU32(file, static_cast<std::uint32_t>(frameSizes.size()));
    putU32(file, 320);
    putU32(file, 240);
    putU32(file, 25);
    putU32(file, 1);
    putU32(file, 0);       // video flags
    putU32(file, 1);       // one audio track
    putU32(file, 0);       // its largest decoded size
    putU16(file, 44100);
    putU16(file, 0xE000);  // 16 bits, stereo, RDFT (as the game's videos)
    putU32(file, 0);       // track id
    std::uint32_t offset = static_cast<std::uint32_t>(file.size() + 4 * frameSizes.size());
    for (std::size_t i = 0; i < frameSizes.size(); ++i) {
        putU32(file, offset | (i == 0 ? 1u : 0u));  // the first frame is a key frame
        offset += frameSizes[i];
    }
    for (const auto size : frameSizes) {
        putU32(file, audioSize);
        for (std::uint32_t i = 0; i < size - 4; ++i) file.push_back(static_cast<std::byte>(i & 0xFF));
    }
    return file;
}

// Writes bits least significant first.
struct BitWriter {
    std::vector<std::byte> data;
    std::size_t bit = 0;
    void write(std::uint32_t value, int count) {
        for (int i = 0; i < count; ++i, ++bit) {
            if (bit / 8 >= data.size()) data.push_back(std::byte{0});
            if ((value >> i) & 1u) data[bit / 8] |= static_cast<std::byte>(1u << (bit % 8));
        }
    }
    void alignTo32() {
        while (bit % 32 != 0) write(0, 1);
    }
};

} // namespace

TEST_CASE("Bits are read least significant first", "[video][bits]") {
    // Bink packs every value from the lowest bit of each byte upwards; any
    // other order scrambles the whole stream.
    const auto data = bytes({0b10110100, 0xFF, 0x01});
    BitReader bits(data);
    CHECK(bits.read(2) == 0b00);
    CHECK(bits.read(3) == 0b101);
    CHECK(bits.peek(4) == 0b1101);  // three bits of the first byte, one of the second
    CHECK(bits.read(11) == 0b11111111101);
    CHECK(bits.read(8) == 0x01);
    CHECK(bits.bitsLeft() == 0);
    // Past the end: zeros, and the position still moves on.
    CHECK(bits.read(5) == 0);
    CHECK(bits.bitsLeft() == -5);
}

TEST_CASE("Bit reading can realign to 32 bits", "[video][bits]") {
    // Planes and audio blocks start on 32-bit boundaries.
    const std::vector<std::byte> data(8, std::byte{0xAA});
    BitReader bits(data);
    bits.read(3);
    bits.alignTo32();
    CHECK(bits.position() == 32);
    bits.alignTo32();
    CHECK(bits.position() == 32);
}

TEST_CASE("The Bink container is split into audio and video packets", "[video][bink]") {
    // Each frame starts with the audio packet of each track (its size
    // first); the rest is the picture.
    const auto file = BinkFile::parse(makeBink({40, 60}, 12));
    CHECK(file.revision() == 'f');
    CHECK(file.width() == 320);
    CHECK(file.height() == 240);
    CHECK(file.framesPerSecond() == Approx(25.0));
    REQUIRE(file.audioTracks().size() == 1);
    CHECK(file.audioTracks()[0].sampleRate == 44100);
    CHECK(file.audioTracks()[0].stereo);
    CHECK_FALSE(file.audioTracks()[0].usesDct);
    REQUIRE(file.frameCount() == 2);

    const auto first = file.frame(0);
    CHECK(first.keyFrame);
    REQUIRE(first.audio.size() == 1);
    CHECK(first.audio[0].size() == 12);
    CHECK(first.video.size() == 40 - 4 - 12);
    const auto second = file.frame(1);
    CHECK_FALSE(second.keyFrame);
    CHECK(second.video.size() == 60 - 4 - 12);
}

TEST_CASE("Damaged or foreign files are refused", "[video][bink]") {
    // The PC release has 1-byte placeholders for two videos; they and any
    // other non-Bink file must be refused cleanly, never read past the end.
    CHECK_THROWS_AS(BinkFile::parse(bytes({'#'})), core::FormatError);
    auto wrong = makeBink({40}, 12);
    wrong[0] = std::byte{'X'};
    CHECK_THROWS_AS(BinkFile::parse(wrong), core::FormatError);
    auto tooBig = makeBink({40}, 12);
    tooBig.resize(tooBig.size() - 41);  // the frame table points past the end
    CHECK_THROWS_AS(BinkFile::parse(tooBig), core::FormatError);
}

TEST_CASE("A silent audio block decodes to silence of the right length", "[video][binkaudio]") {
    // 44.1 kHz stereo: 4096-sample transforms, of which 4096 - 256 are
    // output per block (the rest overlaps the next block).
    BinkAudioTrack track;
    track.sampleRate = 44100;
    track.stereo = true;
    BinkAudioDecoder decoder(track, 'f');
    CHECK(decoder.channels() == 2);

    BitWriter bits;
    bits.write(3840 * 2, 32);           // decoded size in bytes (not used)
    bits.write(0, 29);                  // constant term: 0
    bits.write(0, 29);                  // highest frequency: 0
    for (int band = 0; band < 25; ++band) bits.write(0, 8);  // quantisers: 25 bands at 88.2 kHz
    for (int i = 2; i < 4096; i += 8) {
        bits.write(0, 1);               // a run of 8 coefficients
        bits.write(0, 4);               // of width 0: all zero
    }
    bits.alignTo32();
    const auto samples = decoder.decode(bits.data);
    CHECK(samples.size() == 3840);
    for (const float sample : samples) CHECK(sample == 0.0f);
}

TEST_CASE("Only the RDFT audio variant is accepted", "[video][binkaudio]") {
    // The game's videos all use the RDFT variant; a DCT track would be
    // decoded as noise, so it is refused.
    BinkAudioTrack track;
    track.sampleRate = 22050;
    track.usesDct = true;
    CHECK_THROWS_AS(BinkAudioDecoder(track, 'f'), core::FormatError);
}

TEST_CASE("A fresh picture is black, in the video's TV colour range", "[video][binkvideo]") {
    // Bink pictures are YUV with black at 16 and neutral colour at 128;
    // converting with the wrong range gives grey or tinted pictures.
    BinkVideoDecoder decoder(16, 16, 'f', 0);
    const auto image = decoder.toRgba();
    REQUIRE(image.width == 16);
    REQUIRE(image.height == 16);
    for (std::size_t i = 0; i < image.pixels.size(); i += 4) {
        CHECK(image.pixels[i] == 0);
        CHECK(image.pixels[i + 1] == 0);
        CHECK(image.pixels[i + 2] == 0);
        CHECK(image.pixels[i + 3] == 255);
    }
}

TEST_CASE("Unsupported Bink variants are refused", "[video][binkvideo]") {
    // Revision 'b' and transparent videos are coded differently; decoding
    // them with these rules would show garbage.
    CHECK_THROWS_AS(BinkVideoDecoder(320, 240, 'b', 0), core::FormatError);
    CHECK_THROWS_AS(BinkVideoDecoder(320, 240, 'f', 0x00100000), core::FormatError);
}
