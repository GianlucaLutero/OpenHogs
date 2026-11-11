// SoundBank.h
//
// Reader for .SRL files: the lists that give each sound of the game its
// number. The game asks for "sound 39 of the effects bank" (the machine
// gun) rather than for a file name, so these lists are the link between
// game code and the .WAV files. See docs/formats/audio.md.
//
// An SRL file is plain text, one value per line (CRLF line ends):
//
//     099                 number of sounds
//     000                 unknown, always 000
//     000                 unknown, always 000
//     AUDIO\SFXDAY        bank name
//     0                   sound number
//     AUDIO\AMB_1D.wav    its file
//     1
//     AUDIO\AMB_2D.wav
//     ...

#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace openhogs::formats {

struct SoundBank {
    std::string name;                // e.g. "AUDIO\SFXDAY"
    std::vector<std::string> files;  // index = sound number, game-relative paths

    // File of a sound number, or std::nullopt if the bank has no such sound.
    std::optional<std::string> file(std::size_t soundNumber) const;

    // Throws core::FormatError if the text does not follow the layout above
    // (wrong count, numbers out of order, missing lines).
    static SoundBank parse(std::span<const std::byte> data);
};

} // namespace openhogs::formats
