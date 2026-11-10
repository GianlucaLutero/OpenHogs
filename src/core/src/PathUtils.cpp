// PathUtils.cpp - see PathUtils.h for an overview.

#include "openhogs/core/PathUtils.h"

#include <cctype>

namespace openhogs::core {

std::string toLower(std::string_view text) {
    std::string result(text);
    for (char& c : result) {
        // The cast avoids undefined behaviour for bytes above 127.
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
}

std::string trim(std::string_view text) {
    constexpr std::string_view whitespace = " \t\r\n";

    const auto first = text.find_first_not_of(whitespace);
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = text.find_last_not_of(whitespace);
    return std::string(text.substr(first, last - first + 1));
}

std::string makeLookupKey(std::string_view relativePath) {
    std::string key;
    key.reserve(relativePath.size());

    for (char c : relativePath) {
        if (c == '\\') {
            c = '/';
        }
        // Skip separators at the start and collapse "//" into "/".
        if (c == '/' && (key.empty() || key.back() == '/')) {
            continue;
        }
        key.push_back(c);
    }

    // Remove any leading "./" segments.
    while (key.starts_with("./")) {
        key.erase(0, 2);
    }

    return toLower(key);
}

} // namespace openhogs::core
