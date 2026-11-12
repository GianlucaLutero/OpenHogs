// BinkAudioDecoder.h
//
// Decodes the sound of a Bink 1 video (the RDFT variant, which all the
// game's videos use; the DCT variant is refused). Each audio packet holds blocks of
// frequency coefficients; a block becomes sound through an inverse real
// Fourier transform (the "RDFT" variant the game's videos use) or a DCT,
// and consecutive blocks are cross-faded over a short overlap. See
// docs/formats/bink.md.
//
// The decoding follows FFmpeg's libavcodec/binkaudio.c (LGPL 2.1 or
// later), written anew for OpenHogs; see the notice in the .cpp file.

#pragma once

#include "openhogs/video/BinkFile.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace openhogs::video {

class BinkAudioDecoder {
public:
    // `revision` is the file's ('b' .. 'i').
    BinkAudioDecoder(const BinkAudioTrack& track, char revision);

    // Decodes one audio packet; returns its samples, between -1 and 1,
    // interleaved when the track is stereo. Throws core::FormatError on a
    // damaged packet.
    std::vector<float> decode(std::span<const std::byte> packet);

    std::uint32_t sampleRate() const { return sampleRate_; }
    int channels() const { return channels_; }

private:
    void decodeBlock(class BitReader& bits, std::vector<float>& output);
    void inverseRdft(std::vector<float>& coefficients, std::vector<float>& samples) const;

    std::uint32_t sampleRate_ = 0;
    int channels_ = 1;  // of the sound (the RDFT variant codes them interleaved, as one)
    bool versionB_ = false;
    int frameLength_ = 0;    // transform size
    int overlapLength_ = 0;
    float root_ = 0.0f;
    std::vector<float> quantTable_;  // 96 entries
    std::vector<int> bands_;
    bool first_ = true;
    std::vector<float> previous_;  // the end of the last block, to cross-fade with
};

} // namespace openhogs::video
