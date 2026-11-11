// TranslationOverrides.cpp - see TranslationOverrides.h for an overview.

#include "openhogs/langpack/TranslationOverrides.h"

#include "openhogs/core/BinaryReader.h"  // FormatError
#include "openhogs/core/IniFile.h"
#include "openhogs/core/PathUtils.h"

#include <stdexcept>

namespace openhogs::langpack {

namespace {

// The two text tables of the game that can be overridden.
constexpr const char* kTables[] = {"gtext", "fetext"};

std::size_t parseIndex(const std::string& key, const char* table) {
    std::size_t index = 0;
    bool valid = !key.empty();
    for (const char c : key) {
        valid = valid && c >= '0' && c <= '9';
        index = index * 10 + static_cast<std::size_t>(c - '0');
    }
    if (!valid) {
        throw core::FormatError(std::string("Translation overrides: [") + table + "] key '" + key +
                                "' must be a string index (a whole number)");
    }
    return index;
}

} // namespace

TranslationOverrides TranslationOverrides::fromIni(const core::IniFile& ini) {
    TranslationOverrides overrides;
    for (const char* table : kTables) {
        for (const auto& [key, text] : ini.entries(table)) {
            overrides.overrides_[table][parseIndex(key, table)] = text;
        }
    }
    return overrides;
}

TranslationOverrides TranslationOverrides::load(const std::filesystem::path& path) {
    return fromIni(core::IniFile::load(path));
}

std::size_t TranslationOverrides::apply(std::string_view table, std::vector<std::string>& entries) const {
    const auto it = overrides_.find(core::toLower(table));
    if (it == overrides_.end()) {
        return 0;
    }
    for (const auto& [index, text] : it->second) {
        if (index >= entries.size()) {
            throw std::runtime_error("Translation overrides: [" + std::string(table) + "] " + std::to_string(index) +
                                     " is outside the table (" + std::to_string(entries.size()) + " strings)");
        }
        entries[index] = text;
    }
    return it->second.size();
}

} // namespace openhogs::langpack
