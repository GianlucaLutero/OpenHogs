// OggDecoder.cpp - see OggDecoder.h for an overview.

#include "openhogs/audio/OggDecoder.h"

#include "openhogs/core/BinaryReader.h"  // FormatError

#include <cstdlib>
#include <limits>

// stb_vorbis is a single C file: with this define only its declarations
// are included here, the code itself is compiled in third_party/stb_vorbis.
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

namespace openhogs::audio {

formats::PcmSound decodeOggVorbis(std::span<const std::byte> data) {
    if (data.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw core::FormatError("OGG: file too large");
    }

    int channels = 0;
    int sampleRate = 0;
    short* decoded = nullptr;
    const int frames = stb_vorbis_decode_memory(reinterpret_cast<const unsigned char*>(data.data()),
                                                static_cast<int>(data.size()), &channels, &sampleRate, &decoded);
    if (frames < 0 || decoded == nullptr) {
        throw core::FormatError("OGG: not a valid Ogg Vorbis file");
    }

    formats::PcmSound sound;
    sound.sampleRate = static_cast<std::uint32_t>(sampleRate);
    sound.channels = static_cast<std::uint16_t>(channels);
    sound.samples.assign(decoded, decoded + static_cast<std::size_t>(frames) * static_cast<std::size_t>(channels));
    std::free(decoded);  // allocated by stb_vorbis with malloc

    if (sound.channels != 1 && sound.channels != 2) {
        throw core::FormatError("OGG: only mono and stereo are supported");
    }
    return sound;
}

} // namespace openhogs::audio
