// TempDirectory.h
//
// Test helper: a uniquely named folder in the system temp directory that is
// deleted automatically when the object goes out of scope.
//
// Tests for the file system and content packs need real folders, but must
// never depend on the user's game installation or leave files behind.

#pragma once

#include <filesystem>
#include <string_view>

namespace openhogs::tests {

class TempDirectory {
public:
    TempDirectory();
    ~TempDirectory();

    TempDirectory(const TempDirectory&) = delete;
    TempDirectory& operator=(const TempDirectory&) = delete;

    const std::filesystem::path& path() const { return path_; }

    // Creates a text file (and its parent folders) at `relativePath`.
    // Returns the full path of the new file.
    std::filesystem::path writeFile(const std::filesystem::path& relativePath,
                                    std::string_view content) const;

    // Creates a folder (and its parents). Returns its full path.
    std::filesystem::path makeFolder(const std::filesystem::path& relativePath) const;

private:
    std::filesystem::path path_;
};

} // namespace openhogs::tests
