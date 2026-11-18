// AudioSettings.h
//
// Volume settings from the [Audio] section of config/openhogs.ini:
//
//     [Audio]
//     MasterVolume  = 100   ; 0 (silent) .. 100 (full)
//     EffectsVolume = 100
//     SpeechVolume  = 100
//     MusicVolume   = 80
//     SoundMode     = stereo  ; mono, stereo or surround (played as stereo)
//
// volumesToIni() gives the same keys back, so that changes made in the
// game's VOLUME CONTROL can be saved into the file.
//
// Missing keys keep their default (100). Values outside 0..100 are an
// error, so a typo such as "800" is reported instead of deafening anyone.

#pragma once

#include "openhogs/audio/Mixer.h"

#include <string>
#include <utility>
#include <vector>

namespace openhogs::core {
class IniFile;
}

namespace openhogs::audio {

// Throws core::FormatError for volumes that are not whole numbers in
// 0..100 and for an unknown SoundMode.
Volumes volumesFromIni(const core::IniFile& ini);

// The [Audio] keys and values for these volumes (percentages rounded to
// whole numbers), ready for core::updateIniFile().
std::vector<std::pair<std::string, std::string>> volumesToIni(const Volumes& volumes);

} // namespace openhogs::audio
