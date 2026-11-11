// SoundLibrary.h
//
// Loads the game's sounds through the virtual file system, so that content
// packs can replace any of them (e.g. a pack with re-recorded speech or
// remastered music) without touching the installation.
//
// Decoded sounds are cached: the same effect (a gunshot) is decoded once
// and shared by every voice that plays it.
//
// Where the sounds are (see docs/formats/audio.md):
//   Audio/*.wav, Audio/sfxday.srl          in-game effects and their bank
//   FESounds/*.wav, FESounds/Fesounds.srl  menu sounds and their bank
//   Speech/Sku1/...                        voices
//   MUSIC/Track02.ogg ... Track32.ogg      music (numbered like CD tracks)

#pragma once

#include "openhogs/core/VirtualFileSystem.h"
#include "openhogs/formats/SoundBank.h"
#include "openhogs/formats/WaveFile.h"

#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace openhogs::audio {

class SoundLibrary {
public:
    // The file system must outlive the library.
    explicit SoundLibrary(const core::VirtualFileSystem& vfs);

    // Loads a .wav or .ogg file (decided by its extension), using the cache.
    // Throws std::runtime_error if the file is missing, core::FormatError
    // if it cannot be decoded.
    std::shared_ptr<const formats::PcmSound> load(std::string_view path);

    // Loads a .srl sound bank.
    formats::SoundBank loadBank(std::string_view path) const;

    // Loads sound `number` of a bank. The bank stores Windows paths that
    // may differ in case from the real files; the VFS resolves them.
    std::shared_ptr<const formats::PcmSound> loadFromBank(const formats::SoundBank& bank, std::size_t number);

    // Path of a music track: 2 -> "MUSIC/Track02.ogg". The numbering starts
    // at 2 because on the original CD track 1 was the data track.
    static std::string musicTrackPath(int track);

    // Track numbers present in the file system, sorted.
    std::vector<int> musicTracks() const;

    // Paths of all .srl banks in the file system.
    std::vector<std::string> soundBanks() const;

    // Frees the decoded sounds that nothing else is using.
    void trimCache();

private:
    const core::VirtualFileSystem& vfs_;
    std::map<std::string, std::shared_ptr<const formats::PcmSound>> cache_;  // by lookup key
};

} // namespace openhogs::audio
