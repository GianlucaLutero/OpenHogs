// XaAudio.h
//
// Decoder for CD-ROM XA ADPCM audio, the format of the PlayStation's
// speech files (SPEECH/SKU1/.../*.IXA, docs/formats/psx-speech.md).
//
// XA audio lives in Mode 2 Form 2 sectors of 2352 bytes:
//
//     0-11   sync pattern
//     12-15  header (address, mode 2)
//     16-19  subheader: file number, channel, submode, coding info
//     20-23  copy of the subheader
//     24-    2304 bytes: 18 sound groups of 128 bytes (then padding)
//
// Submode bit 2 marks an audio sector. The coding byte says how it is
// coded: bit 0 stereo, bit 2 half rate (18,900 instead of 37,800 samples
// a second), bit 4 eight bits per sample instead of four.
//
// An "interleaved" file mixes several channels sector by sector; each
// channel is decoded on its own. A sound group holds a 16-byte header of
// filter and range values, then the samples of 8 sound units of 28
// samples (4 units with 8-bit samples). Each sample is the coded value
// shifted by its unit's range, plus a prediction from the two samples
// before it (filter coefficients 0, 60/64, 115/64 - 52/64, 98/64 - 55/64).
// In stereo the even units are the left channel, the odd ones the right.
//
// The algorithm is the one documented by Martin Korth's "Nocash PSX
// Specifications" (CDROM XA audio ADPCM); no code was copied.

#pragma once

#include "openhogs/formats/WaveFile.h"

#include <cstddef>
#include <span>

namespace openhogs::formats {

inline constexpr std::size_t kXaSectorSize = 2352;

// Decodes the audio sectors of `channel` in `sectors` (whole 2352-byte
// sectors, as PsxDisc::readRawSectors returns them). Other channels and
// sectors that are not audio are skipped. Returns no samples, and a
// sample rate of 0, if the channel has no audio. Throws core::FormatError
// if `sectors` is not a whole number of sectors.
PcmSound decodeXaChannel(std::span<const std::byte> sectors, int channel);

// Drops the near-silent samples at the end of a sound (the padding of the
// last sector): samples whose magnitude is below `threshold`.
void trimTrailingSilence(PcmSound& sound, int threshold = 64);

} // namespace openhogs::formats
