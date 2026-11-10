// ContentConfig.h
//
// The user's content settings, read from config/openhogs.ini.
//
// Example:
//
//     [Game]
//     Directory = C:\Program Files (x86)\Steam\steamapps\common\HogsofWar
//
//     [Content]
//     PacksDirectory = ../content/packs
//     Language       = it
//     EnabledPacks   = hd-music, ps1-difficulty
//
// Relative paths are resolved against the folder that contains the INI file,
// so the whole OpenHogs folder can be moved without editing the config.

#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace openhogs::core {
class IniFile;
}

namespace openhogs::content {

// Language of the original game files. Choosing it needs no language pack.
inline constexpr const char* kOriginalLanguage = "en";

struct ContentConfig {
    std::filesystem::path gameDirectory;   // original Hogs of War installation
    std::filesystem::path packsDirectory;  // folder containing the content packs
    std::string language = kOriginalLanguage;  // e.g. "en", "it"
    std::vector<std::string> enabledPacks;     // ids of optional packs to use

    // Builds the config from parsed INI data. `baseDirectory` is used to
    // resolve relative paths. Throws FormatError if [Game] Directory is missing.
    static ContentConfig fromIni(const core::IniFile& ini,
                                 const std::filesystem::path& baseDirectory);

    // Loads and parses an INI file (see fromIni).
    static ContentConfig load(const std::filesystem::path& iniPath);
};

} // namespace openhogs::content
