// Tests for formats::PsxDisc (PlayStation disc images) and decodeRaw15.
//
// The tests build tiny but valid ISO 9660 images in both layouts used by
// disc dumps (raw 2352-byte sectors and plain 2048-byte sectors), so they
// never need a real game disc.

#include "openhogs/formats/PsxDisc.h"
#include "openhogs/formats/RawImage.h"

#include "ByteBuilder.h"
#include "TempDirectory.h"
#include "openhogs/core/BinaryReader.h"  // FormatError

#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <string>
#include <vector>

using namespace openhogs::formats;
using openhogs::tests::ByteBuilder;
using openhogs::tests::TempDirectory;

namespace {

using Sector = std::vector<std::uint8_t>;

void putU32BothEndian(Sector& s, std::size_t offset, std::uint32_t value) {
    for (int i = 0; i < 4; ++i) {
        s[offset + static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(value >> (8 * i));       // little
        s[offset + 7 - static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(value >> (8 * i));   // big
    }
}

// Appends an ISO 9660 directory record to a directory sector.
void addRecord(Sector& dir, std::size_t& offset, const std::string& name, std::uint32_t extent,
               std::uint32_t size, bool isDirectory) {
    const std::size_t length = 33 + name.size() + (name.size() % 2 == 0 ? 1 : 0);
    dir[offset] = static_cast<std::uint8_t>(length);
    putU32BothEndian(dir, offset + 2, extent);
    putU32BothEndian(dir, offset + 10, size);
    dir[offset + 25] = isDirectory ? 0x02 : 0x00;
    dir[offset + 32] = static_cast<std::uint8_t>(name.size());
    for (std::size_t i = 0; i < name.size(); ++i) {
        dir[offset + 33 + i] = static_cast<std::uint8_t>(name[i]);
    }
    offset += length;
}

// Sectors of a small disc:
//   /README.TXT          "hello"
//   /TEXT/GITALIAN.BIN   2100 bytes spanning two sectors ('A'..., then 'B'...)
std::vector<Sector> makeDiscSectors() {
    std::vector<Sector> sectors(24, Sector(2048, 0));

    Sector& pvd = sectors[16];
    pvd[0] = 1;
    pvd[1] = 'C'; pvd[2] = 'D'; pvd[3] = '0'; pvd[4] = '0'; pvd[5] = '1';
    std::size_t rootOffset = 156;
    addRecord(pvd, rootOffset, std::string(1, '\0'), 18, 2048, true);

    std::size_t offset = 0;
    addRecord(sectors[18], offset, std::string(1, '\0'), 18, 2048, true);
    addRecord(sectors[18], offset, std::string(1, '\1'), 18, 2048, true);
    addRecord(sectors[18], offset, "TEXT", 19, 2048, true);
    addRecord(sectors[18], offset, "README.TXT;1", 20, 5, false);

    offset = 0;
    addRecord(sectors[19], offset, std::string(1, '\0'), 19, 2048, true);
    addRecord(sectors[19], offset, std::string(1, '\1'), 18, 2048, true);
    addRecord(sectors[19], offset, "GITALIAN.BIN;1", 21, 2100, false);

    const std::string hello = "hello";
    std::copy(hello.begin(), hello.end(), sectors[20].begin());
    std::fill(sectors[21].begin(), sectors[21].end(), 'A');
    std::fill(sectors[22].begin(), sectors[22].end(), 'B');
    return sectors;
}

void writeImage(const std::filesystem::path& path, bool raw) {
    std::ofstream file(path, std::ios::binary);
    for (const Sector& data : makeDiscSectors()) {
        if (raw) {
            // Sync pattern, 3-byte address, mode 2, 8-byte subheader, data, error correction.
            Sector sector(2352, 0);
            for (int i = 1; i <= 10; ++i) sector[static_cast<std::size_t>(i)] = 0xFF;
            sector[15] = 2;
            std::copy(data.begin(), data.end(), sector.begin() + 24);
            file.write(reinterpret_cast<const char*>(sector.data()), static_cast<std::streamsize>(sector.size()));
        } else {
            file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        }
    }
}

std::string asText(const std::vector<std::byte>& data) {
    return {reinterpret_cast<const char*>(data.data()), data.size()};
}

} // namespace

// Users have either raw .bin dumps (2352-byte sectors, PlayStation Mode 2)
// or plain .iso files. Both must expose the same files, including files in
// sub-folders and files longer than one sector.
TEST_CASE("PsxDisc reads files from raw and ISO images", "[formats][PsxDisc]") {
    TempDirectory folder;
    for (const bool raw : {true, false}) {
        const auto path = folder.path() / (raw ? "disc.bin" : "disc.iso");
        writeImage(path, raw);

        const PsxDisc disc(path);
        INFO(path.filename().string());
        CHECK(disc.sectorSize() == (raw ? 2352u : 2048u));
        CHECK(disc.listFiles() == std::vector<std::string>{"README.TXT", "TEXT/GITALIAN.BIN"});
        CHECK(asText(disc.readFile("README.TXT")) == "hello");

        const auto big = disc.readFile("TEXT/GITALIAN.BIN");
        REQUIRE(big.size() == 2100);
        CHECK(static_cast<char>(big[2047]) == 'A');
        CHECK(static_cast<char>(big[2048]) == 'B');
    }
}

// Disc paths appear with different spellings (lower case, '\', ";1"
// version suffix, leading '/'); all must find the same file.
TEST_CASE("PsxDisc paths are case-insensitive and ignore version suffixes", "[formats][PsxDisc]") {
    TempDirectory folder;
    writeImage(folder.path() / "disc.bin", true);
    const PsxDisc disc(folder.path() / "disc.bin");

    CHECK(disc.exists("\\text\\gitalian.bin;1"));
    CHECK(disc.exists("/TEXT/GITALIAN.BIN"));
    CHECK_FALSE(disc.exists("TEXT/MISSING.BIN"));
    CHECK_THROWS(disc.readFile("TEXT/MISSING.BIN"));
}

// A file that is not a disc image (e.g. an audio track or a random file
// picked by mistake) must be rejected with a clear error.
TEST_CASE("PsxDisc rejects files that are not disc images", "[formats][PsxDisc]") {
    TempDirectory folder;
    const auto path = folder.writeFile("track2.bin", std::string(40 * 2352, '\x55'));
    CHECK_THROWS_AS(PsxDisc(path), openhogs::core::FormatError);
}

// Raw briefing pictures have no header: the colour layout (5 bits red in
// the low bits) and the row order must match the PlayStation, otherwise
// the converted briefings would have swapped colours.
TEST_CASE("decodeRaw15 converts 15-bit pixels to opaque RGBA", "[formats][RawImage]") {
    ByteBuilder data;
    data.u16(0x001F).u16(0x7C00);  // red, blue

    const auto image = decodeRaw15(data.bytes(), 2, 1);

    REQUIRE(image.pixels.size() == 8);
    CHECK(image.pixels[0] == 255);  // red channel of pixel 0
    CHECK(image.pixels[2] == 0);
    CHECK(image.pixels[6] == 255);  // blue channel of pixel 1
    CHECK(image.pixels[7] == 255);  // always opaque
    CHECK_THROWS_AS(decodeRaw15(data.bytes(), 4, 4), openhogs::core::FormatError);
}
