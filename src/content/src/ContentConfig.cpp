// ContentConfig.cpp - see ContentConfig.h for an overview.

#include "openhogs/content/ContentConfig.h"

#include "openhogs/core/BinaryReader.h"  // FormatError
#include "openhogs/core/IniFile.h"
#include "openhogs/core/PathUtils.h"

namespace fs = std::filesystem;

namespace openhogs::content {

namespace {

// Default location of the packs, relative to the config folder:
//   <OpenHogs>/config/openhogs.ini  ->  <OpenHogs>/content/packs
constexpr const char* kDefaultPacksDirectory = "../content/packs";

fs::path resolveAgainst(const fs::path& base, const std::string& value) {
    const fs::path path(value);
    return path.is_absolute() ? path : (base / path).lexically_normal();
}

} // namespace

ContentConfig ContentConfig::fromIni(const core::IniFile& ini, const fs::path& baseDirectory) {
    ContentConfig config;

    const auto gameDirectory = ini.get("Game", "Directory");
    if (!gameDirectory || gameDirectory->empty()) {
        throw core::FormatError(
            "[Game] Directory is not set: point it to your Hogs of War installation");
    }
    config.gameDirectory = resolveAgainst(baseDirectory, *gameDirectory);

    config.packsDirectory = resolveAgainst(
        baseDirectory, ini.getString("Content", "PacksDirectory", kDefaultPacksDirectory));

    // Language codes and pack ids are compared in lower case everywhere.
    config.language = core::toLower(ini.getString("Content", "Language", kOriginalLanguage));

    for (const std::string& id : ini.getList("Content", "EnabledPacks")) {
        config.enabledPacks.push_back(core::toLower(id));
    }

    return config;
}

ContentConfig ContentConfig::load(const fs::path& iniPath) {
    const auto ini = core::IniFile::load(iniPath);
    return fromIni(ini, iniPath.parent_path());
}

} // namespace openhogs::content
