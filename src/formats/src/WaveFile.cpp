// WaveFile.cpp - see WaveFile.h for an overview.
//
// A WAV file is a "RIFF" container: a 12-byte header ("RIFF", size,
// "WAVE") followed by chunks, each with a 4-letter id, a 32-bit size and
// the data (padded to an even size). The "fmt " chunk describes the
// samples, the "data" chunk holds them.

#include "openhogs/formats/WaveFile.h"

#include "openhogs/core/BinaryReader.h"

#include <algorithm>
#include <optional>
#include <string>

namespace openhogs::formats {

namespace {

constexpr std::uint16_t kFormatPcm = 1;

struct WaveFormat {
    std::uint16_t channels = 0;
    std::uint32_t sampleRate = 0;
    std::uint16_t bitsPerSample = 0;
};

WaveFormat readFormatChunk(core::BinaryReader& reader) {
    const std::uint16_t formatTag = reader.readU16();
    if (formatTag != kFormatPcm) {
        throw core::FormatError("WAV: only uncompressed PCM is supported (format tag " +
                                std::to_string(formatTag) + ")");
    }
    WaveFormat format;
    format.channels = reader.readU16();
    format.sampleRate = reader.readU32();
    reader.skip(4 + 2);  // bytes per second, block alignment: derived values
    format.bitsPerSample = reader.readU16();

    if (format.channels != 1 && format.channels != 2) {
        throw core::FormatError("WAV: only mono and stereo are supported");
    }
    if (format.bitsPerSample != 8 && format.bitsPerSample != 16) {
        throw core::FormatError("WAV: only 8-bit and 16-bit samples are supported");
    }
    if (format.sampleRate == 0) {
        throw core::FormatError("WAV: sample rate is zero");
    }
    return format;
}

std::vector<std::int16_t> readSamples(std::span<const std::byte> bytes, const WaveFormat& format) {
    std::vector<std::int16_t> samples;
    if (format.bitsPerSample == 16) {
        core::BinaryReader reader(bytes.first(bytes.size() - bytes.size() % 2));
        samples.reserve(bytes.size() / 2);
        while (reader.remaining() > 0) {
            samples.push_back(reader.readI16());
        }
    } else {
        // 8-bit WAV is unsigned with 128 as silence: shift and scale to 16 bits.
        samples.reserve(bytes.size());
        for (const std::byte b : bytes) {
            samples.push_back(static_cast<std::int16_t>((std::to_integer<int>(b) - 128) * 256));
        }
    }
    // Drop a trailing incomplete frame of a stereo file.
    samples.resize(samples.size() - samples.size() % format.channels);
    return samples;
}

void appendText(std::vector<std::byte>& out, const char* text) {
    for (; *text != '\0'; ++text) out.push_back(static_cast<std::byte>(*text));
}

void appendU16(std::vector<std::byte>& out, std::uint16_t value) {
    out.push_back(static_cast<std::byte>(value & 0xFF));
    out.push_back(static_cast<std::byte>(value >> 8));
}

void appendU32(std::vector<std::byte>& out, std::uint32_t value) {
    appendU16(out, static_cast<std::uint16_t>(value & 0xFFFF));
    appendU16(out, static_cast<std::uint16_t>(value >> 16));
}

} // namespace

PcmSound parseWave(std::span<const std::byte> data) {
    core::BinaryReader reader(data);
    if (reader.readFixedString(4) != "RIFF") {
        throw core::FormatError("Not a WAV file: missing RIFF header");
    }
    reader.skip(4);  // total size: not trusted, the chunks are checked instead
    if (reader.readFixedString(4) != "WAVE") {
        throw core::FormatError("Not a WAV file: RIFF type is not WAVE");
    }

    std::optional<WaveFormat> format;
    while (reader.remaining() >= 8) {
        const std::string id = reader.readFixedString(4);
        const std::uint32_t size = reader.readU32();
        const std::size_t start = reader.position();
        // Some tools write a "data" size larger than the file: clamp it.
        const std::size_t available = std::min<std::size_t>(size, reader.remaining());

        if (id == "fmt ") {
            format = readFormatChunk(reader);
        } else if (id == "data") {
            if (!format) {
                throw core::FormatError("WAV: data chunk before the fmt chunk");
            }
            PcmSound sound;
            sound.sampleRate = format->sampleRate;
            sound.channels = format->channels;
            sound.samples = readSamples(data.subspan(start, available), *format);
            return sound;
        }
        // Chunks are padded to an even size.
        reader.seek(std::min(start + available + (available % 2), data.size()));
    }
    throw core::FormatError("WAV: no data chunk");
}

std::vector<std::byte> encodeWave(const PcmSound& sound) {
    const auto dataBytes = static_cast<std::uint32_t>(sound.samples.size() * 2);
    std::vector<std::byte> out;
    out.reserve(44 + dataBytes);

    appendText(out, "RIFF");
    appendU32(out, 36 + dataBytes);
    appendText(out, "WAVE");

    appendText(out, "fmt ");
    appendU32(out, 16);
    appendU16(out, kFormatPcm);
    appendU16(out, sound.channels);
    appendU32(out, sound.sampleRate);
    appendU32(out, sound.sampleRate * sound.channels * 2u);  // bytes per second
    appendU16(out, static_cast<std::uint16_t>(sound.channels * 2u));  // bytes per frame
    appendU16(out, 16);

    appendText(out, "data");
    appendU32(out, dataBytes);
    for (const std::int16_t sample : sound.samples) {
        appendU16(out, static_cast<std::uint16_t>(sample));
    }
    return out;
}

} // namespace openhogs::formats
