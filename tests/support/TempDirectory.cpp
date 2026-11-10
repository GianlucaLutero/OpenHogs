// TempDirectory.cpp - see TempDirectory.h for an overview.

#include "TempDirectory.h"

#include <fstream>
#include <random>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

namespace openhogs::tests {

TempDirectory::TempDirectory() {
    // A random suffix keeps parallel test runs from sharing folders.
    std::random_device device;
    std::mt19937_64 generator(device());
    path_ = fs::temp_directory_path() / ("openhogs-test-" + std::to_string(generator()));
    fs::create_directories(path_);
}

TempDirectory::~TempDirectory() {
    std::error_code ignored;  // never throw from a destructor
    fs::remove_all(path_, ignored);
}

fs::path TempDirectory::writeFile(const fs::path& relativePath, std::string_view content) const {
    const fs::path fullPath = path_ / relativePath;
    fs::create_directories(fullPath.parent_path());

    std::ofstream file(fullPath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Test helper cannot create " + fullPath.string());
    }
    file << content;
    return fullPath;
}

fs::path TempDirectory::makeFolder(const fs::path& relativePath) const {
    const fs::path fullPath = path_ / relativePath;
    fs::create_directories(fullPath);
    return fullPath;
}

} // namespace openhogs::tests
