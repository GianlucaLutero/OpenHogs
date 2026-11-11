// SoundLibrary.cpp - see SoundLibrary.h for an overview.

#include "openhogs/audio/SoundLibrary.h"

#include "openhogs/audio/OggDecoder.h"
#include "openhogs/core/PathUtils.h"

#include <cstdio>
#include <stdexcept>

namespace openhogs::audio {

SoundLibrary::SoundLibrary(const core::VirtualFileSystem& vfs) : vfs_(vfs) {}

std::shared_ptr<const formats::PcmSound> SoundLibrary::load(std::string_view path) {
    const std::string key = core::makeLookupKey(path);
    if (const auto it = cache_.find(key); it != cache_.end()) {
        return it->second;
    }

    const auto data = vfs_.readFile(path);
    std::shared_ptr<const formats::PcmSound> sound;
    if (key.ends_with(".ogg")) {
        sound = std::make_shared<formats::PcmSound>(decodeOggVorbis(data));
    } else if (key.ends_with(".wav")) {
        sound = std::make_shared<formats::PcmSound>(formats::parseWave(data));
    } else {
        throw std::runtime_error("Unsupported sound file (expected .wav or .ogg): " + std::string(path));
    }

    cache_[key] = sound;
    return sound;
}

formats::SoundBank SoundLibrary::loadBank(std::string_view path) const {
    return formats::SoundBank::parse(vfs_.readFile(path));
}

std::shared_ptr<const formats::PcmSound> SoundLibrary::loadFromBank(const formats::SoundBank& bank,
                                                                    std::size_t number) {
    const auto file = bank.file(number);
    if (!file) {
        throw std::runtime_error("Sound bank " + bank.name + " has no sound " + std::to_string(number) + " (it has " +
                                 std::to_string(bank.files.size()) + ")");
    }
    return load(*file);
}

std::string SoundLibrary::musicTrackPath(int track) {
    char name[32];
    std::snprintf(name, sizeof(name), "MUSIC/Track%02d.ogg", track);
    return name;
}

std::vector<int> SoundLibrary::musicTracks() const {
    std::vector<int> tracks;
    for (const auto& key : vfs_.listFiles("music")) {
        // "music/track07.ogg" -> 7
        const std::string prefix = "music/track";
        if (!key.starts_with(prefix) || !key.ends_with(".ogg")) {
            continue;
        }
        const std::string number = key.substr(prefix.size(), key.size() - prefix.size() - 4);
        if (!number.empty() && number.find_first_not_of("0123456789") == std::string::npos) {
            tracks.push_back(std::stoi(number));
        }
    }
    return tracks;  // listFiles is sorted and the numbers have two digits
}

std::vector<std::string> SoundLibrary::soundBanks() const {
    std::vector<std::string> banks;
    for (const auto& key : vfs_.listFiles("")) {
        if (key.ends_with(".srl")) {
            banks.push_back(key);
        }
    }
    return banks;
}

void SoundLibrary::trimCache() {
    std::erase_if(cache_, [](const auto& entry) { return entry.second.use_count() == 1; });
}

} // namespace openhogs::audio
