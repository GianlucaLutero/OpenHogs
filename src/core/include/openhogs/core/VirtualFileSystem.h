// VirtualFileSystem.h
//
// A layered, case-insensitive view over several folders.
//
// This is what makes OpenHogs content replaceable. The original game folder
// is mounted first (the bottom layer); content packs such as a language pack
// or a music pack are mounted on top of it. When the engine asks for
//
//     "Speech/Sku1/Pig01/01AM0101.wav"
//
// the file is taken from the highest layer that contains it. A pack therefore
// only needs to contain the files it changes, and disabling a pack is just a
// matter of not mounting it: the original files are never modified.
//
// Lookups ignore case and accept both '/' and '\' (see makeLookupKey), so
// the Windows-era names used inside the game data resolve on every OS.

#pragma once

#include <cstddef>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace openhogs::core {

class VirtualFileSystem {
public:
    // One mounted folder.
    struct Layer {
        std::string name;               // shown in logs, e.g. "game" or "lang-it"
        std::filesystem::path root;     // real folder on disk
        // lookup key ("maps/camp.mad") -> real path on disk
        std::map<std::string, std::filesystem::path> files;
    };

    // Mounts `root` on top of all layers mounted so far, so its files win
    // over earlier ones. The folder is scanned once, here, which keeps later
    // lookups fast. Hidden files (names starting with '.') are ignored.
    // Throws std::runtime_error if `root` is not a folder.
    void mount(std::string name, const std::filesystem::path& root);

    // Returns the real path of the file, or std::nullopt if no layer has it.
    std::optional<std::filesystem::path> resolve(std::string_view relativePath) const;

    bool exists(std::string_view relativePath) const;

    // Name of the layer that provides the file (useful to explain to users
    // where a file comes from), or std::nullopt if it does not exist.
    std::optional<std::string> providingLayer(std::string_view relativePath) const;

    // Reads the whole file. Throws std::runtime_error if it is missing or
    // cannot be read.
    std::vector<std::byte> readFile(std::string_view relativePath) const;

    // Lookup keys of all files inside `directory` (and its sub-folders),
    // merged across layers, sorted and without duplicates.
    std::vector<std::string> listFiles(std::string_view directory) const;

    // Layers from bottom (lowest priority) to top (highest priority).
    const std::vector<Layer>& layers() const { return layers_; }

private:
    // Returns the top-most layer containing `key`, or nullptr.
    const Layer* findLayer(const std::string& key) const;

    std::vector<Layer> layers_;
};

} // namespace openhogs::core
