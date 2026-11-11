// AudioDevice.cpp - see AudioDevice.h for an overview.

#include "openhogs/audio/AudioDevice.h"

#include <SDL3/SDL.h>

#include <stdexcept>
#include <string>

namespace openhogs::audio {

AudioDevice::AudioDevice(Mixer& mixer) : mixer_(mixer) {
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        throw std::runtime_error(std::string("Cannot initialise audio: ") + SDL_GetError());
    }

    SDL_AudioSpec spec{};
    spec.format = SDL_AUDIO_F32;
    spec.channels = 2;
    spec.freq = static_cast<int>(mixer.outputRate());

    // SDL converts our format to whatever the sound card uses.
    const auto callback = [](void* user, SDL_AudioStream* stream, int additionalAmount, int /*totalAmount*/) {
        static_cast<AudioDevice*>(user)->feed(stream, additionalAmount);
    };
    stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, callback, this);
    if (stream_ == nullptr) {
        const std::string error = SDL_GetError();
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        throw std::runtime_error("Cannot open the sound card: " + error);
    }
    SDL_ResumeAudioStreamDevice(stream_);  // devices start paused
}

AudioDevice::~AudioDevice() {
    SDL_DestroyAudioStream(stream_);  // stops the callbacks before we go away
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void AudioDevice::feed(SDL_AudioStream* stream, int bytesNeeded) {
    if (bytesNeeded <= 0) {
        return;
    }
    // Round down to whole stereo frames of floats.
    const auto frames = static_cast<std::size_t>(bytesNeeded) / (2 * sizeof(float));
    buffer_.resize(frames * 2);
    mixer_.mix(buffer_);
    SDL_PutAudioStreamData(stream, buffer_.data(), static_cast<int>(buffer_.size() * sizeof(float)));
}

} // namespace openhogs::audio
