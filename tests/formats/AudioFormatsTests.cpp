// Tests for the audio file formats: WAV sounds and .srl sound banks.
//
// Every sound effect and speech line of the game is a WAV file, and the
// game picks effects by number through .srl banks. If these parsers break,
// the game is silent or plays the wrong sound, so the tests cover the
// layouts found in the real files plus the ways a replacement sound made
// by a user (with any audio editor) may differ from the originals.

#include "openhogs/core/BinaryReader.h"
#include "openhogs/formats/SoundBank.h"
#include "openhogs/formats/WaveFile.h"

#include "ByteBuilder.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <string_view>

using namespace openhogs::formats;
using openhogs::core::FormatError;
using openhogs::tests::ByteBuilder;
using Catch::Matchers::ContainsSubstring;

namespace {

struct WaveSpec {
    std::uint16_t formatTag = 1;
    std::uint16_t channels = 1;
    std::uint32_t sampleRate = 22050;
    std::uint16_t bitsPerSample = 16;
    bool listChunkBeforeData = false;
};

// Builds a WAV file around raw sample bytes, the way audio editors write them.
std::vector<std::byte> makeWave(const WaveSpec& spec, const ByteBuilder& samples) {
    const auto blockAlign = static_cast<std::uint16_t>(spec.channels * spec.bitsPerSample / 8);
    ByteBuilder body;
    body.raw("WAVE").raw("fmt ").u32(16);
    body.u16(spec.formatTag).u16(spec.channels).u32(spec.sampleRate);
    body.u32(spec.sampleRate * blockAlign).u16(blockAlign).u16(spec.bitsPerSample);
    if (spec.listChunkBeforeData) {
        body.raw("LIST").u32(6).raw("INFOab");
    }
    body.raw("data").u32(static_cast<std::uint32_t>(samples.size()));
    for (const auto b : samples.bytes()) {
        body.u8(static_cast<std::uint8_t>(b));
    }

    ByteBuilder file;
    file.raw("RIFF").u32(static_cast<std::uint32_t>(body.size()));
    for (const auto b : body.bytes()) {
        file.u8(static_cast<std::uint8_t>(b));
    }
    return file.bytes();
}

std::vector<std::byte> toBytes(std::string_view text) {
    std::vector<std::byte> bytes;
    for (const char c : text) {
        bytes.push_back(static_cast<std::byte>(c));
    }
    return bytes;
}

} // namespace

// The original effects and speech are 16-bit mono at 22,050 Hz: the
// samples and the rate must come out exactly, or every sound plays at the
// wrong pitch or as noise.
TEST_CASE("WAV: reads 16-bit mono samples and the sample rate", "[formats][wav]") {
    ByteBuilder samples;
    samples.u16(0).u16(1000).u16(static_cast<std::uint16_t>(-1000)).u16(32767);

    const auto sound = parseWave(makeWave({}, samples));

    CHECK(sound.sampleRate == 22050);
    CHECK(sound.channels == 1);
    CHECK(sound.samples == std::vector<std::int16_t>{0, 1000, -1000, 32767});
    CHECK(sound.frameCount() == 4);
}

// Replacement sounds may be saved as 8-bit WAV, which stores unsigned
// values centred on 128. They must be converted to the same signed scale,
// otherwise they play with a loud DC offset and wrong volume.
TEST_CASE("WAV: converts 8-bit unsigned samples to 16-bit signed", "[formats][wav]") {
    ByteBuilder samples;
    samples.u8(128).u8(0).u8(255);

    const auto sound = parseWave(makeWave({.bitsPerSample = 8}, samples));

    CHECK(sound.samples == std::vector<std::int16_t>{0, -32768, 32512});
}

// Stereo samples are interleaved left/right; the frame count must count
// pairs, or a stereo sound would last twice as long as it should.
TEST_CASE("WAV: keeps stereo samples interleaved", "[formats][wav]") {
    ByteBuilder samples;
    samples.u16(1).u16(2).u16(3).u16(4);

    const auto sound = parseWave(makeWave({.channels = 2, .sampleRate = 44100}, samples));

    CHECK(sound.channels == 2);
    CHECK(sound.frameCount() == 2);
    CHECK(sound.samples == std::vector<std::int16_t>{1, 2, 3, 4});
}

// Many editors add metadata chunks (LIST, INFO...) before the audio. They
// must be skipped instead of being played or rejected.
TEST_CASE("WAV: skips chunks it does not know", "[formats][wav]") {
    ByteBuilder samples;
    samples.u16(42);

    const auto sound = parseWave(makeWave({.listChunkBeforeData = true}, samples));

    CHECK(sound.samples == std::vector<std::int16_t>{42});
}

// Compressed WAV (ADPCM, MP3...) cannot be read as plain samples. A clear
// error tells the user to re-save the file, instead of playing static.
TEST_CASE("WAV: rejects compressed files with a clear message", "[formats][wav]") {
    ByteBuilder samples;
    samples.u16(0);

    CHECK_THROWS_WITH(parseWave(makeWave({.formatTag = 2}, samples)), ContainsSubstring("PCM"));
    CHECK_THROWS_AS(parseWave(toBytes("not a wave file at all")), FormatError);
}

// The sound test writes its mixes with encodeWave; reading the result back
// must give the same sound, or rendered files would not match what plays.
TEST_CASE("WAV: a written file reads back identical", "[formats][wav]") {
    PcmSound original;
    original.sampleRate = 48000;
    original.channels = 2;
    original.samples = {-32768, 32767, 0, 123};

    const auto copy = parseWave(encodeWave(original));

    CHECK(copy.sampleRate == original.sampleRate);
    CHECK(copy.channels == original.channels);
    CHECK(copy.samples == original.samples);
}

// The layout of the game's Audio/sfxday.srl: a count, two zero lines, the
// bank name, then number/path pairs, with Windows line ends. Sound numbers
// are what the game code uses to pick effects, so the order must be kept.
TEST_CASE("SRL: reads the bank name and the numbered sound files", "[formats][srl]") {
    const auto bank = SoundBank::parse(toBytes("002\r\n000\r\n000\r\nsfx day\r\n"
                                               "000\r\nAUDIO\\AMB_1D.wav\r\n"
                                               "001\r\nAUDIO\\L_MGUN.wav\r\n"));

    CHECK(bank.name == "sfx day");
    CHECK(bank.files == std::vector<std::string>{"AUDIO\\AMB_1D.wav", "AUDIO\\L_MGUN.wav"});
    CHECK(bank.file(1) == "AUDIO\\L_MGUN.wav");
    CHECK_FALSE(bank.file(2).has_value());
}

// A hand-edited bank that lost a line would shift every following sound to
// the wrong number. It must be rejected, naming the problem.
TEST_CASE("SRL: rejects banks with missing or out-of-order entries", "[formats][srl]") {
    CHECK_THROWS_WITH(SoundBank::parse(toBytes("002\n000\n000\nbank\n000\na.wav\n")),
                      ContainsSubstring("lists only 1"));
    CHECK_THROWS_WITH(SoundBank::parse(toBytes("001\n000\n000\nbank\n005\na.wav\n")),
                      ContainsSubstring("expected sound number 0"));
    CHECK_THROWS_AS(SoundBank::parse(toBytes("abc\n000\n000\nbank\n")), FormatError);
}
