// GameFiles.cpp - see GameFiles.h for an overview.

#include "openhogs/apps/GameFiles.h"

#include "openhogs/content/ContentManager.h"

#include <iostream>
#include <stdexcept>

namespace openhogs::apps {

core::VirtualFileSystem openGameFiles(const std::optional<std::filesystem::path>& gameFolder,
                                      const std::filesystem::path& configPath) {
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
