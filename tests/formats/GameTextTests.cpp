// Tests for the game text encoding (formats::decodeGameText / encodeGameText).
//
// Translated text will be encoded with these functions, so they must agree
// with the original files byte for byte.

#include "openhogs/formats/GameText.h"

#include "ByteBuilder.h"
#include "openhogs/core/BinaryReader.h"  // FormatError

#include <catch2/catch_test_macros.hpp>

using namespace openhogs::formats;
using openhogs::core::FormatError;
using openhogs::tests::ByteBuilder;

// Sample taken from the structure of gtext.bin: "I SPY..." is stored as
// I=0x2A, space=0x01, S=0x34, P=0x31, Y=0x3A, '.'=0x0F. This pins down the
// mapping (ASCII = code + 0x1F, space = 0x01) found in the real files.
TEST_CASE("decodeGameText maps font codes to readable characters", "[formats][GameText]") {
    ByteBuilder encoded;
    encoded.u8(0x2A).u8(0x01).u8(0x34).u8(0x31).u8(0x3A).u8(0x0F).u8(0x0F).u8(0x0F);

    CHECK(decodeGameText(encoded.bytes()) == "I SPY...");
}

// Codes outside the ASCII range (accented letters in other languages) must
// survive a decode/encode round trip, or translated tables would be corrupt.
TEST_CASE("Unknown font codes round-trip through {xx} escapes", "[formats][GameText]") {
    ByteBuilder encoded;
    encoded.u8(0x2C).u8(0x50).u8(0x68);  // 'K' followed by two extra glyphs

    const auto text = decodeGameText(encoded.bytes());
    CHECK(text == "K{50}{68}");
    CHECK(encodeGameText(text) == encoded.bytes());
}

// Translators will naturally type lower case; the font only has capitals,
// so lower case must be accepted and stored as upper case.
TEST_CASE("encodeGameText stores lower case as upper case", "[formats][GameText]") {
    CHECK(encodeGameText("Bye bye") == encodeGameText("BYE BYE"));
}

// Characters the font cannot draw, broken escapes and {00} (which would cut
// the string short inside the game) must be refused with an error.
TEST_CASE("encodeGameText rejects text the game cannot display", "[formats][GameText]") {
    CHECK_THROWS_AS(encodeGameText("caf\xC3\xA8"), FormatError);  // UTF-8 'è' without a code
    CHECK_THROWS_AS(encodeGameText("{5"), FormatError);
    CHECK_THROWS_AS(encodeGameText("{zz}"), FormatError);
    CHECK_THROWS_AS(encodeGameText("{00}"), FormatError);
}
