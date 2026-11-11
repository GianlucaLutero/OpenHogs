// IniFile.cpp - see IniFile.h for an overview.

#include "openhogs/core/IniFile.h"

#include "openhogs/core/BinaryReader.h"  // FormatError
#include "openhogs/core/PathUtils.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace openhogs::core {

namespace {

bool isCommentOrBlank(const std::string& line) {
    return line.empty() || line.front() == ';' || line.front() == '#';
}

std::string formatLineError(std::size_t lineNumber, const std::string& message) {
    return "INI line " + std::to_string(lineNumber) + ": " + message;
}

} // namespace

IniFile IniFile::parse(std::string_view text) {
    IniFile ini;
    std::string currentSection;
    bool insideSection = false;

    std::istringstream stream{std::string(text)};
    std::string rawLine;
    std::size_t lineNumber = 0;

    while (std::getline(stream, rawLine)) {
        ++lineNumber;
        const std::string line = trim(rawLine);

        if (isCommentOrBlank(line)) {
            continue;
        }

        // "[Section]"
        if (line.front() == '[') {
            if (line.back() != ']') {
                throw FormatError(formatLineError(lineNumber, "missing ']' in section header"));
            }
            currentSection = toLower(trim(line.substr(1, line.size() - 2)));
            ini.sections_[currentSection];  // create it even if it stays empty
            insideSection = true;
            continue;
        }

        // "Key = Value"
        const auto equals = line.find('=');
        if (equals == std::string::npos) {
            throw FormatError(formatLineError(lineNumber, "expected 'Key = Value'"));
        }
        if (!insideSection) {
            throw FormatError(formatLineError(lineNumber, "key found before any [Section]"));
        }

        const std::string key = toLower(trim(line.substr(0, equals)));
        if (key.empty()) {
            throw FormatError(formatLineError(lineNumber, "empty key name"));
        }
        ini.sections_[currentSection][key] = trim(line.substr(equals + 1));
    }

    return ini;
}

IniFile IniFile::load(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open INI file: " + path.string());
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return parse(buffer.str());
}

bool IniFile::hasSection(std::string_view section) const {
    return sections_.contains(toLower(section));
}

std::optional<std::string> IniFile::get(std::string_view section, std::string_view key) const {
    const auto sectionIt = sections_.find(toLower(section));
    if (sectionIt == sections_.end()) {
        return std::nullopt;
    }
    const auto keyIt = sectionIt->second.find(toLower(key));
    if (keyIt == sectionIt->second.end()) {
        return std::nullopt;
    }
    return keyIt->second;
}

std::string IniFile::getString(std::string_view section, std::string_view key,
                               std::string_view fallback) const {
    return get(section, key).value_or(std::string(fallback));
}

int IniFile::getInt(std::string_view section, std::string_view key, int fallback) const {
    const auto value = get(section, key);
    if (!value) {
        return fallback;
    }
    try {
        std::size_t parsedLength = 0;
        const int number = std::stoi(*value, &parsedLength);
        // Reject values like "12abc" instead of silently using 12.
        if (parsedLength != value->size()) {
            throw std::invalid_argument("trailing characters");
        }
        return number;
    } catch (const std::exception&) {
        throw FormatError("[" + std::string(section) + "] " + std::string(key) +
                          " must be a whole number, got '" + *value + "'");
    }
}

bool IniFile::getBool(std::string_view section, std::string_view key, bool fallback) const {
    const auto value = get(section, key);
    if (!value) {
        return fallback;
    }
    const std::string lowered = toLower(*value);
    if (lowered == "1" || lowered == "true" || lowered == "yes" || lowered == "on") {
        return true;
    }
    if (lowered == "0" || lowered == "false" || lowered == "no" || lowered == "off") {
        return false;
    }
    throw FormatError("[" + std::string(section) + "] " + std::string(key) +
                      " must be true/false, got '" + *value + "'");
}

std::vector<std::pair<std::string, std::string>> IniFile::entries(std::string_view section) const {
    const auto it = sections_.find(toLower(section));
    if (it == sections_.end()) {
        return {};
    }
    return {it->second.begin(), it->second.end()};
}

std::vector<std::string> IniFile::getList(std::string_view section, std::string_view key) const {
    std::vector<std::string> items;
    const std::string value = getString(section, key);

    std::size_t start = 0;
    while (start <= value.size()) {
        auto comma = value.find(',', start);
        if (comma == std::string::npos) {
            comma = value.size();
        }
        std::string item = trim(value.substr(start, comma - start));
        if (!item.empty()) {
            items.push_back(std::move(item));
        }
        start = comma + 1;
    }
    return items;
}

} // namespace openhogs::core
