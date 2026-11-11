// GameFiles.h
//
// Opens the game files for the command-line applications, the same way in
// every tool:
//
//   --game <folder>   the bare game folder (no configuration, no packs)
//   otherwise         the configuration file (config/openhogs.ini by
//                     default): the game folder plus the content packs it
//                     enables
//
// The active packs are printed so the user knows what is being shown.

#pragma once

#include "openhogs/core/VirtualFileSystem.h"

#include <filesystem>
#include <optional>

namespace openhogs::apps {

// Throws std::runtime_error with a hint if the configuration is missing.
core::VirtualFileSystem openGameFiles(const std::optional<std::filesystem::path>& gameFolder,
                                      const std::filesystem::path& configPath);

} // namespace openhogs::apps
