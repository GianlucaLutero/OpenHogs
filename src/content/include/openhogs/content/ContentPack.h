// ContentPack.h
//
// A content pack is a folder that replaces or adds game files without
// touching the original installation:
//
//     content/packs/lang-it/
//     ├── pack.ini          <- manifest, described below
//     └── data/             <- same layout as the game folder
//         ├── Language/Text/gtext.bin
//         └── Speech/Sku1/Pig01/01AM0101.wav
//
// pack.ini:
//
//     [Pack]
//     Id          = lang-it            ; unique, used in openhogs.ini
//     Name        = Italian language
//     Type        = language           ; see PackType
//     Language    = it                 ; only for Type = language
//     Priority    = 100                ; higher wins when packs overlap
//     Author      = ...
//     Version     = 1.0
//     Description = ...

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace openhogs::core {
class IniFile;
}

namespace openhogs::content {

// What a pack changes. The type is informative (shown to users, used to
// group packs in a future launcher) except for `Language`: language packs
// are switched on by the [Content] Language setting instead of EnabledPacks.
enum class PackType {
    Language,  // text, speech and briefing images for one language
    Audio,     // sound effects
    Music,     // soundtrack
    Textures,  // replacement images
    Maps,      // new or modified maps
    Misc       // anything else
};

std::optional<PackType> parsePackType(std::string_view text);
std::string_view toString(PackType type);

struct ContentPack {
    std::string id;           // lower case, e.g. "lang-it"
    std::string name;
    PackType type = PackType::Misc;
    std::string language;     // lower case; only set for language packs
    int priority = 0;
    std::string author;
    std::string version;
    std::string description;

    std::filesystem::path directory;      // the pack folder
    std::filesystem::path dataDirectory;  // directory / "data", mounted in the VFS

    // Builds a pack from its manifest. Throws FormatError when a required
    // field is missing or invalid, naming the pack folder in the message.
    static ContentPack fromManifest(const core::IniFile& manifest,
                                    const std::filesystem::path& packDirectory);

    // Reads <packDirectory>/pack.ini (see fromManifest).
    static ContentPack load(const std::filesystem::path& packDirectory);
};

// Name of the manifest file and of the data folder inside each pack.
inline constexpr const char* kPackManifestName = "pack.ini";
inline constexpr const char* kPackDataFolderName = "data";

} // namespace openhogs::content
