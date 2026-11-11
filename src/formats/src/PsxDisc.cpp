// PsxDisc.cpp - see PsxDisc.h for an overview.

#include "openhogs/formats/PsxDisc.h"

#include "openhogs/core/BinaryReader.h"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace openhogs::formats {

namespace {

constexpr std::size_t kRawSectorSize = 2352;
constexpr std::size_t kIsoSectorSize = 2048;
constexpr std::size_t kDataSize = 2048;

// Raw sectors start with this 12-byte sync pattern.
constexpr std::array<std::uint8_t, 12> kSyncPattern{0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};

// ISO 9660 layout.
constexpr std::uint32_t kPrimaryVolumeDescriptorSector = 16;
constexpr std::size_t kRootRecordOffset = 156;
constexpr std::uint8_t kDirectoryFlag = 0x02;
constexpr int kMaxDirectoryDepth = 16;  // protects against corrupt, looping trees

std::uint32_t readU32At(const std::vector<std::byte>& data, std::size_t offset) {
    core::BinaryReader reader(data);
    reader.seek(offset);
    return reader.readU32();
}

} // namespace

std::string normalisePsxPath(std::string_view path) {
    std::string result;
    for (char c : path) {
        if (c == '\\') c = '/';
        if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
        if (c == '/' && (result.empty() || result.back() == '/')) continue;
        result.push_back(c);
    }
    // "FILE.BIN;1" -> "FILE.BIN": ISO 9660 version numbers are not part of the name.
    if (const auto semicolon = result.rfind(';'); semicolon != std::string::npos) {
        result.erase(semicolon);
    }
    return result;
}

PsxDisc::PsxDisc(const std::filesystem::path& imagePath)
    : file_(imagePath, std::ios::binary) {
    if (!file_) {
        throw std::runtime_error("Cannot open disc image: " + imagePath.string());
    }

    // Raw images start with the sync pattern; plain ISO images do not.
    std::array<char, 16> header{};
    file_.read(header.data(), header.size());
    const bool isRaw = std::equal(kSyncPattern.begin(), kSyncPattern.end(), header.begin(),
                                  [](std::uint8_t a, char b) { return a == static_cast<std::uint8_t>(b); });
    if (isRaw) {
        sectorSize_ = kRawSectorSize;
        // Byte 15 is the sector mode: Mode 2 (PlayStation) has an 8-byte
        // subheader before the data, Mode 1 does not.
        dataOffset_ = (static_cast<std::uint8_t>(header[15]) == 2) ? 24 : 16;
    } else {
        sectorSize_ = kIsoSectorSize;
        dataOffset_ = 0;
    }

    const auto descriptor = readSector(kPrimaryVolumeDescriptorSector);
    const auto byteAt = [&](std::size_t i) { return std::to_integer<char>(descriptor[i]); };
    if (byteAt(0) != 1 || byteAt(1) != 'C' || byteAt(2) != 'D' || byteAt(3) != '0' || byteAt(4) != '0' ||
        byteAt(5) != '1') {
        throw core::FormatError("Not a PlayStation/ISO 9660 disc image: " + imagePath.string());
    }

    readDirectory(readU32At(descriptor, kRootRecordOffset + 2), readU32At(descriptor, kRootRecordOffset + 10),
                  "", 0);
}

std::vector<std::byte> PsxDisc::readSector(std::uint32_t sector) const {
    std::vector<std::byte> data(kDataSize);
    file_.clear();
    file_.seekg(static_cast<std::streamoff>(std::uint64_t{sector} * sectorSize_ + dataOffset_));
    file_.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(kDataSize));
    if (!file_) {
        throw core::FormatError("Disc image is truncated: cannot read sector " + std::to_string(sector));
    }
    return data;
}

void PsxDisc::readDirectory(std::uint32_t firstSector, std::uint32_t size, const std::string& prefix, int depth) {
    if (depth > kMaxDirectoryDepth) {
        throw core::FormatError("Disc image directory tree is too deep (corrupt image?)");
    }

    const auto sectorCount = static_cast<std::uint32_t>((size + kDataSize - 1) / kDataSize);
    for (std::uint32_t s = 0; s < sectorCount; ++s) {
        const auto data = readSector(firstSector + s);

        std::size_t offset = 0;
        while (offset < kDataSize) {
            const auto recordLength = std::to_integer<std::size_t>(data[offset]);
            if (recordLength == 0) {
                break;  // records never cross sectors: the rest is padding
            }
            if (offset + recordLength > kDataSize || recordLength < 34) {
                throw core::FormatError("Corrupt directory record on the disc image");
            }

            const std::uint32_t extent = readU32At(data, offset + 2);
            const std::uint32_t length = readU32At(data, offset + 10);
            const auto flags = std::to_integer<std::uint8_t>(data[offset + 25]);
            const auto nameLength = std::to_integer<std::size_t>(data[offset + 32]);
            std::string name;
            for (std::size_t i = 0; i < nameLength && offset + 33 + i < kDataSize; ++i) {
                name.push_back(std::to_integer<char>(data[offset + 33 + i]));
            }
            offset += recordLength;

            // Names "\0" and "\1" are the entries for "." and "..".
            if (name.size() == 1 && (name[0] == '\0' || name[0] == '\1')) {
                continue;
            }
            const std::string path = normalisePsxPath(prefix + name);
            if (flags & kDirectoryFlag) {
                readDirectory(extent, length, path + "/", depth + 1);
            } else {
                files_[path] = {extent, length};
            }
        }
    }
}

std::vector<std::string> PsxDisc::listFiles() const {
    std::vector<std::string> paths;
    for (const auto& [path, location] : files_) {
        paths.push_back(path);
    }
    return paths;
}

bool PsxDisc::exists(std::string_view path) const {
    return files_.contains(normalisePsxPath(path));
}

std::vector<std::byte> PsxDisc::readFile(std::string_view path) const {
    const auto it = files_.find(normalisePsxPath(path));
    if (it == files_.end()) {
        throw std::runtime_error("File not found on the disc image: " + std::string(path));
    }

    std::vector<std::byte> content;
    content.reserve(it->second.size);
    std::uint32_t remaining = it->second.size;
    for (std::uint32_t sector = it->second.firstSector; remaining > 0; ++sector) {
        const auto data = readSector(sector);
        const std::uint32_t take = std::min<std::uint32_t>(remaining, kDataSize);
        content.insert(content.end(), data.begin(), data.begin() + take);
        remaining -= take;
    }
    return content;
}

} // namespace openhogs::formats
