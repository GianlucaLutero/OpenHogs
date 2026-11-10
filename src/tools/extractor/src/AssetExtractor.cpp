// AssetExtractor.cpp - see AssetExtractor.h for an overview.

#include "openhogs/extractor/AssetExtractor.h"

#include "openhogs/core/PathUtils.h"
#include "openhogs/extractor/PngWriter.h"
#include "openhogs/formats/PackageArchive.h"
#include "openhogs/formats/TextTable.h"
#include "openhogs/formats/TimImage.h"

#include <cstdio>
#include <exception>
#include <fstream>
#include <set>
#include <stdexcept>

namespace fs = std::filesystem;

namespace openhogs::extractor {

namespace {

bool hasExtension(const std::string& key, std::string_view extension) {
    // Keys are lower case, so a plain comparison is enough.
    return key.size() > extension.size() && key.ends_with(extension);
}

void writeBytes(const fs::path& path, std::span<const std::byte> data) {
    fs::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!file) {
        throw std::runtime_error("Cannot write " + path.string());
    }
}

void writeText(const fs::path& path, const std::string& text) {
    writeBytes(path, std::as_bytes(std::span(text.data(), text.size())));
}

// "[0042] DEATH VALLEY": the index is what the engine uses to find a string,
// so it is kept visible for translators.
std::string formatTextTable(const formats::TextTable& table) {
    std::string text;
    char index[32];  // large enough for any std::size_t
    for (std::size_t i = 0; i < table.entries.size(); ++i) {
        std::snprintf(index, sizeof(index), "[%04zu] ", i);
        text += index;
        text += table.entries[i];
        text += '\n';
    }
    return text;
}

} // namespace

AssetExtractor::AssetExtractor(fs::path outputDirectory)
    : outputDirectory_(std::move(outputDirectory)) {}

std::string AssetExtractor::makeSafeFileName(std::string_view entryName) {
    std::string name;
    for (const char c : entryName) {
        const bool unsafe = c == '/' || c == '\\' || c == ':' ||
                            static_cast<unsigned char>(c) < 0x20;
        name.push_back(unsafe ? '_' : c);
    }
    // "", "." and ".." would refer to the folder itself or to its parent.
    if (name.find_first_not_of('.') == std::string::npos) {
        name = "_" + name;
    }
    return name;
}

std::string AssetExtractor::makeUniqueFileName(const std::string& fileName,
                                               std::set<std::string>& usedNames) {
    // Names are compared in lower case because Windows file names are
    // case-insensitive: "BOX.TIM" and "box.tim" would be the same file.
    if (usedNames.insert(core::toLower(fileName)).second) {
        return fileName;
    }

    // "BOX.TIM" -> "BOX~2.TIM", "BOX~3.TIM", ...
    const auto dot = fileName.rfind('.');
    const std::string stem = fileName.substr(0, dot);
    const std::string extension = dot == std::string::npos ? "" : fileName.substr(dot);
    for (int copy = 2;; ++copy) {
        std::string candidate = stem + "~" + std::to_string(copy) + extension;
        if (usedNames.insert(core::toLower(candidate)).second) {
            return candidate;
        }
    }
}

ExtractionReport AssetExtractor::extractAll(const core::VirtualFileSystem& vfs) {
    ExtractionReport report;

    for (const std::string& key : vfs.listFiles("")) {
        // One failing file must not stop the whole extraction.
        try {
            if (hasExtension(key, ".mad") || hasExtension(key, ".mtd")) {
                extractArchive(vfs, key, report);
            } else if (hasExtension(key, ".tim")) {
                extractImage(vfs, key, report);
            } else if (hasExtension(key, ".ofs")) {
                extractTextTable(vfs, key, report);
            }
        } catch (const std::exception& error) {
            report.problems.push_back(key + ": " + error.what());
        }
    }
    return report;
}

void AssetExtractor::extractArchive(const core::VirtualFileSystem& vfs, const std::string& key,
                                    ExtractionReport& report) {
    auto data = vfs.readFile(key);
    if (!formats::PackageArchive::looksLikePackage(data)) {
        // e.g. Chars/mcap.mad: same extension, different content.
        report.problems.push_back(key + ": not a package archive, skipped");
        return;
    }

    const auto archive = formats::PackageArchive::parse(std::move(data));
    const fs::path folder = outputDirectory_ / key;
    std::set<std::string> usedNames;

    for (const auto& entry : archive.entries()) {
        const std::string fileName = makeUniqueFileName(makeSafeFileName(entry.name), usedNames);
        const auto bytes = archive.entryData(entry);
        writeBytes(folder / fileName, bytes);
        ++report.archiveEntries;

        if (hasExtension(core::toLower(fileName), ".tim")) {
            try {
                writeBytes(folder / (fileName + ".png"), encodePng(formats::decodeTim(bytes)));
                ++report.images;
            } catch (const std::exception& error) {
                report.problems.push_back(key + "/" + entry.name + ": " + error.what());
            }
        }
    }
    ++report.archives;
}

void AssetExtractor::extractImage(const core::VirtualFileSystem& vfs, const std::string& key,
                                  ExtractionReport& report) {
    const auto data = vfs.readFile(key);
    writeBytes(outputDirectory_ / (key + ".png"), encodePng(formats::decodeTim(data)));
    ++report.images;
}

void AssetExtractor::extractTextTable(const core::VirtualFileSystem& vfs, const std::string& offsetsKey,
                                      ExtractionReport& report) {
    // "language/text/gtext.ofs" pairs with "language/text/gtext.bin".
    const std::string baseKey = offsetsKey.substr(0, offsetsKey.size() - 4);
    const std::string stringsKey = baseKey + ".bin";
    if (!vfs.exists(stringsKey)) {
        return;  // an .ofs without its .bin is not a text table
    }

    const auto table = formats::TextTable::parse(vfs.readFile(offsetsKey), vfs.readFile(stringsKey));
    writeText(outputDirectory_ / (baseKey + ".txt"), formatTextTable(table));
    ++report.textTables;
}

} // namespace openhogs::extractor
