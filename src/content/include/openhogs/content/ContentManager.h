// ContentManager.h
//
// Turns the user's settings into the set of files the game will see.
//
// The process has three explicit steps, so each one can be tested and shown
// to the user (e.g. "these packs are active") on its own:
//
//   1. discoverPacks      - find every pack in the packs folder
//   2. selectActivePacks  - decide which packs are on, and in which order
//   3. buildFileSystem    - mount the game folder, then the active packs
//
// Selection rules:
//   - The language pack(s) whose Language matches [Content] Language are on.
//     Choosing "en" (the original language) needs no pack.
//   - Every other pack is on only if its id is listed in EnabledPacks.
//   - Active packs are mounted by ascending Priority (ties: by id), so the
//     pack with the highest Priority wins when two packs replace the same file.

#pragma once

#include "openhogs/content/ContentConfig.h"
#include "openhogs/content/ContentPack.h"
#include "openhogs/core/VirtualFileSystem.h"

#include <filesystem>
#include <vector>

namespace openhogs::content {

// Name of the bottom layer (the original installation) in the VFS.
inline constexpr const char* kGameLayerName = "game";

// Loads every sub-folder of `packsDirectory` that contains a pack.ini.
// Folders whose name starts with '_' (e.g. "_template") are skipped, so
// examples can live next to real packs. A missing packs folder simply means
// "no packs". Throws FormatError on invalid manifests or duplicate ids.
std::vector<ContentPack> discoverPacks(const std::filesystem::path& packsDirectory);

// Applies the selection rules above. Returns packs ordered bottom -> top.
// Throws std::runtime_error with a user-friendly message when the config
// asks for something that does not exist (unknown pack id, a language
// without a pack, a language pack listed in EnabledPacks).
std::vector<ContentPack> selectActivePacks(const ContentConfig& config,
                                           const std::vector<ContentPack>& availablePacks);

// Mounts the game folder and then each active pack's data folder.
core::VirtualFileSystem buildFileSystem(const ContentConfig& config,
                                        const std::vector<ContentPack>& activePacks);

// The three steps above in one call, as every application needs them.
struct LoadedContent {
    core::VirtualFileSystem files;
    std::vector<ContentPack> activePacks;  // bottom -> top
};
LoadedContent loadContent(const ContentConfig& config);

} // namespace openhogs::content
