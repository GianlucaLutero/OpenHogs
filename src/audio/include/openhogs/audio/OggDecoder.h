// OggDecoder.h
//
// Decodes Ogg Vorbis audio (the game's music, MUSIC/TrackNN.ogg: stereo,
// 44,100 Hz) into PCM samples, using the stb_vorbis library (see Credits).
//
// A whole track is decoded at once. A three-minute track takes about 30 MB
// as 16-bit samples, which is acceptable for one playing track and keeps
// the mixer simple (no streaming thread).

#pragma once

#include "openhogs/formats/WaveFile.h"

#include <cstddef>
#include <span>

namespace openhogs::audio {

// Throws core::FormatError if the data is not valid Ogg Vorbis.
formats::PcmSound decodeOggVorbis(std::span<const std::byte> data);

} // namespace openhogs::audio
