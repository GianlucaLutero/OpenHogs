// Mixer.cpp - see Mixer.h for an overview.

#include "openhogs/audio/Mixer.h"

#include <algorithm>
#include <stdexcept>

namespace openhogs::audio {

namespace {

constexpr float kSampleScale = 1.0f / 32768.0f;  // int16 -> -1..1

// Sample of `channel` at a whole source frame; mono sounds use channel 0
// for both sides.
float sampleAt(const formats::PcmSound& sound, std::size_t frame, std::size_t channel) {
    const std::size_t c = (sound.channels == 1) ? 0 : channel;
    return static_cast<float>(sound.samples[frame * sound.channels + c]) * kSampleScale;
}

} // namespace

float Volumes::forCategory(SoundCategory category) const {
    switch (category) {
        case SoundCategory::Effects: return effects;
        case SoundCategory::Speech: return speech;
        case SoundCategory::Music: return music;
    }
    return 1.0f;
}

Mixer::Mixer(std::uint32_t outputRate) : outputRate_(outputRate) {
    if (outputRate == 0) {
        throw std::invalid_argument("Mixer output rate must be greater than zero");
    }
}

Mixer::VoiceId Mixer::play(std::shared_ptr<const formats::PcmSound> sound, SoundCategory category, float volume,
                           bool loop, float pitch) {
    if (!sound || sound->frameCount() == 0 || sound->sampleRate == 0) {
        return 0;  // nothing to play; 0 is never a valid id
    }
    std::lock_guard lock(mutex_);
    const VoiceId id = nextId_++;
    voices_.push_back({id, std::move(sound), category, volume, loop, pitch > 0.0f ? pitch : 1.0f});
    return id;
}

void Mixer::stop(VoiceId voice) {
    std::lock_guard lock(mutex_);
    std::erase_if(voices_, [voice](const Voice& v) { return v.id == voice; });
}

void Mixer::stopCategory(SoundCategory category) {
    std::lock_guard lock(mutex_);
    std::erase_if(voices_, [category](const Voice& v) { return v.category == category; });
}

void Mixer::stopAll() {
    std::lock_guard lock(mutex_);
    voices_.clear();
}

bool Mixer::isPlaying(VoiceId voice) const {
    std::lock_guard lock(mutex_);
    return std::any_of(voices_.begin(), voices_.end(), [voice](const Voice& v) { return v.id == voice; });
}

std::size_t Mixer::activeVoiceCount() const {
    std::lock_guard lock(mutex_);
    return voices_.size();
}

void Mixer::setVolumes(const Volumes& volumes) {
    std::lock_guard lock(mutex_);
    volumes_ = volumes;
}

Volumes Mixer::volumes() const {
    std::lock_guard lock(mutex_);
    return volumes_;
}

bool Mixer::mixVoice(Voice& voice, std::span<float> output, float gain) const {
    const formats::PcmSound& sound = *voice.sound;
    const std::size_t frames = sound.frameCount();
    // How far to advance in the source for one output frame.
    const double step =
        static_cast<double>(sound.sampleRate) / static_cast<double>(outputRate_) * static_cast<double>(voice.pitch);

    for (std::size_t out = 0; out + 1 < output.size(); out += 2) {
        if (voice.position >= static_cast<double>(frames)) {
            if (!voice.loop) {
                return false;
            }
            voice.position -= static_cast<double>(frames);
        }

        // Linear interpolation between the two nearest source frames.
        const auto index = static_cast<std::size_t>(voice.position);
        const std::size_t nextIndex = (index + 1 < frames) ? index + 1 : (voice.loop ? 0 : index);
        const auto fraction = static_cast<float>(voice.position - static_cast<double>(index));
        for (std::size_t channel = 0; channel < 2; ++channel) {
            const float a = sampleAt(sound, index, channel);
            const float b = sampleAt(sound, nextIndex, channel);
            output[out + channel] += (a + (b - a) * fraction) * gain;
        }
        voice.position += step;
    }
    return voice.loop || voice.position < static_cast<double>(frames);
}

void Mixer::mix(std::span<float> output) {
    std::fill(output.begin(), output.end(), 0.0f);

    std::lock_guard lock(mutex_);
    std::erase_if(voices_, [&](Voice& voice) {
        const float gain = voice.volume * volumes_.forCategory(voice.category) * volumes_.master;
        return !mixVoice(voice, output, gain);
    });

    // Several loud sounds can add up beyond full scale: clamp instead of
    // letting the values wrap around, which would sound like loud cracks.
    for (float& value : output) {
        value = std::clamp(value, -1.0f, 1.0f);
    }
}

} // namespace openhogs::audio
