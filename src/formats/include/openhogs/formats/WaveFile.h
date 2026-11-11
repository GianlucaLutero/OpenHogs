// WaveFile.h
//
// Reader for the .WAV files of the game: sound effects (Audio/, FESounds/)
// and speech (Speech/). See docs/formats/audio.md.
//
// All game files are uncompressed PCM, 16-bit mono, at 22,050 Hz (effects,
// speech) or 37,800 Hz (some menu sounds). The reader also accepts 8-bit
// and stereo PCM so that content packs can use common tools to make
// replacement sounds.

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace openhogs::formats {

// Decoded audio, ready for the mixer.
struct PcmSound {
    std::uint32_t sampleRate = 0;   // frames per second, e.g. 22050
    std::uint16_t channels = 0;     // 1 = mono, 2 = stereo
    std::vector<std::int16_t> samples;  // interleaved when stereo

    std::size_t frameCount() const { return channels == 0 ? 0 : samples.size() / channels; }
    double durationSeconds() const {
        return sampleRate == 0 ? 0.0 : static_cast<double>(frameCount()) / sampleRate;
    }
};

// Parses a RIFF/WAVE file. Chunks other than "fmt " and "data" (e.g.
// "LIST" metadata) are skipped. Throws core::FormatError for files that are
// not PCM WAV, or that use a sample size other than 8 or 16 bits.
PcmSound parseWave(std::span<const std::byte> data);

// Writes PCM audio as a 16-bit WAV file (used to save mixed output).
std::vector<std::byte> encodeWave(const PcmSound& sound);

} // namespace openhogs::formats
