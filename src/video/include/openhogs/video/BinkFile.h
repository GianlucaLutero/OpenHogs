// BinkFile.h
//
// The container of a Bink 1 video (.bik), as the game's cutscenes in
// Language/Streams use: a header (size, frame rate, audio tracks), a table
// of where each frame starts, then the frames. Each frame holds one audio
// packet per track followed by the compressed picture. See
// docs/formats/bink.md.
//
// BinkFile only splits the file into packets; BinkVideoDecoder and
// BinkAudioDecoder decode them.

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace openhogs::video {

struct BinkAudioTrack {
    std::uint32_t sampleRate = 0;
    bool stereo = false;
    bool usesDct = false;  // the DCT variant (otherwise RDFT)
};

struct BinkFrame {
    std::vector<std::span<const std::byte>> audio;  // one packet per track (may be empty)
    std::span<const std::byte> video;
    bool keyFrame = false;
};

class BinkFile {
public:
    // Parses a whole file held in memory (kept by the object). Throws
    // core::FormatError if it is not a Bink 1 file.
    static BinkFile parse(std::vector<std::byte> data);

    char revision() const { return revision_; }  // 'b' .. 'i', the game's are 'f'
    std::uint32_t width() const { return width_; }
    std::uint32_t height() const { return height_; }
    double framesPerSecond() const { return framesPerSecond_; }
    std::uint32_t videoFlags() const { return videoFlags_; }
    const std::vector<BinkAudioTrack>& audioTracks() const { return audioTracks_; }

    std::size_t frameCount() const { return frameOffsets_.size(); }
    BinkFrame frame(std::size_t index) const;

private:
    std::vector<std::byte> data_;
    char revision_ = 0;
    std::uint32_t width_ = 0;
    std::uint32_t height_ = 0;
    double framesPerSecond_ = 0.0;
    std::uint32_t videoFlags_ = 0;
    std::vector<BinkAudioTrack> audioTracks_;
    std::vector<std::uint32_t> frameOffsets_;  // start of each frame
    std::vector<bool> keyFrames_;
};

} // namespace openhogs::video
