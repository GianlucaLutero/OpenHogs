// Tests for core::makeLookupKey.
//
// Every file lookup in OpenHogs goes through this function, so it decides
// whether a name written inside the game data finds the file on disk.

#include "openhogs/core/PathUtils.h"

#include <catch2/catch_test_macros.hpp>

using openhogs::core::makeLookupKey;

// The game data mixes case and Windows separators ("Language\Tims\Level1.BMP"
// next to "level10.BMP"). All spellings of the same file must map to one key,
// otherwise files would be "missing" on Linux or inside packs.
TEST_CASE("makeLookupKey normalises separators, case and prefixes", "[core][PathUtils]") {
    CHECK(makeLookupKey(".\\Speech\\Sku1\\Pig01\\01AM0101.WAV") == "speech/sku1/pig01/01am0101.wav");
    CHECK(makeLookupKey("/Maps//CAMP.MAD") == "maps/camp.mad");
    CHECK(makeLookupKey("maps/camp.mad") == makeLookupKey("MAPS\\Camp.Mad"));
}
