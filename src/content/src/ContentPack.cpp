// ContentPack.cpp - see ContentPack.h for an overview.

#include "openhogs/content/ContentPack.h"

#include "openhogs/core/BinaryReader.h"  // FormatError
#include "openhogs/core/IniFile.h"
#include "openhogs/core/PathUtils.h"

#include <array>
#include <utility>

namespace fs = std::filesystem;

namespace openhogs::content {

namespace {

// Single table used for both directions of the conversion, so the two
// functions below can never disagree.
constexpr std::array<std::pair<PackType, std::string_view>, 6> kPackTypeNames{{
    {PackType::Language, "language"},
    {PackType::Audio, "audio"},
    {PackType::Music, "music"},
    {PackType::Textures, "textures"},
    {PackType::Maps, "maps"},
    {PackType::Misc, "misc"},
}};

// Pack ids end up in openhogs.ini lists and in folder names, so they are
// restricted to characters that are safe in both places.
bool isValidPackId(const std::string& id) {
    if (id.empty()) {
        return false;
    }
    for (const char c : id) {
        const bool allowed = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
        if (!allowed) {
            return false;
        }
    }
    return true;
}

} // namespace

std::optional<PackType> parsePackType(std::string_view text) {
    const std::string lowered = core::toLower(core::trim(text));
    for (const auto& [type, name] : kPackTypeNames) {
        if (name == lowered) {
            return type;
        }
    }
    return std::nullopt;
}

std::string_view toString(PackType type) {
    for (const auto& [candidate, name] : kPackTypeNames) {
        if (candidate == type) {
            return name;
        }
    }
    return "misc";
}

ContentPack ContentPack::fromManifest(const core::IniFile& manifest, const fs::path& packDirectory) {
    const std::string where = " (pack folder: " + packDirectory.string() + ")";

    if (!manifest.hasSection("Pack")) {
        throw core::FormatError("pack.ini has no [Pack] section" + where);
    }

    ContentPack pack;
    pack.directory = packDirectory;
    pack.dataDirectory = packDirectory / kPackDataFolderName;

    pack.id = core::toLower(manifest.getString("Pack", "Id"));
    if (!isValidPackId(pack.id)) {
        throw core::FormatError(
            "pack.ini: Id must use only a-z, 0-9, '-' and '_', got '" + pack.id + "'" + where);
    }

    const std::string typeText = manifest.getString("Pack", "Type", "misc");
    const auto type = parsePackType(typeText);
    if (!type) {
        throw core::FormatError("pack.ini: unknown Type '" + typeText + "'" + where);
    }
    pack.type = *type;

    pack.language = core::toLower(manifest.getString("Pack", "Language"));
    if (pack.type == PackType::Language && pack.language.empty()) {
        throw core::FormatError("pack.ini: language packs must set Language (e.g. 'it')" + where);
    }

    pack.name = manifest.getString("Pack", "Name", pack.id);
    pack.priority = manifest.getInt("Pack", "Priority", 0);
    pack.author = manifest.getString("Pack", "Author");
    pack.version = manifest.getString("Pack", "Version");
    pack.description = manifest.getString("Pack", "Description");

    return pack;
}

ContentPack ContentPack::load(const fs::path& packDirectory) {
    const auto manifest = core::IniFile::load(packDirectory / kPackManifestName);
    return fromManifest(manifest, packDirectory);
}

} // namespace openhogs::content
