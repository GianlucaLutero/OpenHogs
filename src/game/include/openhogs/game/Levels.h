// Levels.h
//
// The game's numbered levels and what the executable says about each one.
//
// warhogs_.exe keeps an array of the level names (map files) at file offset
// 0xD0790, and a table of 60-byte level properties at 0xD4010 (address
// 0x4D5210) whose first field is the level's sky. The level's name in the
// game text table gtext is entry 11 + level number. See
// docs/formats/executable.md.
//
// Generated levels (GENSNOW, GENDESRT...) follow the 53 fixed ones; they
// are not playable yet.

#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>

namespace openhogs::game {

// The sky of a level: which sky dome texture it uses (Chars/<sky>.mad),
// whether it is night (night sounds) and its weather.
enum class SkyType {
    Cold,     // coldsky.mad, snow falls
    Desert,   // desert.mad
    Night1,   // night1.mad (the three nights share it)
    Night2,
    Night3,
    Ominous,  // ominous.mad, rain falls
    Sunny,    // sunny.mad
    Sunrise,  // sunrise.mad
    Sunset,   // sunset.mad (counts as night for the sounds)
    Toy,      // toy.mad (the toy-room levels)
    Space,    // space.mad (the moon levels)
};

struct LevelInfo {
    int number = 0;            // index in the executable's tables
    std::string_view map;      // Maps/<map>.PMG ...
    std::size_t nameEntry = 0; // gtext entry with the level's name
    SkyType sky = SkyType::Sunny;
};

inline constexpr std::size_t kFirstLevelNameEntry = 11;

// The 53 fixed levels, in the executable's order.
std::span<const LevelInfo> allLevels();

// The level of a map (case-insensitive); empty for maps that are not levels.
std::optional<LevelInfo> findLevel(std::string_view map);

// Level `number`. Throws std::out_of_range for an unknown number.
const LevelInfo& level(int number);

// The original plays the night sound bank (Audio/sfxnight.srl) under these
// skies: the three nights and the sunset.
bool isNightSky(SkyType sky);

} // namespace openhogs::game
