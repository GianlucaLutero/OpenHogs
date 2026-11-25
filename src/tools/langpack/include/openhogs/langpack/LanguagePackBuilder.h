// LanguagePackBuilder.h
//
// Builds a language content pack (content/packs/lang-<code>/) from the
// user's own PlayStation disc and PC installation.
//
// A localised PC CD (the Italian PC release, say) works too: it has the
// PC's own layout, so every file of Language/ and Speech/ that differs
// from the installed one (text, pictures, films, the dubbed speech) is
// copied under the installed file's name (buildFromPcDisc below).
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
//   data/Speech/Sku1/...                    dubbed speech (localised releases)
//
// Text is aligned onto the PC tables (TextAlignment), then the optional
// hand-written overrides are applied (TranslationOverrides).
//
// Speech: SLES-01041 has the PC's English voices, so by default speech is
// only taken from a localised PAL release (SLES-02766 to 02769), whose
// voices are dubbed (decision 139): each PC line is replaced by the same
// line decoded from the disc's XA files (SpeechExtraction, XaAudio). The
// text and pictures are taken too when the disc has them.
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
    // Auto: speech only from a dubbed release; Always: from any disc (the
    // European disc then gives its English voices); Never: no speech.
    enum class Speech { Auto, Always, Never };
    Speech speech = Speech::Auto;
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
    bool textFromDisc = true;  // false when the disc has no text in that language
    std::size_t speechLines = 0;  // speech files replaced
    // From a localised PC CD: the files copied, by kind.
    bool pcDisc = false;
    std::size_t textFiles = 0;  // Language/Text
    std::size_t films = 0;      // Language/Streams
    std::vector<std::string> warnings;
};

// Disc folder name of a language ("it" -> "ITALIAN"). Throws
// std::invalid_argument for languages the PlayStation release lacks.
std::string discLanguageName(const std::string& language);

// The name a PC file has on a CD's ISO 9660 file system, where names are
// at most 8 characters, a dot and 3 more, in A-Z 0-9 and '_' (others turn
// into '_'): "Language/Streams/FMV 01.bik" -> "LANGUAGE/STREAMS/FMV_01.BIK",
// "Language/Tims/Copy of wat02.TIM" -> "LANGUAGE/TIMS/COPY_OF_.TIM".
std::string cdName(std::string_view pcPath);

// Creates the pack. Existing files in the output folder are overwritten.
// Throws std::runtime_error (or core::FormatError) if an input is missing
// or not what was expected, e.g. a disc of another release.
LanguagePackReport buildLanguagePack(const LanguagePackOptions& options);

} // namespace openhogs::langpack
