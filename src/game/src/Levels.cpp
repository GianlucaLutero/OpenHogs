// Levels.cpp - see Levels.h for an overview.

#include "openhogs/game/Levels.h"

#include "openhogs/core/PathUtils.h"

#include <array>
#include <stdexcept>
#include <string>

namespace openhogs::game {

namespace {

using S = SkyType;

constexpr LevelInfo makeLevel(int number, std::string_view map, SkyType sky, scene::PaletteGrading grading) {
    return {number, map, kFirstLevelNameEntry + static_cast<std::size_t>(number), sky, grading};
}

// The recolourings of the level table (fields 1 to 4: desaturation,
// brightness, tint per cent, tint), named after their look. Eight sets
// are shared by the 53 levels.
constexpr scene::PaletteGrading kPlain{};
constexpr scene::PaletteGrading kGreen{0, 0, 15, {0, 25, 0}};     // sunny meadows
constexpr scene::PaletteGrading kWarm{0, -2, 10, {25, 0, 0}};     // deserts and sunsets
constexpr scene::PaletteGrading kBright{0, 2, 0, {0, 0, 0}};      // the rainy maps
constexpr scene::PaletteGrading kBrighter1{0, 1, 0, {0, 0, 0}};   // RUMBLE
constexpr scene::PaletteGrading kCool{0, -2, 0, {0, 0, 0}};       // snow
constexpr scene::PaletteGrading kDusk{2, 0, 25, {11, 5, 0}};      // OASIS, SEPIA1
constexpr scene::PaletteGrading kNight{2, 3, 60, {3, 3, 13}};     // night and the moon

// Names from the pointer array at 0xD0790; skies and recolourings from the
// level table at 0x4D5210 (field 0, values 0..10, see SkyType; fields 1-4).
constexpr std::array<LevelInfo, 53> kLevels{{
    makeLevel(0, "ROAD", S::Sunny, kGreen),
    makeLevel(1, "TRENCH", S::Ominous, kBright),
    makeLevel(2, "RUMBLE", S::Sunny, kBrighter1),
    makeLevel(3, "DEVI", S::Desert, kWarm),
    makeLevel(4, "TWIN", S::Sunrise, kPlain),
    makeLevel(5, "ZULUS", S::Sunrise, kPlain),
    makeLevel(6, "SNIPER", S::Sunset, kWarm),
    makeLevel(7, "GUNS", S::Desert, kWarm),
    makeLevel(8, "OASIS", S::Sunset, kDusk),
    makeLevel(9, "MASHED", S::Sunny, kGreen),
    makeLevel(10, "CAMP", S::Cold, kCool),
    makeLevel(11, "LIBERATE", S::Sunrise, kPlain),
    makeLevel(12, "MEDIX", S::Ominous, kBright),
    makeLevel(13, "FJORDS", S::Cold, kCool),
    makeLevel(14, "EYRIE", S::Sunset, kWarm),
    makeLevel(15, "BRIDGE", S::Sunny, kGreen),
    makeLevel(16, "BAY", S::Night1, kNight),
    makeLevel(17, "DESVAL", S::Desert, kWarm),
    makeLevel(18, "SNAKE", S::Cold, kCool),
    makeLevel(19, "EMPLACE", S::Ominous, kBright),
    makeLevel(20, "KEEP", S::Cold, kCool),
    makeLevel(21, "SUPLINE", S::Sunny, kGreen),
    makeLevel(22, "TESTER", S::Desert, kWarm),
    makeLevel(23, "FOOT", S::Cold, kCool),
    makeLevel(24, "FINAL", S::Ominous, kBright),
    makeLevel(25, "ESTU", S::Sunny, kGreen),
    makeLevel(26, "DEMO", S::Desert, kWarm),
    makeLevel(27, "BOOM", S::Desert, kPlain),
    makeLevel(28, "BHILL", S::Sunrise, kPlain),
    makeLevel(29, "LECPROD", S::Cold, kCool),
    makeLevel(30, "DVAL", S::Sunny, kGreen),
    makeLevel(31, "ICE", S::Cold, kPlain),
    makeLevel(32, "BUTE", S::Space, kNight),
    makeLevel(33, "MAZE", S::Sunny, kGreen),
    makeLevel(34, "SEPIA1", S::Sunny, kDusk),
    makeLevel(35, "DBOWL", S::Desert, kGreen),
    makeLevel(36, "MLAKE", S::Cold, kCool),
    makeLevel(37, "CMASS", S::Space, kNight),
    makeLevel(38, "ARTGUN", S::Sunny, kGreen),
    makeLevel(39, "DVAL2", S::Ominous, kBright),
    makeLevel(40, "HELL3", S::Desert, kPlain),
    makeLevel(41, "HELL2", S::Desert, kPlain),
    makeLevel(42, "LUNAR1", S::Space, kNight),
    makeLevel(43, "CREEPY2", S::Night1, kNight),
    makeLevel(44, "PLAY1", S::Toy, kGreen),
    makeLevel(45, "PLAY2", S::Toy, kGreen),
    makeLevel(46, "ICEFLOW", S::Cold, kCool),
    makeLevel(47, "RIDGE", S::Sunset, kWarm),
    makeLevel(48, "ARCHI", S::Sunset, kWarm),
    makeLevel(49, "DEMO2", S::Cold, kCool),
    makeLevel(50, "ISLAND", S::Sunrise, kPlain),
    makeLevel(51, "LAKE", S::Sunrise, kPlain),
    makeLevel(52, "ONEWAY", S::Desert, kWarm),
}};

// The executable's sky settings: FUN_004866b0 picks the dome's pictures,
// FUN_00485550 sets the fog (afSetFog) per sky: start, end and colour.
// Every sky has fog.
SkyLook executableSkyLook(SkyType sky) {
    switch (sky) {
        case SkyType::Cold: return {"chars/coldsky.mad", 238, 4524, {0xF8, 0xF8, 0xF8}};
        case SkyType::Desert: return {"chars/desert.mad", 238, 3571, {0xFC, 0xC0, 0x74}};
        case SkyType::Night1:
        case SkyType::Night2:
        case SkyType::Night3: return {"chars/night1.mad", 238, 2749, {0, 0, 0}};
        case SkyType::Ominous: return {"chars/ominous.mad", 425, 2125, {0x8F, 0xAF, 0xCD}};
        case SkyType::Sunny: return {"chars/sunny.mad", 238, 4048, {0xD0, 0xD7, 0xE0}};
        case SkyType::Sunrise: return {"chars/sunrise.mad", 238, 4048, {0xE5, 0xBF, 0x80}};
        case SkyType::Sunset: return {"chars/sunset.mad", 238, 4048, {0xFF, 0xD0, 0x9F}};
        case SkyType::Toy: return {"chars/toy.mad", 238, 4048, {0xC0, 0xFF, 0xFF}};
        case SkyType::Space: return {"chars/space.mad", 238, 2749, {0, 0, 0}};
    }
    return {"chars/sunny.mad", 238, 4048, {0xD0, 0xD7, 0xE0}};
}


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

SkyLook skyLook(SkyType sky) {
    SkyLook look = executableSkyLook(sky);
    look.fogStart *= kFogDistanceScale;
    look.fogEnd *= kFogDistanceScale;
    return look;
}

bool isNightSky(SkyType sky) {
    // FUN_00485550: the day bank for skies 0, 1 and 5..7 and above 8, the
    // night bank for 2, 3, 4 and 8.
    return sky == SkyType::Night1 || sky == SkyType::Night2 || sky == SkyType::Night3 || sky == SkyType::Sunset;
}

} // namespace openhogs::game
