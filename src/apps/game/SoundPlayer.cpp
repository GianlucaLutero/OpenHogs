// SoundPlayer.cpp - see SoundPlayer.h for an overview.

#include "SoundPlayer.h"

#include <exception>
#include <iostream>

namespace openhogs::gameapp {

namespace {

constexpr std::uint32_t kAudioOutputRate = 48000;

const char* const kMatchBank = "Audio/sfxday.srl";
const char* const kFrontEndBank = "FESounds/Fesounds.srl";

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

void SoundPlayer::play(SoundBankId bank, std::string_view name, float volume, float pitch) {
    if (!mixer_ || name.empty()) {
        return;
    }
    if (auto sound = find(bankFor(bank), name)) {
        mixer_->play(std::move(sound), audio::SoundCategory::Effects, volume, false, pitch);
    }
}

bool SoundPlayer::speaking() const { return mixer_ && speech_ && mixer_->isPlaying(*speech_); }

bool SoundPlayer::say(std::string_view path, float volume) {
    if (!mixer_ || path.empty() || speaking()) {
        return false;
    }
    try {
        speech_ = mixer_->play(library_->load(path), audio::SoundCategory::Speech, volume);
        return true;
    } catch (const std::exception& error) {
        // A missing line (a voice not recorded) only stays silent.
        std::cerr << "Warning: " << error.what() << "\n";
        return false;
    }
}

void SoundPlayer::playMusic(int track, float volume, bool loop) {
    if (!mixer_) {
        return;
    }
    stopMusic();
    try {
        music_ = mixer_->play(library_->load(audio::SoundLibrary::musicTrackPath(track)), audio::SoundCategory::Music,
                              volume, loop);
        musicTrack_ = track;
    } catch (const std::exception& error) {
        std::cerr << "Warning: no music (" << error.what() << ")\n";
    }
}

void SoundPlayer::playMusicSound(std::shared_ptr<const formats::PcmSound> sound, float volume) {
    if (!mixer_) {
        return;
    }
    stopMusic();
    music_ = mixer_->play(std::move(sound), audio::SoundCategory::Music, volume);
    musicTrack_ = -1;  // not a numbered track
}

void SoundPlayer::keepMusic(int track, float volume, bool loop) {
    if (mixer_ && music_ && musicTrack_ == track && mixer_->isPlaying(*music_)) {
        return;
    }
    playMusic(track, volume, loop);
}

void SoundPlayer::stopMusic() {
    if (mixer_ && music_) {
        mixer_->stop(*music_);
    }
    music_.reset();
    musicTrack_ = 0;
}

void SoundPlayer::setVolumes(const audio::Volumes& volumes) {
    volumes_ = volumes;
    if (mixer_) {
        mixer_->setVolumes(volumes);
    }
}

} // namespace openhogs::gameapp
