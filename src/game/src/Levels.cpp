// Levels.cpp - see Levels.h for an overview.

#include "openhogs/game/Levels.h"

#include "openhogs/core/PathUtils.h"

#include <array>
#include <stdexcept>
#include <string>

namespace openhogs::game {

namespace {

using S = SkyType;

constexpr LevelInfo makeLevel(int number, std::string_view map, SkyType sky) {
    return {number, map, kFirstLevelNameEntry + static_cast<std::size_t>(number), sky};
}

// Names from the pointer array at 0xD0790, skies from the first field of
// the level table at 0x4D5210 (values 0..10, see SkyType).
constexpr std::array<LevelInfo, 53> kLevels{{
    makeLevel(0, "ROAD", S::Sunny),       makeLevel(1, "TRENCH", S::Ominous),  makeLevel(2, "RUMBLE", S::Sunny),
    makeLevel(3, "DEVI", S::Desert),      makeLevel(4, "TWIN", S::Sunrise),    makeLevel(5, "ZULUS", S::Sunrise),
    makeLevel(6, "SNIPER", S::Sunset),    makeLevel(7, "GUNS", S::Desert),     makeLevel(8, "OASIS", S::Sunset),
    makeLevel(9, "MASHED", S::Sunny),     makeLevel(10, "CAMP", S::Cold),      makeLevel(11, "LIBERATE", S::Sunrise),
    makeLevel(12, "MEDIX", S::Ominous),   makeLevel(13, "FJORDS", S::Cold),    makeLevel(14, "EYRIE", S::Sunset),
    makeLevel(15, "BRIDGE", S::Sunny),    makeLevel(16, "BAY", S::Night1),     makeLevel(17, "DESVAL", S::Desert),
    makeLevel(18, "SNAKE", S::Cold),      makeLevel(19, "EMPLACE", S::Ominous), makeLevel(20, "KEEP", S::Cold),
    makeLevel(21, "SUPLINE", S::Sunny),   makeLevel(22, "TESTER", S::Desert),  makeLevel(23, "FOOT", S::Cold),
    makeLevel(24, "FINAL", S::Ominous),   makeLevel(25, "ESTU", S::Sunny),     makeLevel(26, "DEMO", S::Desert),
    makeLevel(27, "BOOM", S::Desert),     makeLevel(28, "BHILL", S::Sunrise),  makeLevel(29, "LECPROD", S::Cold),
    makeLevel(30, "DVAL", S::Sunny),      makeLevel(31, "ICE", S::Cold),       makeLevel(32, "BUTE", S::Space),
    makeLevel(33, "MAZE", S::Sunny),      makeLevel(34, "SEPIA1", S::Sunny),   makeLevel(35, "DBOWL", S::Desert),
    makeLevel(36, "MLAKE", S::Cold),      makeLevel(37, "CMASS", S::Space),    makeLevel(38, "ARTGUN", S::Sunny),
    makeLevel(39, "DVAL2", S::Ominous),   makeLevel(40, "HELL3", S::Desert),   makeLevel(41, "HELL2", S::Desert),
    makeLevel(42, "LUNAR1", S::Space),    makeLevel(43, "CREEPY2", S::Night1), makeLevel(44, "PLAY1", S::Toy),
    makeLevel(45, "PLAY2", S::Toy),       makeLevel(46, "ICEFLOW", S::Cold),   makeLevel(47, "RIDGE", S::Sunset),
    makeLevel(48, "ARCHI", S::Sunset),    makeLevel(49, "DEMO2", S::Cold),     makeLevel(50, "ISLAND", S::Sunrise),
    makeLevel(51, "LAKE", S::Sunrise),    makeLevel(52, "ONEWAY", S::Desert),
}};

} // namespace

std::span<const LevelInfo> allLevels() {
    return kLevels;
}

std::optional<LevelInfo> findLevel(std::string_view map) {
    const std::string key = core::makeLookupKey(map);
    for (const auto& info : kLevels) {
        if (core::makeLookupKey(info.map) == key) return info;
    }
    return std::nullopt;
}

const LevelInfo& level(int number) {
    if (number < 0 || number >= static_cast<int>(kLevels.size())) {
        throw std::out_of_range("No level " + std::to_string(number));
    }
    return kLevels[static_cast<std::size_t>(number)];
}

bool isNightSky(SkyType sky) {
    // FUN_00485550: the day bank for skies 0, 1 and 5..7 and above 8, the
    // night bank for 2, 3, 4 and 8.
    return sky == SkyType::Night1 || sky == SkyType::Night2 || sky == SkyType::Night3 || sky == SkyType::Sunset;
}

} // namespace openhogs::game
