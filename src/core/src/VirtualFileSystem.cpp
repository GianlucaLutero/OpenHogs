// VirtualFileSystem.cpp - see VirtualFileSystem.h for an overview.

#include "openhogs/core/VirtualFileSystem.h"

#include "openhogs/core/PathUtils.h"

#include <fstream>
#include <set>
#include <stdexcept>

namespace fs = std::filesystem;

namespace openhogs::core {

void VirtualFileSystem::mount(std::string name, const fs::path& root) {
    if (!fs::is_directory(root)) {
        throw std::runtime_error("Cannot mount '" + name + "': not a folder: " + root.string());
    }

    Layer layer;
    layer.name = std::move(name);
    layer.root = root;

    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        // Hidden files (".gitkeep", ".DS_Store"...) are never game data.
        if (entry.path().filename().string().starts_with('.')) {
            continue;
        }
        // generic_string() always uses '/', which makeLookupKey expects.
        const std::string relative = fs::relative(entry.path(), root).generic_string();
        layer.files[makeLookupKey(relative)] = entry.path();
    }

    layers_.push_back(std::move(layer));
}

const VirtualFileSystem::Layer* VirtualFileSystem::findLayer(const std::string& key) const {
    // Walk from the top layer down: the first match wins.
    for (auto it = layers_.rbegin(); it != layers_.rend(); ++it) {
        if (it->files.contains(key)) {
            return &*it;
        }
    }
    return nullptr;
}

std::optional<fs::path> VirtualFileSystem::resolve(std::string_view relativePath) const {
    const std::string key = makeLookupKey(relativePath);
    if (const Layer* layer = findLayer(key)) {
        return layer->files.at(key);
    }
    return std::nullopt;
}

bool VirtualFileSystem::exists(std::string_view relativePath) const {
    return findLayer(makeLookupKey(relativePath)) != nullptr;
}

std::optional<std::string> VirtualFileSystem::providingLayer(std::string_view relativePath) const {
    if (const Layer* layer = findLayer(makeLookupKey(relativePath))) {
        return layer->name;
    }
    return std::nullopt;
}

std::vector<std::byte> VirtualFileSystem::readFile(std::string_view relativePath) const {
    const auto path = resolve(relativePath);
    if (!path) {
        throw std::runtime_error("File not found in any mounted layer: " + std::string(relativePath));
    }

    std::ifstream file(*path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + path->string());
    }

    const auto size = fs::file_size(*path);
    std::vector<std::byte> data(static_cast<std::size_t>(size));
    file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(size));
    if (!file) {
        throw std::runtime_error("Cannot read file: " + path->string());
    }
    return data;
}

std::vector<std::string> VirtualFileSystem::listFiles(std::string_view directory) const {
    std::string prefix = makeLookupKey(directory);
    if (!prefix.empty() && prefix.back() != '/') {
        prefix.push_back('/');
    }

    // std::set keeps the result sorted and removes files present in many layers.
    std::set<std::string> keys;
    for (const Layer& layer : layers_) {
        for (const auto& [key, path] : layer.files) {
            if (key.starts_with(prefix)) {
                keys.insert(key);
            }
        }
    }
    return {keys.begin(), keys.end()};
}

} // namespace openhogs::core
