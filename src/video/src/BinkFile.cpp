// BinkFile.cpp - see BinkFile.h for an overview, docs/formats/bink.md for
// the layout (it follows FFmpeg's libavformat/bink.c, used as reference).

#include "openhogs/video/BinkFile.h"

#include "openhogs/core/BinaryReader.h"

#include <string>

namespace openhogs::video {

namespace {

// Audio track flags.
constexpr std::uint16_t kAudioStereo = 0x2000;
constexpr std::uint16_t kAudioUsesDct = 0x1000;

constexpr std::uint32_t kMaxAudioTracks = 256;
constexpr std::uint32_t kMaxFrames = 1000000;

} // namespace

BinkFile BinkFile::parse(std::vector<std::byte> data) {
    BinkFile file;
    file.data_ = std::move(data);
    core::BinaryReader reader(file.data_);
    try {
        const auto signature = reader.readFixedString(3);
        file.revision_ = static_cast<char>(reader.readU8());
        if (signature != "BIK" || file.revision_ < 'b' || file.revision_ > 'i') {
            throw core::FormatError("not a Bink 1 video");
        }
        reader.skip(4);  // file size - 8
        const std::uint32_t frames = reader.readU32();
        reader.skip(4);  // largest frame
        reader.skip(4);  // frames again
        file.width_ = reader.readU32();
        file.height_ = reader.readU32();
        const std::uint32_t rateNumerator = reader.readU32();
        const std::uint32_t rateDenominator = reader.readU32();
        file.videoFlags_ = reader.readU32();
        const std::uint32_t tracks = reader.readU32();
        if (frames == 0 || frames > kMaxFrames || tracks > kMaxAudioTracks || rateNumerator == 0 ||
            rateDenominator == 0 || file.width_ == 0 || file.height_ == 0 || file.width_ > 7680 ||
            file.height_ > 4800) {
            throw core::FormatError("invalid Bink header");
        }
        file.framesPerSecond_ = static_cast<double>(rateNumerator) / rateDenominator;

        reader.skip(4 * tracks);  // largest decoded size of each track
        for (std::uint32_t i = 0; i < tracks; ++i) {
            BinkAudioTrack track;
            track.sampleRate = reader.readU16();
            const std::uint16_t flags = reader.readU16();
            track.stereo = (flags & kAudioStereo) != 0;
            track.usesDct = (flags & kAudioUsesDct) != 0;
            file.audioTracks_.push_back(track);
        }
        reader.skip(4 * tracks);  // track ids

        // Frame offsets; the lowest bit marks key frames.
        for (std::uint32_t i = 0; i < frames; ++i) {
            const std::uint32_t entry = reader.readU32();
            file.frameOffsets_.push_back(entry & ~1u);
            file.keyFrames_.push_back((entry & 1u) != 0);
        }
    } catch (const core::FormatError& error) {
        throw core::FormatError(std::string("Bink video: ") + error.what());
    }
    for (std::size_t i = 0; i < file.frameOffsets_.size(); ++i) {
        const std::uint32_t end = i + 1 < file.frameOffsets_.size()
                                      ? file.frameOffsets_[i + 1]
                                      : static_cast<std::uint32_t>(file.data_.size());
        if (file.frameOffsets_[i] >= end || end > file.data_.size()) {
            throw core::FormatError("Bink video: invalid frame table");
        }
    }
    return file;
}

BinkFrame BinkFile::frame(std::size_t index) const {
    const std::size_t start = frameOffsets_.at(index);
    const std::size_t end = index + 1 < frameOffsets_.size() ? frameOffsets_[index + 1] : data_.size();
    std::span<const std::byte> rest(data_.data() + start, end - start);

    BinkFrame frame;
    frame.keyFrame = keyFrames_[index];
    // One audio packet per track: its size, then its bytes.
    for (std::size_t track = 0; track < audioTracks_.size(); ++track) {
        core::BinaryReader reader(rest);
        const std::uint32_t size = reader.readU32();
        if (size > rest.size() - 4) {
            throw core::FormatError("Bink video: audio packet larger than its frame");
        }
        frame.audio.push_back(rest.subspan(4, size));
        rest = rest.subspan(4 + size);
    }
    frame.video = rest;
    return frame;
}

} // namespace openhogs::video
