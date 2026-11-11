// SoundPlayer.h
//
// The sound of the game application: owns the mixer and the sound device
// and plays
//
//   - sounds by name from the game's sound banks:
//       Audio/sfxday.srl       sounds of a match ("P_OWW", "E_1", footsteps...)
//       FESounds/Fesounds.srl  sounds of the menus ("click1"...)
//   - speech files (Speech/Sku1/...), one line at a time;
//   - music (MUSIC/TrackNN.ogg), one track at a time.
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
    void play(SoundBankId bank, std::string_view name, float volume = 1.0f, float pitch = 1.0f);

    // Says a line (a speech file, e.g. "Speech/Sku1/Sarge/SGEN0301.wav")
    // unless another line is still being said, as the original never
    // talks over itself. Returns false if it was not played.
    bool say(std::string_view path, float volume = 1.0f);
    bool speaking() const;

    // Plays music track `track` (MUSIC/TrackNN.ogg), stopping the one
    // playing. keepMusic() does nothing if that track is already playing.
    void playMusic(int track, float volume = 1.0f, bool loop = false);
    void keepMusic(int track, float volume = 1.0f, bool loop = false);
    void stopMusic();

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
    std::optional<audio::Mixer::VoiceId> speech_;
    std::optional<audio::Mixer::VoiceId> music_;
    int musicTrack_ = 0;
};

} // namespace openhogs::gameapp
