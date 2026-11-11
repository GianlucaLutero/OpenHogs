// AudioSettings.h
//
// Volume settings from the [Audio] section of config/openhogs.ini:
//
//     [Audio]
//     MasterVolume  = 100   ; 0 (silent) .. 100 (full)
//     EffectsVolume = 100
//     SpeechVolume  = 100
//     MusicVolume   = 80
//
// Missing keys keep their default (100). Values outside 0..100 are an
// error, so a typo such as "800" is reported instead of deafening anyone.

#pragma once

#include "openhogs/audio/Mixer.h"

namespace openhogs::core {
class IniFile;
}

namespace openhogs::audio {

// Throws core::FormatError for values that are not whole numbers in 0..100.
Volumes volumesFromIni(const core::IniFile& ini);

} // namespace openhogs::audio
