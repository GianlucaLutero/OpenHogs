// openhogs-extract - command-line front end of the asset extractor.
//
// Usage:
//   openhogs-extract <game-folder> <output-folder>
//       Extracts archives, textures and text tables from the game folder.
//
//   openhogs-extract --list <archive-file>
//       Prints the entries of one .MAD / .MTD archive.

#include "openhogs/extractor/AssetExtractor.h"
#include "openhogs/formats/PackageArchive.h"

#include <cstdio>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace openhogs;

namespace {

void printUsage() {
    std::cout << "OpenHogs asset extractor\n\n"
                 "Usage:\n"
                 "  openhogs-extract <game-folder> <output-folder>\n"
                 "      Extract archives, textures (PNG) and text tables.\n\n"
                 "  openhogs-extract --list <archive-file>\n"
                 "      List the files inside a .MAD or .MTD archive.\n";
}

std::vector<std::byte> readWholeFile(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open " + path.string());
    }
    const std::vector<char> chars((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::vector<std::byte> bytes(chars.size());
    std::memcpy(bytes.data(), chars.data(), chars.size());
    return bytes;
}

int listArchive(const fs::path& archivePath) {
    const auto archive = formats::PackageArchive::parse(readWholeFile(archivePath));
    std::printf("%-16s %10s %10s\n", "NAME", "OFFSET", "SIZE");
    for (const auto& entry : archive.entries()) {
        std::printf("%-16s %10u %10u\n", entry.name.c_str(), entry.offset, entry.size);
    }
    std::printf("%zu entries\n", archive.entries().size());
    return 0;
}

int extractGame(const fs::path& gameFolder, const fs::path& outputFolder) {
    core::VirtualFileSystem vfs;
    vfs.mount("game", gameFolder);

    std::cout << "Extracting " << gameFolder.string() << "\n     into " << outputFolder.string() << "\n";
    extractor::AssetExtractor extractor(outputFolder);
    const auto report = extractor.extractAll(vfs);

    std::cout << "\nArchives:     " << report.archives
              << "\nFiles:        " << report.archiveEntries
              << "\nImages (PNG): " << report.images
              << "\nText tables:  " << report.textTables << "\n";

    if (!report.problems.empty()) {
        std::cout << "\n" << report.problems.size() << " problem(s):\n";
        for (const auto& problem : report.problems) {
            std::cout << "  - " << problem << "\n";
        }
    }
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    const std::vector<std::string> args(argv + 1, argv + argc);

    try {
        if (args.size() == 2 && args[0] == "--list") {
            return listArchive(args[1]);
        }
        if (args.size() == 2 && !args[0].starts_with("--")) {
            return extractGame(args[0], args[1]);
        }
        printUsage();
        return args.empty() || args[0] == "--help" ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }
}
