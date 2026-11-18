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

namespace {

// The section a "[Name]" line opens (lower case), or nothing for other lines.
std::optional<std::string> sectionOpenedBy(const std::string& trimmedLine) {
    if (trimmedLine.size() < 2 || trimmedLine.front() != '[' || trimmedLine.back() != ']') {
        return std::nullopt;
    }
    return toLower(trim(trimmedLine.substr(1, trimmedLine.size() - 2)));
}

// The key a "Key = Value" line sets (lower case), or nothing for other lines.
std::optional<std::string> keySetBy(const std::string& trimmedLine) {
    if (isCommentOrBlank(trimmedLine) || trimmedLine.front() == '[') {
        return std::nullopt;
    }
    const auto equals = trimmedLine.find('=');
    if (equals == std::string::npos) {
        return std::nullopt;
    }
    return toLower(trim(trimmedLine.substr(0, equals)));
}

std::vector<std::string> splitLines(std::string_view text) {
    std::vector<std::string> lines;
    std::istringstream stream{std::string(text)};
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();  // Windows line ends
        lines.push_back(line);
    }
    return lines;
}

std::string joinLines(const std::vector<std::string>& lines) {
    std::string text;
    for (const auto& line : lines) {
        text += line;
        text += '\n';
    }
    return text;
}

} // namespace

std::string setIniValue(std::string_view text, std::string_view section, std::string_view key,
                        std::string_view value) {
    std::vector<std::string> lines = splitLines(text);
    const std::string wantedSection = toLower(section);
    const std::string wantedKey = toLower(key);
    const std::string newLine = std::string(key) + " = " + std::string(value);

    bool inSection = false;
    std::optional<std::size_t> sectionHeader;  // line of "[Section]", if found
    std::optional<std::size_t> lastKeyLine;    // its last key line: a missing key goes after it
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const std::string line = trim(lines[i]);
        if (const auto opened = sectionOpenedBy(line)) {
            if (inSection) break;  // the wanted section has ended
            inSection = *opened == wantedSection;
            if (inSection) sectionHeader = i;
            continue;
        }
        if (!inSection) continue;
        if (const auto found = keySetBy(line)) {
            if (*found == wantedKey) {
                lines[i] = newLine;
                return joinLines(lines);
            }
            lastKeyLine = i;
        }
    }
    if (sectionHeader) {
        // The section exists but has no such key yet.
        const std::size_t after = lastKeyLine.value_or(*sectionHeader);
        lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(after) + 1, newLine);
    } else {
        // No such section: add it at the end, after a blank line.
        if (!lines.empty() && !trim(lines.back()).empty()) lines.emplace_back();
        lines.push_back("[" + std::string(section) + "]");
        lines.push_back(newLine);
    }
    return joinLines(lines);
}

void updateIniFile(const std::filesystem::path& path, std::string_view section,
                   const std::vector<std::pair<std::string, std::string>>& values) {
    std::string text;
    if (std::ifstream file(path, std::ios::binary); file) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        text = buffer.str();
    }
    for (const auto& [key, value] : values) {
        text = setIniValue(text, section, key, value);
    }
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out || !(out << text)) {
        throw std::runtime_error("Cannot write INI file: " + path.string());
    }
}

} // namespace openhogs::core
