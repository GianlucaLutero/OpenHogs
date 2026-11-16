// GameVersion.cpp - see GameVersion.h for an overview.

#include "openhogs/game/GameVersion.h"

#include "openhogs/core/PathUtils.h"

#include <string>

namespace openhogs::game {

VersionRules versionRules(GameVersion version) {
    VersionRules rules;
    if (version == GameVersion::PlayStation) {
        rules.computerWalks = true;
        rules.turnTuneLoops = true;
        rules.hardMode = true;
    }
    return rules;
}

std::optional<GameVersion> parseGameVersion(std::string_view text) {
    const std::string name = core::toLower(text);
    if (name == "pc") return GameVersion::Pc;
    if (name == "ps1" || name == "psx" || name == "playstation") return GameVersion::PlayStation;
    return std::nullopt;
}

} // namespace openhogs::game
