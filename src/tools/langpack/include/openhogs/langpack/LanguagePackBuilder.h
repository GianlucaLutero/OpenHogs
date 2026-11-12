// LanguagePackBuilder.h
//
// Builds a language content pack (content/packs/lang-<code>/) from the
// user's own PlayStation disc and PC installation.
//
// The European PlayStation release (SLES-01041) contains the game in five
// languages. For one of them this produces, in the PC folder layout:
//
//   data/Language/Text/gtext.bin + .ofs     in-game text
//   data/Language/Text/fetext.bin + .ofs    menus
//   data/Language/Tims/Briefing/LevelN.BMP  mission briefings
//   data/Language/Tims/Title/*.bmp          title screens
//   data/Language/Tims/Papers/*.bmp         newspapers and their headlines
//   pack.ini                                manifest (Type = language)
//
// Text is aligned onto the PC tables (TextAlignment), then the optional
// hand-written overrides are applied (TranslationOverrides). Speech is not
// touched: the PlayStation release has English speech only.
//
// The generated files come from copyrighted game data: they are for the
// user's own use and must not be committed or shared.

#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace openhogs::langpack {

struct LanguagePackOptions {
    std::filesystem::path gameDirectory;   // PC installation (Steam/GOG)
    std::filesystem::path discImage;       // PlayStation .bin or .iso (track 1)
    std::string language = "it";           // it, fr, de or es
    std::filesystem::path outputDirectory; // the pack folder to create
    std::optional<std::filesystem::path> overridesFile;  // content/translations/<lang>.ini
};

struct TableReport {
    std::size_t total = 0;       // strings in the PC table
    std::size_t translated = 0;  // taken from the disc
    std::size_t overridden = 0;  // replaced by hand-written overrides
    std::vector<std::size_t> untranslated;  // still English after overrides
};

struct LanguagePackReport {
    TableReport gameText;   // gtext
    TableReport menuText;   // fetext
    std::size_t briefings = 0;
    std::size_t pictures = 0;  // title screens, newspapers and headlines
    std::vector<std::string> warnings;
};

// Disc folder name of a language ("it" -> "ITALIAN"). Throws
// std::invalid_argument for languages the PlayStation release lacks.
std::string discLanguageName(const std::string& language);

// Creates the pack. Existing files in the output folder are overwritten.
// Throws std::runtime_error (or core::FormatError) if an input is missing
// or not what was expected, e.g. a disc of another release.
LanguagePackReport buildLanguagePack(const LanguagePackOptions& options);

} // namespace openhogs::langpack
