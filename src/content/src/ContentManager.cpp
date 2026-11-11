// ContentManager.cpp - see ContentManager.h for an overview.

#include "openhogs/content/ContentManager.h"

#include "openhogs/core/BinaryReader.h"  // FormatError

#include <algorithm>
#include <set>
#include <stdexcept>

namespace fs = std::filesystem;

namespace openhogs::content {

std::vector<ContentPack> discoverPacks(const fs::path& packsDirectory) {
    std::vector<ContentPack> packs;
    if (!fs::is_directory(packsDirectory)) {
        return packs;
    }

    std::set<std::string> seenIds;
    for (const auto& entry : fs::directory_iterator(packsDirectory)) {
        if (!entry.is_directory()) {
            continue;
        }
        const std::string folderName = entry.path().filename().string();
        if (folderName.starts_with('_') || folderName.starts_with('.')) {
            continue;  // templates and hidden folders
        }
        if (!fs::exists(entry.path() / kPackManifestName)) {
            continue;  // not a pack
        }

        ContentPack pack = ContentPack::load(entry.path());
        if (!seenIds.insert(pack.id).second) {
            throw core::FormatError("Two content packs use the same Id '" + pack.id +
                                    "': ids must be unique (see " + entry.path().string() + ")");
        }
        packs.push_back(std::move(pack));
    }

    // directory_iterator order is not specified: sort for stable results.
    std::sort(packs.begin(), packs.end(),
              [](const ContentPack& a, const ContentPack& b) { return a.id < b.id; });
    return packs;
}

std::vector<ContentPack> selectActivePacks(const ContentConfig& config,
                                           const std::vector<ContentPack>& availablePacks) {
    std::vector<ContentPack> active;

    // 1. Optional packs explicitly enabled by id.
    for (const std::string& id : config.enabledPacks) {
        const auto it = std::find_if(availablePacks.begin(), availablePacks.end(),
                                     [&](const ContentPack& pack) { return pack.id == id; });
        if (it == availablePacks.end()) {
            throw std::runtime_error("EnabledPacks lists '" + id +
                                     "', but no pack with that Id was found");
        }
        if (it->type == PackType::Language) {
            // Allowing this would make it possible to mix two languages.
            throw std::runtime_error("'" + id + "' is a language pack: select it with Language = " +
                                     it->language + " instead of EnabledPacks");
        }
        // Ignore duplicates in the list ("a, a") instead of mounting twice.
        const bool alreadyActive = std::any_of(active.begin(), active.end(),
                                               [&](const ContentPack& pack) { return pack.id == id; });
        if (!alreadyActive) {
            active.push_back(*it);
        }
    }

    // 2. Language packs for the selected language.
    bool languageFound = config.language == kOriginalLanguage;
    for (const ContentPack& pack : availablePacks) {
        if (pack.type == PackType::Language && pack.language == config.language) {
            active.push_back(pack);
            languageFound = true;
        }
    }
    if (!languageFound) {
        throw std::runtime_error("Language '" + config.language +
                                 "' was selected, but no language pack provides it");
    }

    // 3. Bottom -> top order. stable_sort + id tie-break keeps it deterministic.
    std::stable_sort(active.begin(), active.end(), [](const ContentPack& a, const ContentPack& b) {
        if (a.priority != b.priority) {
            return a.priority < b.priority;
        }
        return a.id < b.id;
    });
    return active;
}

core::VirtualFileSystem buildFileSystem(const ContentConfig& config,
                                        const std::vector<ContentPack>& activePacks) {
    core::VirtualFileSystem vfs;
    vfs.mount(kGameLayerName, config.gameDirectory);

    for (const ContentPack& pack : activePacks) {
        if (!fs::is_directory(pack.dataDirectory)) {
            throw std::runtime_error("Content pack '" + pack.id + "' has no '" +
                                     kPackDataFolderName + "' folder: " + pack.directory.string());
        }
        vfs.mount(pack.id, pack.dataDirectory);
    }
    return vfs;
}

LoadedContent loadContent(const ContentConfig& config) {
    LoadedContent content;
    content.activePacks = selectActivePacks(config, discoverPacks(config.packsDirectory));
    content.files = buildFileSystem(config, content.activePacks);
    return content;
}

} // namespace openhogs::content
