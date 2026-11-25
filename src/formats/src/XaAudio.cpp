// XaAudio.cpp - see XaAudio.h for an overview.

#include "openhogs/formats/XaAudio.h"

#include "openhogs/core/BinaryReader.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>

namespace openhogs::formats {

namespace {

constexpr std::size_t kSubheaderOffset = 16;
constexpr std::size_t kDataOffset = 24;
constexpr std::size_t kSoundGroups = 18;
constexpr std::size_t kSoundGroupSize = 128;
constexpr std::size_t kSamplesPerUnit = 28;

constexpr std::uint8_t kSubmodeAudio = 0x04;
constexpr std::uint8_t kCodingStereo = 0x01;
constexpr std::uint8_t kCodingHalfRate = 0x04;
constexpr std::uint8_t kCodingEightBit = 0x10;

// Prediction filters, in 64ths: new = coded + (old1 * k0 + old2 * k1) / 64.
constexpr std::array<int, 4> kFilterK0{0, 60, 115, 98};
constexpr std::array<int, 4> kFilterK1{0, 0, -52, -55};

// The two previous samples of one output channel.
struct Predictor {
    int older = 0;
    int old = 0;
};

std::uint8_t byteAt(std::span<const std::byte> data, std::size_t offset) {
    return std::to_integer<std::uint8_t>(data[offset]);
}

// One sample: the coded value (already sign-extended and placed in the
// top bits of 16) shifted by the unit's range, plus the prediction.
std::int16_t predict(int coded, int shift, std::size_t filter, Predictor& history) {
    int sample = coded >> shift;
    sample += (history.old * kFilterK0[filter] + history.older * kFilterK1[filter] + 32) >> 6;
    sample = std::clamp(sample, -32768, 32767);
    history.older = history.old;
    history.old = sample;
    return static_cast<std::int16_t>(sample);
}

// Decodes one 128-byte sound group into `out` (interleaved when stereo).
void decodeSoundGroup(std::span<const std::byte> group, bool stereo, bool eightBit,
                      std::array<Predictor, 2>& history, std::vector<std::int16_t>& out) {
    const std::size_t units = eightBit ? 4 : 8;
    const std::size_t outputChannels = stereo ? 2 : 1;
    const std::size_t first = out.size();
    out.resize(first + units * kSamplesPerUnit);
    for (std::size_t unit = 0; unit < units; ++unit) {
        // The header repeats each unit's byte; this is the first copy.
        const std::uint8_t parameters = byteAt(group, 4 + unit);
        const int range = parameters & 0x0F;
        const std::size_t filter = (parameters >> 4) & 0x03u;
        const std::size_t channel = stereo ? unit % 2 : 0;
        // Where this unit's samples go: in stereo a pair of units makes 28
        // interleaved left/right frames.
        const std::size_t unitBase = stereo ? (unit / 2) * kSamplesPerUnit * outputChannels : unit * kSamplesPerUnit;
        for (std::size_t j = 0; j < kSamplesPerUnit; ++j) {
            int coded = 0;
            if (eightBit) {
                coded = static_cast<std::int8_t>(byteAt(group, 16 + unit + j * 4)) * 256;  // value << 8
            } else {
                const std::uint8_t pair = byteAt(group, 16 + unit / 2 + j * 4);
                const int nibble = unit % 2 == 0 ? (pair & 0x0F) : (pair >> 4);
                coded = static_cast<std::int16_t>(static_cast<std::uint16_t>(nibble << 12));  // value << 12
            }
            const std::size_t at = unitBase + (stereo ? j * 2 + channel : j);
            out[first + at] = predict(coded, range, filter, history[channel]);
        }
    }
}

} // namespace

PcmSound decodeXaChannel(std::span<const std::byte> sectors, int channel) {
    if (sectors.size() % kXaSectorSize != 0) {
        throw core::FormatError("XA audio: data is not a whole number of 2352-byte sectors");
    }
    PcmSound sound;
    std::array<Predictor, 2> history{};
    for (std::size_t offset = 0; offset < sectors.size(); offset += kXaSectorSize) {
        const auto sector = sectors.subspan(offset, kXaSectorSize);
        const int sectorChannel = byteAt(sector, kSubheaderOffset + 1);
        const std::uint8_t submode = byteAt(sector, kSubheaderOffset + 2);
        const std::uint8_t coding = byteAt(sector, kSubheaderOffset + 3);
        if (sectorChannel != channel || (submode & kSubmodeAudio) == 0) continue;

        const bool stereo = (coding & kCodingStereo) != 0;
        const bool eightBit = (coding & kCodingEightBit) != 0;
        sound.channels = stereo ? 2 : 1;
        sound.sampleRate = (coding & kCodingHalfRate) != 0 ? 18900 : 37800;
        for (std::size_t group = 0; group < kSoundGroups; ++group) {
            decodeSoundGroup(sector.subspan(kDataOffset + group * kSoundGroupSize, kSoundGroupSize), stereo, eightBit,
                             history, sound.samples);
        }
    }
    return sound;
}

void trimTrailingSilence(PcmSound& sound, int threshold) {
    const std::size_t step = std::max<std::size_t>(sound.channels, 1);
    std::size_t end = sound.samples.size() - sound.samples.size() % step;
    while (end >= step) {
        bool quiet = true;
        for (std::size_t c = 0; c < step; ++c) {
            if (std::abs(static_cast<int>(sound.samples[end - step + c])) >= threshold) quiet = false;
        }
        if (!quiet) break;
        end -= step;
    }
    sound.samples.resize(end);
}

} // namespace openhogs::formats
