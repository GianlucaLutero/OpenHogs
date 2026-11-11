// LanguagePackBuilder.cpp - see LanguagePackBuilder.h for an overview.

#include "openhogs/langpack/LanguagePackBuilder.h"

#include "openhogs/core/PathUtils.h"
#include "openhogs/core/VirtualFileSystem.h"
#include "openhogs/formats/GameText.h"
#include "openhogs/formats/PsxDisc.h"
#include "openhogs/formats/RawImage.h"
#include "openhogs/formats/TextTable.h"
#include "openhogs/langpack/Bitmap.h"
#include "openhogs/langpack/TextAlignment.h"
#include "openhogs/langpack/TranslationOverrides.h"

#include <array>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace fs = std::filesystem;

namespace openhogs::langpack {

namespace {

// PlayStation briefings are raw 15-bit pictures with no header, so their
// size is fixed here: the full-screen briefings (LEVEL<n>.BIN, 262,144
// bytes) are 512 x 256; the six small pictures of the first mission
// (LEVEL1N<k>.BIN, 40,400 bytes) are 202 x 100. Both sizes were checked by
// decoding the files of the European disc.
constexpr std::uint32_t kBriefingWidth = 512;
constexpr std::uint32_t kBriefingHeight = 256;
constexpr std::uint32_t kSmallBriefingWidth = 202;
constexpr std::uint32_t kSmallBriefingHeight = 100;

constexpr std::array<std::pair<const char*, const char*>, 4> kLanguages{{
    {"it", "ITALIAN"}, {"fr", "FRENCH"}, {"de", "GERMAN"}, {"es", "SPANISH"}}};

// Where each text table lives on the PC install and on the disc.
struct TableSource {
    const char* name;          // "gtext" / "fetext" (also the overrides section)
    const char* pcBase;        // PC files without extension
    std::string discEnglish;   // disc files without extension
    std::string discTranslated;
};

void writeFile(const fs::path& path, std::span<const std::byte> data) {
    fs::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!file) {
        throw std::runtime_error("Cannot write " + path.string());
    }
}

std::vector<std::string> readPcTable(const core::VirtualFileSystem& game, const std::string& base) {
    return formats::TextTable::parse(game.readFile(base + ".ofs"), game.readFile(base + ".bin")).entries;
}

std::vector<std::string> readDiscTable(const formats::PsxDisc& disc, const std::string& base) {
    return formats::TextTable::parse(disc.readFile(base + ".OFS"), disc.readFile(base + ".BIN")).entries;
}

TableReport buildTextTable(const TableSource& source, const core::VirtualFileSystem& game,
                           const formats::PsxDisc& disc, const TranslationOverrides* overrides,
                           const fs::path& dataDirectory) {
    const auto pcEnglish = readPcTable(game, source.pcBase);
    auto aligned = alignTranslation(pcEnglish, readDiscTable(disc, source.discEnglish),
                                    readDiscTable(disc, source.discTranslated));

    TableReport report;
    report.total = pcEnglish.size();
    report.translated = report.total - aligned.untranslated.size();
    if (overrides != nullptr) {
        report.overridden = overrides->apply(source.name, aligned.entries);
    }
    // An entry is still English if it was neither aligned nor overridden.
    for (const std::size_t index : aligned.untranslated) {
        if (aligned.entries[index] == pcEnglish[index]) {
            report.untranslated.push_back(index);
        }
    }

    formats::TextTable table;
    table.entries = std::move(aligned.entries);
    const auto encoded = table.encode();
    writeFile(dataDirectory / (std::string(source.pcBase) + ".ofs"), encoded.offsets);
    writeFile(dataDirectory / (std::string(source.pcBase) + ".bin"), encoded.strings);
    return report;
}

// Where a PC briefing picture comes from on the disc.
struct BriefingSource {
    std::string discName;  // e.g. "LEVEL12.BIN"
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

// "language/tims/briefing/level12.bmp" -> LEVEL12.BIN (512 x 256),
// "language/tims/briefing/level1n3.bmp" -> LEVEL1N3.BIN (202 x 100).
// Other files of the folder (e.g. the loading bar) have no disc version.
std::optional<BriefingSource> briefingSource(const std::string& key) {
    const std::string prefix = "language/tims/briefing/level";
    if (!key.starts_with(prefix) || !key.ends_with(".bmp")) {
        return std::nullopt;
    }
    const std::string id = key.substr(prefix.size(), key.size() - prefix.size() - 4);  // "12", "1n3"
    const bool allDigits = !id.empty() && id.find_first_not_of("0123456789") == std::string::npos;
    const bool smallPicture = id.size() == 3 && id[0] == '1' && id[1] == 'n' && id[2] >= '0' && id[2] <= '9';
    if (!allDigits && !smallPicture) {
        return std::nullopt;
    }

    std::string discName = "LEVEL";
    for (const char c : id) {
        discName.push_back(c == 'n' ? 'N' : c);
    }
    discName += ".BIN";
    if (smallPicture) {
        return BriefingSource{discName, kSmallBriefingWidth, kSmallBriefingHeight};
    }
    return BriefingSource{discName, kBriefingWidth, kBriefingHeight};
}

std::size_t buildBriefings(const core::VirtualFileSystem& game, const formats::PsxDisc& disc,
                           const std::string& discLanguage, const fs::path& dataDirectory,
                           std::vector<std::string>& warnings) {
    std::size_t count = 0;
    for (const auto& key : game.listFiles("language/tims/briefing")) {
        const auto source = briefingSource(key);
        if (!source) {
            continue;
        }
        const std::string discPath = "BRIEF/" + discLanguage + "/" + source->discName;
        if (!disc.exists(discPath)) {
            warnings.push_back("No " + discLanguage + " briefing for " + key + " on the disc: kept the original");
            continue;
        }

        // Same size as the PC picture, so the game shows it the same way.
        const auto pcSize = readBitmapSize(game.readFile(key));
        const auto picture = formats::decodeRaw15(disc.readFile(discPath), source->width, source->height);
        const auto scaled = resizeBilinear(picture, pcSize.width, pcSize.height);

        // Keep the PC file name (with its original letter case).
        const fs::path original = *game.resolve(key);
        writeFile(dataDirectory / "Language" / "Tims" / "Briefing" / original.filename(), encodeBitmap24(scaled));
        ++count;
    }
    return count;
}

void writeManifest(const LanguagePackOptions& options, const std::string& discLanguage) {
    std::string name = discLanguage.substr(0, 1) + core::toLower(discLanguage.substr(1));
    const std::string manifest =
        "; Generated by openhogs-langpack from your own PlayStation disc.\n"
        "; Contains copyrighted game data: do not share or commit this folder.\n"
        "[Pack]\n"
        "Id          = lang-" + options.language + "\n"
        "Name        = " + name + " (from the PlayStation version)\n"
        "Type        = language\n"
        "Language    = " + options.language + "\n"
        "Priority    = 100\n"
        "Description = Text, menus and briefings of the PlayStation release. Speech stays in English, "
        "as on the PlayStation.\n";
    const auto bytes = std::as_bytes(std::span(manifest.data(), manifest.size()));
    writeFile(options.outputDirectory / "pack.ini", bytes);
}

} // namespace

std::string discLanguageName(const std::string& language) {
    for (const auto& [code, name] : kLanguages) {
        if (core::toLower(language) == code) {
            return name;
        }
    }
    throw std::invalid_argument("Language '" + language +
                                "' is not on the PlayStation disc (available: it, fr, de, es)");
}

LanguagePackReport buildLanguagePack(const LanguagePackOptions& options) {
    const std::string discLanguage = discLanguageName(options.language);

    core::VirtualFileSystem game;
    game.mount("game", options.gameDirectory);
    const formats::PsxDisc disc(options.discImage);
    if (!disc.exists("TEXT/G" + discLanguage + ".BIN")) {
        throw std::runtime_error("This disc has no " + discLanguage +
                                 " text: is it the European PlayStation release (SLES-01041)?");
    }

    std::optional<TranslationOverrides> overrides;
    if (options.overridesFile && fs::exists(*options.overridesFile)) {
        overrides = TranslationOverrides::load(*options.overridesFile);
    }
    const TranslationOverrides* overridesPointer = overrides ? &*overrides : nullptr;

    const fs::path data = options.outputDirectory / "data";
    LanguagePackReport report;

    const TableSource gameText{"gtext", "Language/Text/gtext", "TEXT/GENGLISH", "TEXT/G" + discLanguage};
    const TableSource menuText{"fetext", "Language/Text/fetext", "TEXT/FETEXT/ENGLISH",
                               "TEXT/FETEXT/" + discLanguage};
    report.gameText = buildTextTable(gameText, game, disc, overridesPointer, data);
    report.menuText = buildTextTable(menuText, game, disc, overridesPointer, data);
    report.briefings = buildBriefings(game, disc, discLanguage, data, report.warnings);

    writeManifest(options, discLanguage);
    return report;
}

} // namespace openhogs::langpack
