// ConfigLocation.cpp - see ConfigLocation.h for an overview.

#include "openhogs/content/ConfigLocation.h"

#include <system_error>
#include <vector>

namespace openhogs::content {

namespace {

constexpr const char* kDefaultConfigName = "openhogs.default.ini";

bool fileExists(const std::filesystem::path& path) {
    std::error_code error;
    return std::filesystem::exists(path, error);
}

} // namespace

ConfigLocation locateConfig(const std::filesystem::path& requested,
                            const std::optional<std::filesystem::path>& programFolder) {
    // Where to look: as given (from the current folder), then beside the
    // program for a relative path.
    std::vector<std::filesystem::path> candidates{requested};
    if (programFolder && requested.is_relative()) candidates.push_back(*programFolder / requested);

    // The user's own file wins wherever it is...
    for (const auto& candidate : candidates) {
        if (fileExists(candidate)) return {candidate, false};
    }
    // ...else it is made from the first default found.
    for (const auto& candidate : candidates) {
        const auto defaults = candidate.parent_path() / kDefaultConfigName;
        std::error_code error;
        if (fileExists(defaults) &&
            std::filesystem::copy_file(defaults, candidate, std::filesystem::copy_options::skip_existing, error)) {
            return {candidate, true};
        }
    }
    return {requested, false};
}

} // namespace openhogs::content
