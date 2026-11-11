// openhogs-text - converts text tables to editable text and back.
//
// Usage:
//   openhogs-text export <table> <listing.txt>
//       Reads <table>.ofs and <table>.bin and writes a UTF-8 listing,
//       one "[index] TEXT" line per string.
//
//   openhogs-text import <listing.txt> <table>
//       Reads a listing and writes <table>.ofs and <table>.bin.
//
// <table> is the path without extension, e.g. "Language/Text/gtext".
// Accented capitals are written as normal UTF-8 letters (À, È...); icons
// the font draws instead of letters are written as {xx} codes.

#include "openhogs/formats/TextTable.h"

#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using openhogs::formats::TextTable;

namespace {

void printUsage() {
    std::cout << "OpenHogs text table tool\n\n"
                 "Usage:\n"
                 "  openhogs-text export <table> <listing.txt>   e.g. export Language/Text/gtext gtext.txt\n"
                 "  openhogs-text import <listing.txt> <table>   writes <table>.ofs and <table>.bin\n";
}

std::vector<std::byte> readBytes(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open " + path.string());
    }
    const std::vector<char> chars((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::vector<std::byte> bytes(chars.size());
    std::memcpy(bytes.data(), chars.data(), chars.size());
    return bytes;
}

void writeBytes(const fs::path& path, const void* data, std::size_t size) {
    std::ofstream file(path, std::ios::binary);
    file.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
    if (!file) {
        throw std::runtime_error("Cannot write " + path.string());
    }
}

// Appends an extension to a path without replacing what follows the last
// dot ("gtext" -> "gtext.ofs", but also "my.table" -> "my.table.ofs").
fs::path withExtension(const fs::path& base, const char* extension) {
    return fs::path(base.string() + extension);
}

int exportTable(const fs::path& table, const fs::path& listing) {
    const auto parsed = TextTable::parse(readBytes(withExtension(table, ".ofs")), readBytes(withExtension(table, ".bin")));
    const std::string text = parsed.toListing();
    writeBytes(listing, text.data(), text.size());
    std::cout << "Wrote " << parsed.entries.size() << " strings to " << listing.string() << "\n";
    return 0;
}

int importTable(const fs::path& listing, const fs::path& table) {
    const auto bytes = readBytes(listing);
    const std::string text(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    const auto parsed = TextTable::fromListing(text);
    const auto encoded = parsed.encode();
    writeBytes(withExtension(table, ".ofs"), encoded.offsets.data(), encoded.offsets.size());
    writeBytes(withExtension(table, ".bin"), encoded.strings.data(), encoded.strings.size());
    std::cout << "Wrote " << parsed.entries.size() << " strings to " << table.string() << ".ofs/.bin\n";
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    const std::vector<std::string> args(argv + 1, argv + argc);
    try {
        if (args.size() == 3 && args[0] == "export") {
            return exportTable(args[1], args[2]);
        }
        if (args.size() == 3 && args[0] == "import") {
            return importTable(args[1], args[2]);
        }
        printUsage();
        return args.empty() || args[0] == "--help" ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }
}
