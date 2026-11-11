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

// The Italian translation relies on accented capitals. These words come
// from the PlayStation Italian menus ("MODALIT{50}", "S{5A}", "GI{64}") and
// the codes match the glyphs drawn in the PC fonts. If the mapping drifted,
// every translated menu would show the wrong letters.
TEST_CASE("Accented letters decode to UTF-8 and encode back", "[formats][GameText]") {
    ByteBuilder modalita;
    modalita.raw("\x2E\x30\x25\x22\x2D\x2A\x35\x50");  // M O D A L I T À
    ByteBuilder si;
    si.u8(0x34).u8(0x5A);  // S Ì
    ByteBuilder giu;
    giu.u8(0x28).u8(0x2A).u8(0x64);  // G I Ù

    CHECK(decodeGameText(modalita.bytes()) == "MODALITÀ");
    CHECK(decodeGameText(si.bytes()) == "SÌ");
    CHECK(decodeGameText(giu.bytes()) == "GIÙ");

    // Translators type lower case: it must give the same bytes.
    CHECK(encodeGameText("modalità") == modalita.bytes());
    CHECK(encodeGameText("Giù") == giu.bytes());
}

// Icon codes (hearts, PlayStation buttons) have no character: they must
// survive a decode/encode round trip, or translated tables would be corrupt.
TEST_CASE("Icon codes round-trip through {xx} escapes", "[formats][GameText]") {
    ByteBuilder encoded;
    encoded.u8(0x2C).u8(0x45).u8(0x3F);  // 'K', circle button, heart

    const auto text = decodeGameText(encoded.bytes());
    CHECK(text == "K{45}{3F}");
    CHECK(encodeGameText(text) == encoded.bytes());
}

// Characters the fonts cannot draw, broken escapes and {00} (which would
// cut the string short inside the game) must be refused with an error.
TEST_CASE("encodeGameText rejects text the game cannot display", "[formats][GameText]") {
    CHECK_THROWS_AS(encodeGameText("10\xE2\x82\xAC"), FormatError);  // euro sign
    CHECK_THROWS_AS(encodeGameText("a~b"), FormatError);
    CHECK_THROWS_AS(encodeGameText("{5"), FormatError);
    CHECK_THROWS_AS(encodeGameText("{zz}"), FormatError);
    CHECK_THROWS_AS(encodeGameText("{00}"), FormatError);
}
