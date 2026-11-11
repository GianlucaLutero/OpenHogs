// SoundPlayer.h
//
// The sound of the game application: owns the mixer and the sound device
// and plays sounds by name from the game's sound banks:
//
//   Audio/sfxday.srl       sounds of a match ("P_OWW", "E_1", footsteps...)
//   FESounds/Fesounds.srl  sounds of the menus ("click1"...)
//
// Sound is optional: on a computer without a sound device, or with sound
// files missing, the player is "silent" and every call does nothing, so
// the game still runs.

#pragma once

#include "openhogs/audio/AudioDevice.h"
#include "openhogs/audio/Mixer.h"
#include "openhogs/audio/SoundLibrary.h"
#include "openhogs/core/VirtualFileSystem.h"
#include "openhogs/formats/SoundBank.h"

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace openhogs::gameapp {

enum class SoundBankId { Match, FrontEnd };

class SoundPlayer {
public:
    // A silent player.
    SoundPlayer() = default;
    // Starts the sound device; falls back to silence (with a warning) if it
    // cannot.
    SoundPlayer(const core::VirtualFileSystem& vfs, const audio::Volumes& volumes);
    ~SoundPlayer();

    SoundPlayer(const SoundPlayer&) = delete;
    SoundPlayer& operator=(const SoundPlayer&) = delete;

    // Plays a sound of a bank by its file name without folder or extension
    // ("P_OWW", case does not matter). Unknown names are reported once.
    void play(SoundBankId bank, std::string_view name);

    // The background sound of a match, in a loop, until stopAmbience().
    void startAmbience();
    void stopAmbience();

    void setVolumes(const audio::Volumes& volumes);
    audio::Volumes volumes() const { return volumes_; }

private:
    struct Bank {
        std::optional<formats::SoundBank> files;
        std::map<std::string, std::shared_ptr<const formats::PcmSound>, std::less<>> loaded;  // by stem
    };

    std::shared_ptr<const formats::PcmSound> find(Bank& bank, std::string_view name);
    Bank& bankFor(SoundBankId id) { return id == SoundBankId::Match ? matchBank_ : frontEndBank_; }

    audio::Volumes volumes_;
    std::unique_ptr<audio::SoundLibrary> library_;
    std::unique_ptr<audio::Mixer> mixer_;
    std::unique_ptr<audio::AudioDevice> device_;
    Bank matchBank_;
    Bank frontEndBank_;
    std::optional<audio::Mixer::VoiceId> ambience_;
};

} // namespace openhogs::gameapp
