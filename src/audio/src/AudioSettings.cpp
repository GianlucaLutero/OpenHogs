// AudioSettings.cpp - see AudioSettings.h for an overview.

#include "openhogs/audio/AudioSettings.h"

#include "openhogs/core/BinaryReader.h"  // FormatError
#include "openhogs/core/IniFile.h"

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
    return volumes;
}

} // namespace openhogs::audio
