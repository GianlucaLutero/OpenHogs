// AudioDevice.h
//
// Plays the output of a Mixer on the default sound card, using SDL3.
//
// SDL calls back from its own audio thread whenever the sound card needs
// more data; the callback asks the mixer for that many frames. The mixer
// is thread-safe, so the game can start and stop sounds at any time.

#pragma once

#include "openhogs/audio/Mixer.h"

#include <vector>

struct SDL_AudioStream;

namespace openhogs::audio {

class AudioDevice {
public:
    // Opens the default playback device in 32-bit float stereo at the
    // mixer's output rate and starts playing. Throws std::runtime_error if
    // there is no usable sound card. The mixer must outlive the device.
    explicit AudioDevice(Mixer& mixer);
    ~AudioDevice();

    AudioDevice(const AudioDevice&) = delete;
    AudioDevice& operator=(const AudioDevice&) = delete;

private:
    // Called by SDL on its audio thread.
    void feed(SDL_AudioStream* stream, int bytesNeeded);

    Mixer& mixer_;
    SDL_AudioStream* stream_ = nullptr;
    std::vector<float> buffer_;  // reused between callbacks to avoid allocations
};

} // namespace openhogs::audio
