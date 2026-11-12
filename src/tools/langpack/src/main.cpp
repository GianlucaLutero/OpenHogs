// openhogs-langpack - builds a language pack from your PlayStation disc.
//
// Usage:
//   openhogs-langpack --disc <track1.bin|.iso> --game <PC folder> [options]
//
// Options:
//   --language <code>     it (default), fr, de or es
//   --out <folder>        pack folder (default: content/packs/lang-<code>)
//   --overrides <file>    hand-written translations
//                         (default: content/translations/<code>.ini, if present)
//
// Then set "Language = <code>" in config/openhogs.ini.

#include "openhogs/langpack/LanguagePackBuilder.h"

#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace openhogs::langpack;

namespace {

void printUsage() {
    std::cout << "OpenHogs language pack builder\n\n"
                 "Usage: openhogs-langpack --disc <track1.bin|.iso> --game <PC folder> [options]\n\n"
                 "  --language <code>   it (default), fr, de or es\n"
                 "  --out <folder>      pack folder (default: content/packs/lang-<code>)\n"
                 "  --overrides <file>  hand-written translations\n"
                 "                      (default: content/translations/<code>.ini, if present)\n\n"
                 "The pack contains data from your own disc: do not share it.\n";
}

std::optional<LanguagePackOptions> parseArguments(const std::vector<std::string>& args) {
    LanguagePackOptions options;
    std::optional<fs::path> out;

    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];
        if (i + 1 >= args.size()) {
            return std::nullopt;  // every option takes a value
        }
        const std::string& value = args[++i];
        if (arg == "--disc") {
            options.discImage = value;
        } else if (arg == "--game") {
            options.gameDirectory = value;
        } else if (arg == "--language") {
            options.language = value;
        } else if (arg == "--out") {
            out = value;
        } else if (arg == "--overrides") {
            options.overridesFile = value;
        } else {
            return std::nullopt;
        }
    }
    if (options.discImage.empty() || options.gameDirectory.empty()) {
        return std::nullopt;
    }

    // Defaults that depend on the language.
    options.outputDirectory = out.value_or(fs::path("content") / "packs" / ("lang-" + options.language));
    if (!options.overridesFile) {
        const fs::path standard = fs::path("content") / "translations" / (options.language + ".ini");
        if (fs::exists(standard)) {
            options.overridesFile = standard;
        }
    }
    return options;
}

void printTableReport(const char* title, const TableReport& report) {
    std::cout << title << ": " << report.translated << " of " << report.total << " strings from the disc, "
              << report.overridden << " from the overrides, " << report.untranslated.size()
              << " left in English\n";
    if (!report.untranslated.empty()) {
        std::cout << "  still in English:";
        for (const std::size_t index : report.untranslated) {
            std::cout << ' ' << index;
        }
        std::cout << "\n";
    }
}

} // namespace

int main(int argc, char* argv[]) {
    const std::vector<std::string> args(argv + 1, argv + argc);
    const auto options = parseArguments(args);
    if (!options) {
        printUsage();
        return args.empty() || args[0] == "--help" ? 0 : 1;
    }

    try {
        std::cout << "Building " << options->outputDirectory.string() << " from " << options->discImage.string()
                  << "\n";
        if (options->overridesFile) {
            std::cout << "Overrides: " << options->overridesFile->string() << "\n";
        }

        const auto report = buildLanguagePack(*options);

        std::cout << "\n";
        printTableReport("Game text", report.gameText);
        printTableReport("Menu text", report.menuText);
        std::cout << "Briefings: " << report.briefings << "\n";
        std::cout << "Title screens, newspapers and headlines: " << report.pictures << "\n";
        for (const auto& warning : report.warnings) {
            std::cout << "warning: " << warning << "\n";
        }
        std::cout << "\nDone. Set \"Language = " << options->language
                  << "\" in config/openhogs.ini to use the pack.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }
}
