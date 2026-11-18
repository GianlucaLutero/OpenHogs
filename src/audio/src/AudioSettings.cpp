// AudioSettings.cpp - see AudioSettings.h for an overview.

#include "openhogs/audio/AudioSettings.h"

#include "openhogs/core/BinaryReader.h"  // FormatError
#include "openhogs/core/IniFile.h"

#include <cmath>
#include <string>

namespace openhogs::audio {

namespace {

constexpr int kDefaultPercent = 100;

float readPercent(const core::IniFile& ini, const char* key) {
    const int percent = ini.getInt("Audio", key, kDefaultPercent);
    if (percent < 0 || percent > 100) {
        throw core::FormatError(std::string("[Audio] ") + key + " must be between 0 and 100, got " +
                                std::to_string(percent));
    }
    return static_cast<float>(percent) / 100.0f;
}

} // namespace

Volumes volumesFromIni(const core::IniFile& ini) {
    Volumes volumes;
    volumes.master = readPercent(ini, "MasterVolume");
    volumes.effects = readPercent(ini, "EffectsVolume");
    volumes.speech = readPercent(ini, "SpeechVolume");
    volumes.music = readPercent(ini, "MusicVolume");
    const std::string mode = ini.getString("Audio", "SoundMode", "stereo");
    if (mode == "mono") {
        volumes.mode = SoundMode::Mono;
    } else if (mode == "surround") {
        volumes.mode = SoundMode::Surround;
    } else if (mode != "stereo") {
        throw core::FormatError("[Audio] SoundMode must be mono, stereo or surround, got " + mode);
    }
    return volumes;
}

std::vector<std::pair<std::string, std::string>> volumesToIni(const Volumes& volumes) {
    const auto percent = [](float volume) { return std::to_string(std::lround(volume * 100.0f)); };
    const char* mode = volumes.mode == SoundMode::Mono       ? "mono"
                       : volumes.mode == SoundMode::Surround ? "surround"
                                                             : "stereo";
    return {{"MasterVolume", percent(volumes.master)},
            {"EffectsVolume", percent(volumes.effects)},
            {"SpeechVolume", percent(volumes.speech)},
            {"MusicVolume", percent(volumes.music)},
            {"SoundMode", mode}};
}

} // namespace openhogs::audio
