// GameFiles.cpp - see GameFiles.h for an overview.

#include "openhogs/apps/GameFiles.h"

#include "openhogs/content/ConfigLocation.h"
#include "openhogs/content/ContentManager.h"

#include <SDL3/SDL.h>

#include <iostream>
#include <stdexcept>

namespace openhogs::apps {

std::filesystem::path findConfig(const std::filesystem::path& requested) {
    std::optional<std::filesystem::path> programFolder;
    if (const char* base = SDL_GetBasePath()) programFolder = std::filesystem::path(base);
    const auto location = content::locateConfig(requested, programFolder);
    if (location.createdFromDefault) {
        std::cout << "Created " << location.path.string()
                  << " from openhogs.default.ini: edit it if your game is not in the Steam folder.\n";
    }
    return location.path;
}

core::VirtualFileSystem openGameFiles(const std::optional<std::filesystem::path>& gameFolder,
                                      const std::filesystem::path& requestedConfig) {
    const auto configPath = gameFolder ? requestedConfig : findConfig(requestedConfig);
    if (gameFolder) {
        core::VirtualFileSystem vfs;
        vfs.mount(content::kGameLayerName, *gameFolder);
        return vfs;
    }
    if (!std::filesystem::exists(configPath)) {
        throw std::runtime_error("Configuration not found: " + configPath.string() +
                                 "\nCopy config/openhogs.default.ini to config/openhogs.ini, or use --game <folder>.");
    }
    auto loaded = content::loadContent(content::ContentConfig::load(configPath));
    for (const auto& pack : loaded.activePacks) {
        std::cout << "Content pack: " << pack.id << "\n";
    }
    return std::move(loaded.files);
}

} // namespace openhogs::apps
