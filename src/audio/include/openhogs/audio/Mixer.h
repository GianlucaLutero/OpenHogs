// Mixer.h
//
// Mixes all playing sounds into one stereo stream.
//
// The game's sounds come at different rates (22,050 Hz effects and speech,
// 37,800 Hz menu sounds, 44,100 Hz music) while the sound card wants one
// rate. The mixer converts each sound on the fly with linear interpolation,
// applies its volume and its category volume, adds everything up and
// clamps the result.
//
// The mixer knows nothing about audio devices: AudioDevice calls mix() from
// the sound card's thread, tools call it to render into a WAV file, tests
// call it directly. All functions are thread-safe.

#pragma once

#include "openhogs/formats/WaveFile.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <vector>

namespace openhogs::audio {

// Every sound belongs to a category with its own volume setting.
enum class SoundCategory { Effects, Speech, Music };

// Volumes from 0 (silent) to 1 (unchanged).
struct Volumes {
    float master = 1.0f;
    float effects = 1.0f;
    float speech = 1.0f;
    float music = 1.0f;

    float forCategory(SoundCategory category) const;
};

class Mixer {
public:
    using VoiceId = std::uint32_t;

    // `outputRate` is the rate of the mixed stream, e.g. 48,000 Hz.
    explicit Mixer(std::uint32_t outputRate);

    // Starts playing a sound and returns an id to control it. The sound is
    // shared, so the same decoded sound can play several times at once.
    VoiceId play(std::shared_ptr<const formats::PcmSound> sound, SoundCategory category,
                 float volume = 1.0f, bool loop = false);

    void stop(VoiceId voice);
    void stopCategory(SoundCategory category);
    void stopAll();

    // False once a sound has finished (or was stopped).
    bool isPlaying(VoiceId voice) const;
    std::size_t activeVoiceCount() const;

    void setVolumes(const Volumes& volumes);
    Volumes volumes() const;

    // Fills `output` with interleaved stereo frames (left, right, left...),
    // values between -1 and 1. Finished sounds are removed.
    void mix(std::span<float> output);

    std::uint32_t outputRate() const { return outputRate_; }

private:
    struct Voice {
        VoiceId id;
        std::shared_ptr<const formats::PcmSound> sound;
        SoundCategory category;
        float volume;
        bool loop;
        double position = 0.0;  // in source frames, fractional for resampling
    };

    // Adds one voice into `output`; returns false when the voice has ended.
    bool mixVoice(Voice& voice, std::span<float> output, float gain) const;

    mutable std::mutex mutex_;
    std::uint32_t outputRate_;
    Volumes volumes_;
    std::vector<Voice> voices_;
    VoiceId nextId_ = 1;
};

} // namespace openhogs::audio
