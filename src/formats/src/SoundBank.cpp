// SoundBank.cpp - see SoundBank.h for an overview.

#include "openhogs/formats/SoundBank.h"

#include "openhogs/core/BinaryReader.h"  // FormatError
#include "openhogs/core/PathUtils.h"

#include <sstream>

namespace openhogs::formats {

namespace {

// Parses a whole line as a non-negative number.
std::optional<std::size_t> parseNumber(const std::string& text) {
    if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos) {
        return std::nullopt;
    }
    std::size_t value = 0;
    for (const char c : text) {
        value = value * 10 + static_cast<std::size_t>(c - '0');
    }
    return value;
}

} // namespace

std::optional<std::string> SoundBank::file(std::size_t soundNumber) const {
    if (soundNumber >= files.size()) {
        return std::nullopt;
    }
    return files[soundNumber];
}

SoundBank SoundBank::parse(std::span<const std::byte> data) {
    // Collect the non-empty lines, without line ends or surrounding spaces.
    std::istringstream stream(std::string(reinterpret_cast<const char*>(data.data()), data.size()));
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(stream, line)) {
        line = core::trim(line);
        if (!line.empty()) {
            lines.push_back(line);
        }
    }

    const auto fail = [](const std::string& message) { throw core::FormatError("SRL sound bank: " + message); };

    if (lines.size() < 4) {
        fail("too short");
    }
    const auto count = parseNumber(lines[0]);
    if (!count) {
        fail("first line must be the number of sounds, got '" + lines[0] + "'");
    }

    SoundBank bank;
    bank.name = lines[3];

    // Then pairs of lines: sound number, file.
    std::size_t next = 4;
    for (std::size_t sound = 0; sound < *count; ++sound) {
        if (next + 1 >= lines.size()) {
            fail("announces " + std::to_string(*count) + " sounds but lists only " + std::to_string(sound));
        }
        const auto number = parseNumber(lines[next]);
        if (!number || *number != sound) {
            fail("expected sound number " + std::to_string(sound) + ", got '" + lines[next] + "'");
        }
        bank.files.push_back(lines[next + 1]);
        next += 2;
    }
    return bank;
}

} // namespace openhogs::formats
