// IniFile.h
//
// Minimal reader for Windows-style .ini files.
//
// INI is used for every user-editable setting in OpenHogs (config/openhogs.ini,
// the pack.ini manifest of each content pack) because it is easy to edit by
// hand and it is the format the original game already used (warhogs_.ini).
//
// Supported syntax:
//
//     ; comment            # comment too
//     [Section]
//     Key = value with spaces
//
// Section and key names are case-insensitive, like in the Windows API the
// original game relied on. Values are returned with surrounding spaces
// removed but are otherwise untouched (so Windows paths keep their '\').

#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace openhogs::core {

class IniFile {
public:
    // Parses INI text. Throws FormatError on malformed lines, reporting the
    // line number so that users can fix their configuration easily.
    static IniFile parse(std::string_view text);

    // Reads and parses a file. Throws std::runtime_error if it cannot be opened.
    static IniFile load(const std::filesystem::path& path);

    bool hasSection(std::string_view section) const;

    // Returns the value, or std::nullopt if the section or key is missing.
    std::optional<std::string> get(std::string_view section, std::string_view key) const;

    // Convenience getters with a default for missing keys.
    std::string getString(std::string_view section, std::string_view key,
                          std::string_view fallback = {}) const;
    int getInt(std::string_view section, std::string_view key, int fallback) const;
    bool getBool(std::string_view section, std::string_view key, bool fallback) const;

    // Splits a comma-separated value ("a, b ,c") into trimmed, non-empty items.
    std::vector<std::string> getList(std::string_view section, std::string_view key) const;

private:
    // Keys are stored lower-case to make lookups case-insensitive.
    using Section = std::map<std::string, std::string>;
    std::map<std::string, Section> sections_;
};

} // namespace openhogs::core
