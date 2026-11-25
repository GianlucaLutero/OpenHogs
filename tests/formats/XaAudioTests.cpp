// Tests for formats::XaAudio, the decoder of the PlayStation's XA speech.
//
// The localised PAL releases keep their dubbed voices in interleaved XA
// files: a wrong channel, a wrong sample order or a wrong prediction makes
// the voices noise or puts another hog's line in a hog's mouth. The tests
// build sectors by hand with known values instead of using a real disc.

#include "openhogs/core/BinaryReader.h"  // FormatError
#include "openhogs/formats/XaAudio.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <vector>

using namespace openhogs::formats;

namespace {

constexpr std::uint8_t kMonoHalfRate = 0x04;  // 18,900 Hz, 4 bits, mono
constexpr std::uint8_t kStereoFullRate = 0x01;
constexpr std::uint8_t kEightBitMono = 0x10;

// One audio sector of `channel` whose every sound group has the same
// filter/range byte for all units and the same data bytes (`fill`).
std::vector<std::byte> audioSector(int channel, std::uint8_t coding, std::uint8_t parameters, std::uint8_t fill,
                                   bool audio = true) {
    std::vector<std::byte> sector(kXaSectorSize, std::byte{0});
    sector[16] = std::byte{1};
    sector[17] = static_cast<std::byte>(channel);
    sector[18] = static_cast<std::byte>(audio ? 0x64 : 0x08);  // audio + form 2 + real time, or data
    sector[19] = static_cast<std::byte>(coding);
    for (std::size_t group = 0; group < 18; ++group) {
        const std::size_t base = 24 + group * 128;
        for (std::size_t i = 0; i < 16; ++i) sector[base + i] = static_cast<std::byte>(parameters);
        for (std::size_t i = 16; i < 128; ++i) sector[base + i] = static_cast<std::byte>(fill);
    }
    return sector;
}

void append(std::vector<std::byte>& to, const std::vector<std::byte>& sector) {
    to.insert(to.end(), sector.begin(), sector.end());
}

} // namespace

// Without prediction (filter 0) and range 12 each sample is its 4-bit
// value as is; the low nibble of a byte belongs to the even unit, the high
// one to the odd unit. Sectors of other channels and data sectors must be
// skipped, or another line would be mixed in.
TEST_CASE("XA: a channel's 4-bit samples, other channels skipped", "[formats][xa]") {
    std::vector<std::byte> sectors;
    append(sectors, audioSector(0, kMonoHalfRate, 0x0C, 0x77));   // another channel
    append(sectors, audioSector(5, kMonoHalfRate, 0x0C, 0x2F));   // ours: low nibble -1, high nibble 2
    append(sectors, audioSector(5, kMonoHalfRate, 0x0C, 0x00, false));  // a data sector of ours
    const PcmSound sound = decodeXaChannel(sectors, 5);
    CHECK(sound.sampleRate == 18900);
    CHECK(sound.channels == 1);
    REQUIRE(sound.samples.size() == 18u * 8u * 28u);  // one sector: 4032 samples
    CHECK(sound.samples[0] == -1);        // unit 0: low nibbles
    CHECK(sound.samples[27] == -1);
    CHECK(sound.samples[28] == 2);        // unit 1: high nibbles
    CHECK(sound.samples[55] == 2);
    CHECK(decodeXaChannel(sectors, 7).samples.empty());
}

// The range shifts the value up (range 0: value x 4096), and filter 1
// adds 60/64 of the sample before: the second sample of a run of the same
// value is larger than the first. The prediction carries over from one
// sector to the next, as the voice is one continuous sound.
TEST_CASE("XA: range and prediction", "[formats][xa]") {
    std::vector<std::byte> sectors;
    append(sectors, audioSector(0, kMonoHalfRate, 0x10, 0x11));  // filter 1, range 0, value 1
    const PcmSound sound = decodeXaChannel(sectors, 0);
    REQUIRE(sound.samples.size() >= 2);
    CHECK(sound.samples[0] == 4096);
    CHECK(sound.samples[1] == 4096 + ((4096 * 60 + 32) >> 6));
    // A long run of positive values saturates instead of wrapping round.
    CHECK(sound.samples.back() == 32767);
}

// In stereo the even units are the left channel and the odd units the
// right, interleaved frame by frame; full rate is 37,800 Hz.
TEST_CASE("XA: stereo sectors interleave left and right", "[formats][xa]") {
    std::vector<std::byte> sectors;
    append(sectors, audioSector(1, kStereoFullRate, 0x0C, 0x31));  // left 1, right 3
    const PcmSound sound = decodeXaChannel(sectors, 1);
    CHECK(sound.sampleRate == 37800);
    CHECK(sound.channels == 2);
    REQUIRE(sound.samples.size() == 18u * 8u * 28u);
    CHECK(sound.samples[0] == 1);
    CHECK(sound.samples[1] == 3);
    CHECK(sound.samples[54] == 1);
    CHECK(sound.samples[55] == 3);
}

// 8-bit sectors have four units of whole signed bytes, shifted by 8 minus
// the range.
TEST_CASE("XA: 8-bit samples", "[formats][xa]") {
    std::vector<std::byte> sectors;
    append(sectors, audioSector(0, kEightBitMono, 0x08, 0xFE));  // range 8: the byte itself, -2
    const PcmSound sound = decodeXaChannel(sectors, 0);
    REQUIRE(sound.samples.size() == 18u * 4u * 28u);
    CHECK(sound.samples[0] == -2);
    CHECK(sound.samples.back() == -2);
}

// A truncated file must be reported, not read past its end; the silent
// tail of the last sector goes, so a line does not end in a pause.
TEST_CASE("XA: truncated data and trailing silence", "[formats][xa]") {
    std::vector<std::byte> partial(kXaSectorSize - 1);
    CHECK_THROWS_AS(decodeXaChannel(partial, 0), openhogs::core::FormatError);

    PcmSound sound;
    sound.channels = 1;
    sound.samples = {1000, -2000, 10, -5, 0};
    trimTrailingSilence(sound);
    CHECK(sound.samples == std::vector<std::int16_t>{1000, -2000});
}
