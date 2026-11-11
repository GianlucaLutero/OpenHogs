// SoundPlayer.cpp - see SoundPlayer.h for an overview.

#include "SoundPlayer.h"

#include <exception>
#include <iostream>

namespace openhogs::gameapp {

namespace {

constexpr std::uint32_t kAudioOutputRate = 48000;

const char* const kMatchBank = "Audio/sfxday.srl";
const char* const kFrontEndBank = "FESounds/Fesounds.srl";

// The background sound of a match: the first sound of the daytime bank
// (Audio/AMB_1D.wav), played in a loop under the footsteps.
constexpr std::size_t kAmbientSound = 0;
constexpr float kAmbientVolume = 0.6f;

// File name without folder and extension, in lower case:
// "AUDIO\\FT_GRASS.wav" -> "ft_grass".
std::string soundStem(std::string_view path) {
    const auto slash = path.find_last_of("\\/");
    std::string stem(path.substr(slash == std::string_view::npos ? 0 : slash + 1));
    const auto dot = stem.rfind('.');
    if (dot != std::string::npos) {
        stem.resize(dot);
    }
    for (char& c : stem) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return stem;
}

} // namespace

SoundPlayer::SoundPlayer(const core::VirtualFileSystem& vfs, const audio::Volumes& volumes) : volumes_(volumes) {
    try {
        library_ = std::make_unique<audio::SoundLibrary>(vfs);
        mixer_ = std::make_unique<audio::Mixer>(kAudioOutputRate);
        mixer_->setVolumes(volumes);
        device_ = std::make_unique<audio::AudioDevice>(*mixer_);
    } catch (const std::exception& error) {
        std::cerr << "Warning: no sound (" << error.what() << ")\n";
        device_.reset();
        mixer_.reset();
        return;
    }
    // A missing bank only silences its own sounds.
    for (auto [bank, path] : {std::pair{&matchBank_, kMatchBank}, std::pair{&frontEndBank_, kFrontEndBank}}) {
        try {
            bank->files = library_->loadBank(path);
        } catch (const std::exception& error) {
            std::cerr << "Warning: " << error.what() << "\n";
        }
    }
}

SoundPlayer::~SoundPlayer() {
    device_.reset();  // stops the sound thread before the mixer goes
}

std::shared_ptr<const formats::PcmSound> SoundPlayer::find(Bank& bank, std::string_view name) {
    const std::string wanted = soundStem(name);
    auto found = bank.loaded.find(wanted);
    if (found != bank.loaded.end()) {
        return found->second;
    }
    std::shared_ptr<const formats::PcmSound> sound;
    if (bank.files) {
        for (std::size_t i = 0; i < bank.files->files.size(); ++i) {
            if (soundStem(bank.files->files[i]) == wanted) {
                try {
                    sound = library_->loadFromBank(*bank.files, i);
                } catch (const std::exception& error) {
                    std::cerr << "Warning: " << error.what() << "\n";
                }
                break;
            }
        }
    }
    if (!sound) {
        std::cerr << "Warning: sound " << name << " not found\n";
    }
    bank.loaded.emplace(wanted, sound);  // remembered even if missing: warn once
    return sound;
}

void SoundPlayer::play(SoundBankId bank, std::string_view name) {
    if (!mixer_ || name.empty()) {
        return;
    }
    if (auto sound = find(bankFor(bank), name)) {
        mixer_->play(std::move(sound), audio::SoundCategory::Effects);
    }
}

void SoundPlayer::startAmbience() {
    if (!mixer_ || !matchBank_.files || ambience_) {
        return;
    }
    try {
        const auto sound = library_->loadFromBank(*matchBank_.files, kAmbientSound);
        ambience_ = mixer_->play(sound, audio::SoundCategory::Effects, kAmbientVolume, true);
    } catch (const std::exception& error) {
        std::cerr << "Warning: no background sound (" << error.what() << ")\n";
    }
}

void SoundPlayer::stopAmbience() {
    if (mixer_ && ambience_) {
        mixer_->stop(*ambience_);
    }
    ambience_.reset();
}

void SoundPlayer::setVolumes(const audio::Volumes& volumes) {
    volumes_ = volumes;
    if (mixer_) {
        mixer_->setVolumes(volumes);
    }
}

} // namespace openhogs::gameapp
