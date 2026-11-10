// AssetExtractor.h
//
// Unpacks the original game data into ordinary files.
//
// What it produces, for every file visible in the virtual file system:
//
//   .MAD / .MTD archives  -> a folder named after the archive with every
//                            entry saved as-is (repeated names get a "~2"
//                            suffix); .TIM entries are also converted to
//                            .png next to the original
//   .TIM images           -> .png
//   .OFS + .BIN tables    -> one .txt file, one "[index] text" line per string
//
// Output paths use the lower-case lookup keys of the virtual file system
// (e.g. "maps/camp.mad/STF02PPP.VTX"), so the result looks the same on
// every operating system. Other files are already readable and are skipped.
//
// A broken file does not stop the extraction: the problem is recorded in
// the report and the extractor moves on.

#pragma once

#include "openhogs/core/VirtualFileSystem.h"

#include <cstddef>
#include <filesystem>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace openhogs::extractor {

struct ExtractionReport {
    std::size_t archives = 0;        // archives unpacked
    std::size_t archiveEntries = 0;  // files written from archives
    std::size_t images = 0;          // PNG images written
    std::size_t textTables = 0;      // text tables written
    std::vector<std::string> problems;  // one message per file that failed
};

class AssetExtractor {
public:
    explicit AssetExtractor(std::filesystem::path outputDirectory);

    // Extracts every supported file from `vfs`.
    ExtractionReport extractAll(const core::VirtualFileSystem& vfs);

    // Archive entry names come from the files themselves; this turns them
    // into a safe single file name (no folders, no "..", no drive letters),
    // so a crafted archive can never write outside the output folder.
    static std::string makeSafeFileName(std::string_view entryName);

    // Some archives contain several entries with the same name (about 1,500
    // in the original data). To keep every one of them, a repeated name gets
    // a "~2", "~3"... suffix before its extension. `usedNames` holds the
    // names already given out in the current folder.
    static std::string makeUniqueFileName(const std::string& fileName,
                                          std::set<std::string>& usedNames);

private:
    void extractArchive(const core::VirtualFileSystem& vfs, const std::string& key,
                        ExtractionReport& report);
    void extractImage(const core::VirtualFileSystem& vfs, const std::string& key,
                      ExtractionReport& report);
    void extractTextTable(const core::VirtualFileSystem& vfs, const std::string& offsetsKey,
                          ExtractionReport& report);

    std::filesystem::path outputDirectory_;
};

} // namespace openhogs::extractor
